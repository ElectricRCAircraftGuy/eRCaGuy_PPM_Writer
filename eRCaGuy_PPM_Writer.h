/*
eRCaGuy_PPM_Writer
-an >=11-bit, jitter-free (hardware-based) RC PPM signal writer for the Arduino!

AUTHOR:
By Gabriel Staples
Website: http://www.ElectricRCAircraftGuy.com
My contact info is available by clicking the "Contact Me" tab at the top of my website.
Library Written: 2 July 2015
Library Last Updated: 12 April 2016

VERSIONING SYSTEM: 
-Using Semantic Versioning 2.0.0 (http://semver.org/)

Current Library Version 0.2.0

HISTORY (newest on top):
20160412 - Version 0.2.1 - User-error-related bug fixed! Previously, in functions such as setChannelVal(channel_i, val), I would *constrain* the user input for the channel index, channel_i, and set the val on the constrained channel_i. This is a BAD idea! For example, if the user tries to set Ch8 to something, but they have set only 7 channels via the setNumChannels function, previously it would set their commanded value (that they intended for Ch8) on Ch7 instead! Bad bad bad! Very confusing to the user. Instead, if the user tries to act on a channel that doesn't exist, simply ignore their command. This way, they are more likely to find their mistake, rather than reading and writing from and to channels they don't know they are reading and writing from and to. Bug fixed. No more of this silliness. :)
20160326 - Version 0.2.0 released; major additions to add in a 0.5us-resolution timestamp capability as a replacement for the micros() function which only has a 4us resolution; also added a few other things
 -functions added include:
 --getCount()
 --getMicros()
 --overflowInterruptOff();
 --overflowInterruptOn();
 --attachOverflowInterrupt();
 --detachOverflowInterrupt();
20150705 - First working version, V0.1.0, released
*/

/*
===================================================================================================
  LICENSE & DISCLAIMER
  Copyright (C) 2015 Gabriel Staples.  All right reserved.
  
  This file is part of eRCaGuy_PPM_Writer.
  
  I AM WILLING TO DUAL-LICENSE THIS SOFTWARE--EX: BY SELLING YOU A SEPARATE COMMERICAL LICENSE FOR
  PRORPRIETARY USE. HOWEVER, UNLESS YOU HAVE PAID FOR AND RECEIVED A RECEIPT FOR AN ALTERNATE 
  LICENSE AGREEMENT, FROM ME, THE COPYRIGHT OWNER, THIS SOFTWARE IS LICENSED UNDER THE GNU GPLV3
  OR LATER, A COPY-LEFT LICENSE, AS FOLLOWS.
  
  NB: THE GNU GPLV3 LICENSE IS AN OPEN SOURCE LICENSE WHICH REQUIRES THAT ALL DERIVATIVE WORKS 
  YOU CREATE (IE: *ANY AND ALL* CODE YOU HAVE LINKING TO, BORROWING FROM, OR OTHERWISE USING THIS CODE) 
  ALSO BE RELEASED UNDER THE SAME LICENSE, AND BE OPEN-SOURCE FOR YOUR USERS AND/OR CUSTOMERS.
  FOR ALL STIPULATIONS AND LEGAL DETAILS, REFER TO THE FULL LICENSE AGREEMENT.
  
  ------------------------------------------------------------------------------------------------
  License: GNU General Public License Version 3 (GPLv3) - https://www.gnu.org/licenses/gpl.html
  ------------------------------------------------------------------------------------------------
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/
===================================================================================================
*/

#ifndef eRCaGuy_PPM_Writer_h
#define eRCaGuy_PPM_Writer_h

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

//Constants
const byte MAX_NUM_CHANNELS = 16;
const byte MIN_NUM_CHANNELS = 1; 
const byte DEFAULT_NUM_CHANNELS = 8;
const unsigned int DEFAULT_MAX_CHANNEL_VAL = 2100*2; //0.5us units
const unsigned int DEFAULT_MIN_CHANNEL_VAL = 900*2; //0.5us
const unsigned int DEFAULT_CHANNEL_VAL = 1500*2; //0.5us
const unsigned int DEFAULT_MIN_FRAME_SPACE = max(DEFAULT_MAX_CHANNEL_VAL + 100*2,3000*2); //0.5us; min time gap between each frame, ie: between the end of the last channel and the start of the first channel
const unsigned int DEFAULT_CHANNEL_SPACE = 400*2; //0.5us; the short pulse whose first edge signifies the start of each new channel
const unsigned long DEFAULT_FRAME_PERIOD = 22000UL*2UL; //0.5us; note: the default period for the Spektrum DX8, for instance, is 22ms, or a freq. of 45.45Hz
// const unsigned int MAX_OUTPUT_COMPARE_INCREMENT = 2^16; //make 2^16, or 65536, since Timer1 is a 16-bit timer/counter
const boolean PPM_WRITER_NORMAL = 0; //base-line HIGH, channelSpace pulses are LOW
const boolean PPM_WRITER_INVERTED = 1; //base-line LOW, channelSpace pulses are HIGH

class eRCaGuy_PPM_Writer
{
  public:
  //class constructor
  eRCaGuy_PPM_Writer();
  
  //"set" methods 
  //primary methods
  void setChannelVal(byte channel_i,unsigned int channelValue); //write the 0.5us value to channel index channel_i (ie: write your desired microsecond value x 2); NB: CHANNELS ARE ZERO-INDEXED. ie: the 1st channel is 0, 2nd channel is 1, etc. Also, the channelValues are in timer counts, which are 0.5us per count. Therefore, a value of 1500us is 3000 0.5us counts.
  void setPPMPeriod(unsigned long pd); //0.5us
  //secondary methods
  void setNumChannels(byte numChannels);
  void setMaxChannelVal(unsigned int maxVal); //0.5us
  void setMinChannelVal(unsigned int minVal); //0.5us
  void setMinFrameSpace(unsigned int minFrameSpace); //0.5us
  void setChannelSpace(unsigned int channelSpace); //0.5us
  void setFrameNumber(unsigned long frameNum); //manually set the frameNumber to a specific value; ex: 0, to reset it
  void setPPMPolarity(boolean polarity); //set PPM_WRITER_NORMAL or PPM_WRITER_INVERTED
  
  //"get" methods 
  //primary methods
  unsigned int getChannelVal(byte channel_i); //0.5us
  unsigned long getPPMPeriod(); //0.5us
  float getPPMFrequency(); //Hz
  //secondary methods
  byte getNumChannels();
  unsigned int getMaxChannelVal(); //0.5us
  unsigned int getMinChannelVal(); //0.5us
  unsigned int getMinFrameSpace(); //0.5us
  unsigned int getChannelSpace(); //0.5us
  unsigned long getFrameNumber();
  boolean getPPMPolarity();
  byte getCurrentChannel(); //returns the PPM channel index for the channel which is *in the process of being* or *about to be* written!
  unsigned long getTimeSincePPMFrameStart(); //0.5us; returns the time, in 0.5us counts, which will be the time elapsed at the start of the NEXT Compare Match A interrupt, since the beginning of this PPM Frame
  
  //other methods
  void begin(); //configures the timer and begins PPM output 
  void end(); //ends PPM output, but otherwise leaves the timer settings as-is
  boolean readChannelFlag(byte channel_i); //each channel has a flag (single bit) that is set true when the channel value is completed being written, and the flag is cleared once a true value is read via this function; this is useful if you want to have some sort of event-driven programming that does something immediately after a specific channel value is written; ex: "while(readChannelFlag(channel_i)==false){};" will stay in the while loop until that channel value is written to the PPM signal
  
  //ISRs
  inline void compareMatchISR(); //interrupt service routine for output compare matches
  inline void overflowISR(); //ISR called when timer overflows; for 0.5us timestamps 
  
  //for 0.5us timestamps:
  unsigned long getCount();
  float getMicros();
  void overflowInterruptOff();
  void overflowInterruptOn();
  //for custom userOverflowFunction
  void attachOverflowInterrupt(void (*myFunc)());
  void detachOverflowInterrupt();  
  
  private:
  
  //private methods (function)
  inline void ensurePPMPolarity();
  
  //volatile variables (that are either written in ISR and read in main code, or which may be written in main code and read in ISR)
  volatile unsigned int _channels[MAX_NUM_CHANNELS]; //0.5us values for each channel
  volatile unsigned long _channelFlags; //32 total flags; the flags are go from right to left, are zero-indexed, and correspond to the channel just written. The flag one place left of the highest channel is for the FrameSpace after the last channel. Ex: for 8 channels the flags would be like this, as bits inside the variable: [FrameSpace Ch7 Ch6 Ch5 Ch4 Ch3 Ch2 Ch1 Ch0]
  volatile unsigned long _pd; //0.5us; the frame period for writing all channels to the PPM signal 
  volatile byte _numChannels; 
  volatile unsigned int _minFrameSpace; //0.5us 
  volatile unsigned int _channelSpace; //0.5us 
  volatile unsigned long _frameNumber; 
  volatile boolean _PPMPolarity;
  //for 0.5us timestamps 
  volatile unsigned long _overflowCount; //a counter of how many times an overflow has occurred on the timer/counter   
  //for a custom userOverflowFunction
  void (* volatile _p_userOverflowFunction)(void); //custom user function to attach to overflow ISR
    //this is a volatile pointer to a function returning a type void, and accepting void as an input parameter; very confusing, I know. See here for help & details: 1) http://stackoverflow.com/questions/31898819/volatile-pointer-to-function-showing-compile-error-when-using-without-typedef-n and 2) http://www.barrgroup.com/Embedded-Systems/How-To/C-Volatile-Keyword
  volatile bool _userOverflowFuncOn; 
  //non-volatile:
  unsigned int _maxChannelVal; //0.5us
  unsigned int _minChannelVal; //0.5us
  
  //PPM writer state machine variables
  enum state_t {FIRST_EDGE,SECOND_EDGE};
  state_t _currentState;
  //volatile variables (since they are changed in ISR, but may be read in the main code)
  volatile byte _currentChannel_i; //zero-based channel # (ie: channel index) which is the PPM channel which is *in the process of being* or *about to be* written!
  volatile unsigned long _timeSinceFrameStart; //0.5us units
};

//external instance of this class, instantiated in .cpp file
extern eRCaGuy_PPM_Writer PPMWriter;

#endif