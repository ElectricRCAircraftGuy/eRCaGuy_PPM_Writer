/*
eRCaGuy_PPM_Writer
-an >=11-bit, jitter-free (hardware-based) RC PPM signal writer for the Arduino!
By Gabriel Staples
http://www.ElectricRCAircraftGuy.com 
Written: 2 July 2015
Last Updated: 2 July 2015

Version 0.1
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

#ifndef eRCaGuy_PPM_Writer_h
#define eRCaGuy_PPM_Writer_h

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

//Constants
const byte MAX_NUM_CHANNELS = 16;
const byte MIN_NUM_CHANNELS = 4; 
const byte DEFAULT_NUM_CHANNELS = 8;
const unsigned int DEFAULT_MAX_CHANNEL_VAL = 2100*2; //0.5us units
const unsigned int DEFAULT_MIN_CHANNEL_VAL = 900*2; //0.5us
const unsigned int DEFAULT_CHANNEL_VAL = 1500*2; //0.5us
const unsigned int DEFAULT_MIN_FRAME_SPACE = max(DEFAULT_MAX_CHANNEL_VAL + 100*2,3000*2); //0.5us; min time gap between each frame, ie: between the end of the last channel and the start of the first channel
const unsigned int DEFAULT_CHANNEL_SPACE = 400*2; //0.5us; the pulse that signifies the start of each new channel
const unsigned long DEFAULT_FRAME_PERIOD = 22000*2; //0.5us; note: the default period for the Spektrum DX8, for instance, is 22ms, or a freq. of 45.45Hz
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
  void setChannelValue(byte channel,unsigned int channelValue); //write the 0.5us value to channel (ie: write your desired microsecond value x 2); NB: CHANNELS ARE ZERO-INDEXED. ie: the 1st channel is 0, 2nd channel is 1, etc.
  void setPeriod(unsigned int pd); //0.5us
  //secondary methods
  void setNumChannels(byte numChannels);
  void setMaxChannelVal(unsigned int maxVal); //0.5us
  void setMinChannelVal(unsigned int minVal); //0.5us
  void setMinFrameSpace(unsigned int minFrameSpace); //0.5us
  void setChannelSpace(unsigned int channelSpace); //0.5us
  void setFrameNumber(unsigned long frameNum); //manually set the frameNumber to a specific value; ex: 0, to reset it
  void setPPMPolarity(boolean polarity);
  
  //"get" methods 
  //primary methods
  unsigned int getChannelValue(byte channel); //0.5us
  unsigned int getPeriod(); //0.5us
  //secondary methods
  byte getNumChannels();
  unsigned int getMaxChannelVal(); //0.5us
  unsigned int getMinChannelVal(); //0.5us
  unsigned int getMinFrameSpace(); //0.5us
  unsigned int getChannelSpace(); //0.5us
  unsigned long getFrameNumber();
  boolean getPPMPolarity();
  byte getCurrentChannel(); //returns the PPM channel which is *in the process of being* or *about to be* written!
  unsigned long getTimeSinceFrameStart(); //0.5us; returns the time, in 0.5us counts, which will be the time elapsed at the start of the NEXT Compare Match A interrupt, since the beginning of this PPM Frame
  
  //other methods
  void begin(); //configures the timer and begins PPM output 
  void end(); //ends PPM output, but otherwise leaves the timer settings as-is
  boolean readChannelFlag(byte channel); //each channel has a flag (single bit) that is set true when the channel value is completed being written, and the flag is cleared once a true value is read via this function; this is useful if you want to have some sort of event-driven programming that does something immediately after a specific channel value is written; ex: "while(readChannelFlag(channel)==false){};" will stay in the while loop until that channel value is written to the PPM signal
  void compareMatchISR(); //interrupt service routine for output compare matches
  
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
  //non-volatile:
  unsigned int _maxChannelVal; //0.5us
  unsigned int _minChannelVal; //0.5us
  
  //PPM writer state machine variables
  enum state_t {FIRST_EDGE,SECOND_EDGE};
  state_t _currentState;
  //volatile variables (since they are changed in ISR, but may be read in the main code)
  volatile byte _currentChannel; //zero-based channel # (ie: channel index) which is the PPM channel which is *in the process of being* or *about to be* written!
  volatile unsigned long _timeSinceFrameStart; //0.5us units
};

//external instance of this class, instantiated in .cpp file
extern eRCaGuy_PPM_Writer PPMWriter;

#endif