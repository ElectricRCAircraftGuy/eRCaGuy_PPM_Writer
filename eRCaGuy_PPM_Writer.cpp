/*
eRCaGuy_PPM_Writer
By Gabriel Staples
http://www.ElectricRCAircraftGuy.com 

FOR PERTINENT INFORMATION ABOUT THIS LIBRARY SEE THE TOP OF THE HEADER (.h) FILE, AND THE COMMENTS IN BOTH THIS FILE AND THE HEADER FILE.

For help on   
ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {} see here: http://www.nongnu.org/avr-libc/user-manual/group__util__atomic.html
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
    byte lastChannel = _currentChannel - 1;
    if (_currentChannel==0)
    {
      lastChannel = _numChannels; //ex: for 8 channels, lastChannel is now equal to 8. Since the 8 channels take bits 0 to 7 in the _channelFlags variable, bit 8 corresponds to the FrameSpace "channel" which occured after channel 8
      
      //if we are on the FIRST_EDGE *AND* the first channel, then it means we are at the *VERY START* of a new PPM frame, so the last PPM frame has just completed!
      _timeSinceFrameStart = 0; //0.5us; reset
      _frameNumber++;
    }
    //each first edge signifies the END of _currentChannel-1 and the START of _currentChannel, so let's set the bit to indicate which channel was just written!
    bitSet(_channelFlags,lastChannel);
    
    incrementVal = _channelSpace; //0.5us
    _currentState = SECOND_EDGE; //move to next state in the state machine
  }
  else //_currentState==SECOND_EDGE
  {
    if (_currentChannel < _numChannels) 
    {
      //we are writing a channel value 
      incrementVal = _channels[_currentChannel] - _channelSpace; //0.5us units
      _currentChannel++;
    }
    else //_currentChannel==_numChannels
    {
      //we are writing a frame space value (ie: the time between the end of one PPM frame and the start of another PPM frame)
      ensurePPMPolarity();
      incrementVal = _pd - _timeSinceFrameStart; //0.5us units 
      //constrain this frame space period; ie: force a minimum value, even at the cost of not reaching the desired PPM output frequency
      incrementVal = max(incrementVal,_minFrameSpace);
      _currentChannel = 0; //reset back to the first channel 
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
  _currentChannel = 0;
  _timeSinceFrameStart = 0; //0.5us units
  
  //configure Timer1 to NORMAL mode (see datasheet Table 16-4, pg. 133)
  //  Mode 0: WGM13=0, WGM12=0, WGM11=0, WGM10=0
  //Toggle OC1A on Compare Match (datasheet Table 16-1, pg. 132)
  //  COM1A1 = 0, COM1A0 = 1
  //Prescaler 8 (0.5us/count) (Table 16-5, pg. 135)
  //  CS12=0, CS11=1, CS10=0
  //TCCR1A pg. 132; TCCR1B pg. 134
  TCCR1A = _BV(COM1A0);
  TCCR1B = _BV(CS11);
  
  //Note: OC1A pin is Arduino D9; see here: https://www.arduino.cc/en/Hacking/PinMapping168
  pinMode(9,OUTPUT);
  
  TCNT1 = 0; //reset Timer1 counter
  OCR1A = TCNT1 + 100; //set to interrupt (to begin PPM signal generation) 50us later
  
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
  TCCR1A &= ~(_BV(COM1A1) | _BV(COM1A0)); //note: it appears that this also forces the pin state to LOW, no matter how it was before disconnecting 0C1A; this is good
}

//--------------------------------------------------------------------------------------------------------
//readChannelFlag
//-NB: channels are ZERO-INDEXED (1st channel is 0, 2nd channel is 1, etc)
//-read a flag to see if this particular channel was just written
//-this operation automatically clears the flag if the flag is true
//-once a flag is set true, it remains true until read by the user. Therefore, if you are reading the flag
// for the first time, you may read a TRUE even if the flag was set a long time ago
//--------------------------------------------------------------------------------------------------------
boolean eRCaGuy_PPM_Writer::readChannelFlag(byte channel)
{
  bool flagVal;
  //ensure a valid channel
  channel = constrain(channel,0,_numChannels - 1);
  
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    flagVal = bitRead(_channelFlags,channel);
    if (flagVal==true) //if read true, reset the flag
      bitClear(_channelFlags,channel);
  }
  return flagVal;
}

//********************************************************************************************************
//"set" methods
//********************************************************************************************************

//--------------------------------------------------------------------------------------------------------
//setChannelValue 
//-write 0.5us value to the channel. 
//-Ex: to set the 1st channel to 1500us, call: setChannelValue(0,1500*2);
//--------------------------------------------------------------------------------------------------------
void eRCaGuy_PPM_Writer::setChannelValue(byte channel_i, unsigned int val)
{
  //constrain within valid bounds
  val = constrain(val,_minChannelVal,_maxChannelVal);
  //ensure atomic access to volatile variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    channel_i = constrain(channel_i,0,_numChannels - 1);
    _channels[channel_i] = val;
  }
}




//********************************************************************************************************
//"get" methods
//********************************************************************************************************





//--------------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------------

