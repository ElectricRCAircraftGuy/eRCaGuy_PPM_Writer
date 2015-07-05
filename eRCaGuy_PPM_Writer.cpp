/*
eRCaGuy_PPM_Writer
By Gabriel Staples
http://www.ElectricRCAircraftGuy.com 

FOR PERTINENT INFORMATION ABOUT THIS LIBRARY SEE THE TOP OF THE HEADER (.h) FILE, AND THE COMMENTS IN BOTH THIS FILE AND THE HEADER FILE.

For help on ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {} see here: http://www.nongnu.org/avr-libc/user-manual/group__util__atomic.html

Pin Mapping: https://www.arduino.cc/en/Hacking/PinMapping168
*/

/*
===================================================================================================
  LICENSE & DISCLAIMER
  Copyright (C) 2015 Gabriel Staples.  All right reserved.
  
  ------------------------------------------------------------------------------------------------
  License: GNU Lesser General Public License Version 3 (LGPLv3) or later - https://www.gnu.org/licenses/lgpl.html
  ------------------------------------------------------------------------------------------------

  This file is part of eRCaGuy_PPM_Writer.
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/
===================================================================================================
*/

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#include "eRCaGuy_PPM_Writer.h"
#include <util/atomic.h> //http://www.nongnu.org/avr-libc/user-manual/group__util__atomic.html

//macros
#define readPinA() (PINB & _BV(1)) //Arduino pin 9
#define togglePinA() (TCCR1C = _BV(FOC1A)) //see datasheet pg. 135 & 132; force OC1A pin (Arduino D9) to toggle; datasheet pg. 132: setting one or both of the COM1A1:0 bits to 1 overrides the normal port functionality of the I/O pin it is connected to, so this is how you must toggle the pin; digitalWrite(9,HIGH/LOW) on pin 9 will NOT work anymore on this pin; note, however, that *reading* the pin port directly, or calling digitalRead(9) DOES still work!

//Direct Port Access digital Writes: writing the pin HIGH or LOW this way works ONLY when the PPM signal is NOT being output (ie: before PPMWRiter.begin() or after PPMWriter.end())
#define makePinAOUTPUT() (DDRB |= _BV(1)) //Arduino pin 9
#define makePinAINPUT() (DDRB &= ~_BV(1)) //Arduino pin 9
#define writePinAHIGH() (PORTB |= _BV(1)) //Arduino pin 9
#define writePinALOW() (PORTB &= ~_BV(1)) //Arduino pin 9

/* //FOR DEBUGGING/CODE SPEED MEASURING WITH OSCILLOSCOPE
#define writePin2HIGH (PORTD |= _BV(2))
#define writePin2LOW (PORTD &= ~_BV(2)) */

eRCaGuy_PPM_Writer PPMWriter; //preinstantiation of object

//========================================================================================================
//ISRs
//========================================================================================================
//Timer 1 Compare Match A interrupt
ISR(TIMER1_COMPA_vect)
{
  // writePin2HIGH; //FOR MEASURING THE ISR PROCESSING TIME. ANSWER: <= ~6us per ISR interrupt
  PPMWriter.compareMatchISR();
  // writePin2LOW;
}

//Here is where the magic happens (ie: the actual writing of the PPM signal)
void eRCaGuy_PPM_Writer::compareMatchISR()
{
  //local variables
  unsigned int incrementVal; //units of 0.5us
  
  if (_currentState==FIRST_EDGE)
  {
    //local variable 
    byte lastChannel_i = _currentChannel_i - 1;
    if (_currentChannel_i==0)
    {
      lastChannel_i = _numChannels; //ex: for 8 channels, lastChannel_i is now equal to 8. Since the 8 channels take bits 0 to 7 in the _channelFlags variable, bit 8 corresponds to the FrameSpace "channel" which occured after channel 8
      
      //if we are on the FIRST_EDGE *AND* the first channel, then it means we are at the *VERY START* of a new PPM frame, so the last PPM frame has just completed!
      _timeSinceFrameStart = 0; //0.5us; reset
      _frameNumber++;
    }
    //each first edge signifies the END of _currentChannel_i-1 and the START of _currentChannel_i, so let's set the bit to indicate which channel was just written!
    bitSet(_channelFlags,lastChannel_i);
    
    incrementVal = _channelSpace; //0.5us
    _currentState = SECOND_EDGE; //move to next state in the state machine
  }
  else //_currentState==SECOND_EDGE
  {
    if (_currentChannel_i < _numChannels) 
    {
      //we are writing a channel value 
      incrementVal = _channels[_currentChannel_i] - _channelSpace; //0.5us units
      _currentChannel_i++;
    }
    else //_currentChannel_i==_numChannels
    {
      //we are writing a frame space value (ie: the time between the end of one PPM frame and the start of another PPM frame)
      ensurePPMPolarity();
      incrementVal = _pd - _timeSinceFrameStart; //0.5us units 
      //constrain this frame space period; ie: force a minimum value, even at the cost of not reaching the desired PPM output frequency
      incrementVal = max(incrementVal,_minFrameSpace);
      _currentChannel_i = 0; //reset back to the first channel 
    }
    _currentState = FIRST_EDGE; //move to next state in the state machine
  }
  
  //set up NEXT signal toggle on the OC1A pin
  OCR1A += incrementVal; 
  _timeSinceFrameStart += incrementVal; //0.5us; this will be the time elapsed at the start of the NEXT Compare Match A interrupt
}

//========================================================================================================
//eRCaGuy_PPM_Writer CLASS METHODS
//========================================================================================================

//--------------------------------------------------------------------------------------------------------
//define class constructor method
//--------------------------------------------------------------------------------------------------------
eRCaGuy_PPM_Writer::eRCaGuy_PPM_Writer()
{
  //set defaults:
  _numChannels = DEFAULT_NUM_CHANNELS;
  for (byte i=0; i<MAX_NUM_CHANNELS; i++)
  {
    _channels[i] = DEFAULT_CHANNEL_VAL; //0.5us
  }
  _channelFlags = 0;
  _pd = DEFAULT_FRAME_PERIOD; //0.5us
  _maxChannelVal = DEFAULT_MAX_CHANNEL_VAL; //0.5us
  _minChannelVal = DEFAULT_MIN_CHANNEL_VAL; //0.5us
  _minFrameSpace = DEFAULT_MIN_FRAME_SPACE; //0.5us
  _channelSpace = DEFAULT_CHANNEL_SPACE; //0.5us
  _frameNumber = 0;
  _PPMPolarity = PPM_WRITER_NORMAL; 
}

//--------------------------------------------------------------------------------------------------------
//begin()
//-configure Timer1 and begin PPM output
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::begin()
{
  //PPM writer state machine initialization
  _currentState = FIRST_EDGE;
  _currentChannel_i = 0;
  _timeSinceFrameStart = 0; //0.5us units
  
  makePinAOUTPUT();
  
  //configure Timer1 to NORMAL mode (see datasheet Table 16-4, pg. 133)
  //  Mode 0: WGM13=0, WGM12=0, WGM11=0, WGM10=0
  //Toggle OC1A on Compare Match (datasheet Table 16-1, pg. 132)
  //  COM1A1 = 0, COM1A0 = 1
  //Prescaler 8 (0.5us/count) (Table 16-5, pg. 135)
  //  CS12=0, CS11=1, CS10=0
  //TCCR1A pg. 132; TCCR1B pg. 134
  TCCR1A = _BV(COM1A0);
  TCCR1B = _BV(CS11);
  
  OCR1A = TCNT1 + 100; //set to interrupt (to begin PPM signal generation) 50us (100 counts) from now
  
  //enable Output Compare A interrupt (TIMSK1, datasheet pg. 136)
  TIMSK1 = _BV(OCIE1A);  
  
  ensurePPMPolarity();
}

//--------------------------------------------------------------------------------------------------------
//ensurePPMPolarity()
//-private method
//-forces correct PPM signal polarity (ie: steady high, pulsing low, vs steady low, pulsing high)
//--------------------------------------------------------------------------------------------------------
inline void eRCaGuy_PPM_Writer::ensurePPMPolarity()
{
  if (_PPMPolarity==PPM_WRITER_NORMAL)
  {
    //ensure pin is HIGH before start
    if (readPinA()==LOW)
      togglePinA();
  }
  else //_PPMPolarity==PPM_WRITER_INVERTED
  {
    //ensure pin is LOW before start 
    if (readPinA()==HIGH)
      togglePinA();
  }
}

//--------------------------------------------------------------------------------------------------------
//end()
//-ends the PPM output, but otherwise leaves the timer settings as-is
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::end()
{
  //turn off the Output Compare Interrupt (TIMSK1, datasheet pg. 136)
  TIMSK1 &= ~_BV(OCIE1A);  
  //disconnect OC1A (see datasheet pg 132)
  TCCR1A &= ~(_BV(COM1A1) | _BV(COM1A0)); //note: once OC1A is disconnected, the pin state goes back to the last state (HIGH or LOW) written to the port output register via either direct port manipulation, OR the Arduino digitalWrite() command!
  //for pin protection (in case of dual-purposing this pin for some reason), force LOW and make input 
  writePinALOW();
  makePinAINPUT();
}

//--------------------------------------------------------------------------------------------------------
//readChannelFlag
//-NB: channels are ZERO-INDEXED (1st channel is 0, 2nd channel is 1, etc)
//-read a flag to see if this particular channel was just written
//-this operation automatically clears the flag if the flag is true
//-once a flag is set true, it remains true until read by the user. Therefore, if you are reading the flag
// for the first time, you may read a TRUE even if the flag was set a long time ago
//--------------------------------------------------------------------------------------------------------
boolean eRCaGuy_PPM_Writer::readChannelFlag(byte channel_i)
{
  bool flagVal;
  //ensure a valid channel
  channel_i = constrain(channel_i,0,_numChannels - 1);
  
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    flagVal = bitRead(_channelFlags,channel_i);
    if (flagVal==true) //if read true, reset the flag
      bitClear(_channelFlags,channel_i);
  }
  return flagVal;
}

//********************************************************************************************************
//"set" methods
//********************************************************************************************************

//--------------------------------------------------------------------------------------------------------
//setChannelVal
//-write 0.5us value to the channel located at channel index channel_i. 
//-Channels are zero-indexed, so Channel 1 is index 0, channel 2 is index 1, etc.
//-channel values are in units of timer counts, which are 0.5us each, so multiply us x 2 to get counts.
//-Ex: to set the 1st channel to 1500us, call: setChannelVal(0,1500*2);
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setChannelVal(byte channel_i, unsigned int val)
{
  //constrain within valid bounds
  val = constrain(val,_minChannelVal,_maxChannelVal);
  channel_i = constrain(channel_i,0,_numChannels - 1);
  
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    _channels[channel_i] = val;
  }
}

//--------------------------------------------------------------------------------------------------------
//setPPMPeriod
//-control the PPM output frequency by setting the desired PPM train frame period directly 
//-units are 0.5us
//-ex: setPPMPeriod(20000*2) sets a pd of 20000us (40000 0.5us counts), which results in a PPM freq. of 1/20ms=50Hz
//-WARNING: if your channel values are all at their max values, the local variable "_minFrameSpace" acts as a safety feature to ensure the frame space after the last channel is longer than the longest channel. This is necessary because that is how a device *reading* a PPM signal finds the first frame: it knows that the *longest* frame is the frame space, and that the first channel comes after that. In the event that you set the PPM period too short, or have too many channels, with all of them maxed out, the PPM writer will force the frame space after the last channel to be at least as long as what is set in the local variable "_minFrameSpace." In this event, the PPM period will be lengthened for that frame, and the desired PPM frequency will not be reached.
//--ex: assume 9 channels, max channel value of 2100us, PPM period set to 20ms; if all channels are simultaneously maxed out it takes 2100x9 = 18.9ms just to write the channels. To keep the 20ms PPM period, the frameSpace would have to be 20-18.9 = 1.1ms. However, THIS WILL BREAK THE SIGNAL AND PREVENT THE DEVICE READING THE PPM SIGNAL FROM DETERMINING WHERE THE START OF THE PPM FRAME IS. Therefore, the _minFrameSpace (default 3ms last I set it) will be used instead of 1.1ms. This makes the PPM frame 18.9ms + 3ms = 21.9ms (45.7Hz) instead of the desired 20ms (50Hz). Depending on your settings, since this PPM writer is totally user-customizable, you may get even more drastic results. Ex: if you were trying to fit 12 full-length channels in a 20ms PPM frame....you do the math.
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setPPMPeriod(unsigned long pd)
{
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    _pd = pd;
  }
}

//--------------------------------------------------------------------------------------------------------
//setNumChannels
//-set the number of channels to be present in the PPM train 
//-NB: READ THE WARNING AND EXAMPLE IN THE "setPPMPeriod" DESCRIPTION ABOVE.
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setNumChannels(byte numChannels)
{
  numChannels = constrain(numChannels,MIN_NUM_CHANNELS,MAX_NUM_CHANNELS);
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    _numChannels = numChannels;
  }
}

//--------------------------------------------------------------------------------------------------------
//setMaxChannelVal
//-set the maximum value a channel can be set to, in units of 0.5us 
//-Ex: to set the the maximum channel value to 2100us, call: setMaxChannelVal(2100*2);
//-NB: READ THE WARNING AND EXAMPLE IN THE "setPPMPeriod" DESCRIPTION ABOVE.
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setMaxChannelVal(unsigned int maxVal)
{
  _maxChannelVal = maxVal;
}

//--------------------------------------------------------------------------------------------------------
//setMinChannelVal
//-set the minimum value a channel can be set to, in units of 0.5us 
//-Ex: to set the the minimum channel value to 900us, call: setMinChannelVal(900*2);
//-NB: READ THE WARNING AND EXAMPLE IN THE "setPPMPeriod" DESCRIPTION ABOVE.
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setMinChannelVal(unsigned int minVal)
{
  _minChannelVal = minVal;
}

//--------------------------------------------------------------------------------------------------------
//setMinFrameSpace
//-set the minimum permitted frame space, in units of 0.5us
//-the frame space is the gap of time AFTER the last channel in the PPM train, and BEFORE the first channel in the PPM train. If it is not longer than _maxChannelVal, it just looks like another channel.
//-WARNING: TO PREVENT YOUR RC VEHICLE FROM CRASHING WHEN ALL CHANNEL VALUES ARE MAXED OUT, YOU *MUST* SET THIS VALUE TO BE *GREATER* THAN WHAT YOU SET _maxChannelVal to be. How much greater you must set it....that's dependent upon how the code in the device *reading* this PPM signal works. I think 3ms should be safe.
//-NB: READ THE WARNING AND EXAMPLE IN THE "setPPMPeriod" DESCRIPTION ABOVE.
//-Ex: to set the the minFrameSpace to 3000us, call: setMinFrameSpace(3000*2);
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setMinFrameSpace(unsigned int minFrameSpace)
{
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    _minFrameSpace = minFrameSpace;
  }
}

//--------------------------------------------------------------------------------------------------------
//setChannelSpace
//-set the pulse width of the spacer, in units of 0.5us, which separates channels in the PPM train. This width can vary slightly from brand to brand.
//-Ex: to set the the channelSpace to 400us, call: setChannelSpace(400*2);
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setChannelSpace(unsigned int channelSpace)
{
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    _channelSpace = channelSpace;
  }
}

//--------------------------------------------------------------------------------------------------------
//setFrameNumber
//-manually set the frameNumber to a specific value; ex: 0, to reset it 
//-the frameNumber is the # of PPM frames that have *already been sent*
//-it is incremented at the first edge of a new PPM frame, which marks the end of the previous PPM frame and the start of the 1st channel of the new PPM frame 
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setFrameNumber(unsigned long frameNum)
{
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    _frameNumber = frameNum;
  }
}

//--------------------------------------------------------------------------------------------------------
//setPPMPolarity
//-set the PPM signal train to either PPM_WRITER_NORMAL or PPM_WRITER_INVERTED.
//-PPM_WRITER_NORMAL has a HIGH base-line signal, with channelSpace pulses that are LOW 
//-PPM_WRITER_INVERTED has a LOW base-line signal, with channelSpace pulses that are HIGH  
//-to set the polarity to be inverted, for instance, call: setPPMPolarity(PPM_WRITER_INVERTED);
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setPPMPolarity(boolean PPMPolarity)
{
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    _PPMPolarity = PPMPolarity;
  }
}

//********************************************************************************************************
//"get" methods
//********************************************************************************************************

//--------------------------------------------------------------------------------------------------------
//getChannelVal
//-read the 0.5us value currently being written on this channel index location
//-channels are zero-indexed, so the 1st channel is at index 0
//-the channelVal is in units of timer counts, which are 0.5us each, so divide by 2 to get us 
//--Ex: to get the us value being written to channel 1, call: getChannelVal(0)/2;
//--------------------------------------------------------------------------------------------------------
unsigned int eRCaGuy_PPM_Writer::getChannelVal(byte channel_i)
{
  //Note: forced atomic access NOT required on _numChannels and _channels below since they are only being READ here, AND since they are NOT ever written to (modified) in an ISR 
  channel_i = constrain(channel_i,0,_numChannels - 1);   
  return _channels[channel_i];
}

//--------------------------------------------------------------------------------------------------------
//getPPMPeriod
//-read the previously-set desired PPM period, in units of 0.5us.
//--Ex: to get the desired PPM period in us, call: getPPMPeriod()/2;
//--------------------------------------------------------------------------------------------------------
unsigned long eRCaGuy_PPM_Writer::getPPMPeriod()
{
  return _pd;
}

//--------------------------------------------------------------------------------------------------------
//getPPMFrequency()
//-calculate and return the desired PPM frame frequency (in Hz), according to the previously commanded PPM train period (in units of 0.5us)
//--------------------------------------------------------------------------------------------------------
float eRCaGuy_PPM_Writer::getPPMFrequency()
{
  return 1000000.0/((float)_pd/2.0); //Hz
}

//--------------------------------------------------------------------------------------------------------
//getNumChannels()
//-return the # of channels currently being sent out in the PPM train
//--------------------------------------------------------------------------------------------------------
byte eRCaGuy_PPM_Writer::getNumChannels()
{
  return _numChannels;
}

//--------------------------------------------------------------------------------------------------------
//getMaxChannelVal()
//-get the max value a channel can be set to, in units of 0.5us 
//--Ex: to get the max channel value allowed, in us, call: getMaxChannelVal()/2; 
//--------------------------------------------------------------------------------------------------------
unsigned int eRCaGuy_PPM_Writer::getMaxChannelVal()
{
  return _maxChannelVal;
}

//--------------------------------------------------------------------------------------------------------
//getMinChannelVal()
//-get the min value a channel can be set to, in units of 0.5us 
//--Ex: to get the min channel value allowed, in us, call: getMinChannelVal()/2; 
//--------------------------------------------------------------------------------------------------------
unsigned int eRCaGuy_PPM_Writer::getMinChannelVal()
{
  return _minChannelVal;
}

//--------------------------------------------------------------------------------------------------------
//getMinFrameSpace()
//-units of 0.5us
//-get the min frame space you are permitting after the last channel in the PPM train, and before the first channel of the next frame in the PPM train 
//-FOR MORE INFO SEE THE DESCRIPTION FOR "setMinFrameSpace"
//--Ex: to get the minFrameSpace allowed, in us, call: getMinFrameSpace()/2; 
//--------------------------------------------------------------------------------------------------------
unsigned int eRCaGuy_PPM_Writer::getMinFrameSpace()
{
  return _minFrameSpace;
}

//--------------------------------------------------------------------------------------------------------
//getChannelSpace
//-get the pulse width of the spacer, in units of 0.5us, which separates channels in the PPM train. This width can vary slightly from brand to brand.
//-Ex: to get the the channelSpace, in us, call: getChannelSpace()/2;
//--------------------------------------------------------------------------------------------------------
unsigned int eRCaGuy_PPM_Writer::getChannelSpace()
{
  return _channelSpace;
}

//--------------------------------------------------------------------------------------------------------
//getFrameNumber 
//-the frameNumber is the # of PPM frames that have *already been sent*
//-it is incremented at the first edge of a new PPM frame, which marks the end of the previous PPM frame and the start of the 1st channel of the new PPM frame 
//--------------------------------------------------------------------------------------------------------
unsigned long eRCaGuy_PPM_Writer::getFrameNumber()
{
  unsigned long frameNumber;
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    frameNumber = _frameNumber;
  }
  return frameNumber;
}

//--------------------------------------------------------------------------------------------------------
//getPPMPolarity
//-read whether the PPMPolarity is set to PPM_WRITER_NORMAL (0) or PPM_WRITER_INVERTED (1).
//-PPM_WRITER_NORMAL has a HIGH base-line signal, with channelSpace pulses that are LOW 
//-PPM_WRITER_INVERTED has a LOW base-line signal, with channelSpace pulses that are HIGH  
//--------------------------------------------------------------------------------------------------------
boolean eRCaGuy_PPM_Writer::getPPMPolarity()
{
  return _PPMPolarity;
}

//--------------------------------------------------------------------------------------------------------
//getCurrentChannel
//-returns the PPM channel index for the channel which is *in the process of being* or *about to be* written to the PPM signal train! 
//--------------------------------------------------------------------------------------------------------
byte eRCaGuy_PPM_Writer::getCurrentChannel()
{
  return _currentChannel_i; //no need to force this operation to be atomic since it is a single byte; single byte variables automatically have atomic read/write access 
}

//--------------------------------------------------------------------------------------------------------
//getTimeSincePPMFrameStart
//-units of 0.5us 
//-returns the time, in 0.5us counts, which will be the time elapsed at the start of the NEXT Compare Match interrupt, since the beginning of this PPM Frame
//--------------------------------------------------------------------------------------------------------
unsigned long eRCaGuy_PPM_Writer::getTimeSincePPMFrameStart()
{
  unsigned long timeSinceFrameStart; //0.5us
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    timeSinceFrameStart = _timeSinceFrameStart;
  }
  return timeSinceFrameStart;
}




