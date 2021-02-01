/*
eRCaGuy_TimerCounter Library
By Gabriel Staples
Website: http://www.ElectricRCAircraftGuy.com
My contact info is available by clicking the "Contact Me" tab at the top of my website.
**************************************************
* SEE eRCaGuy_TimerCounter.h FILE FOR MORE INFO.
**************************************************
*/

/*
===================================================================================================
LICENSE & DISCLAIMER
Copyright (C) 2013-2015 Gabriel Staples.  All right reserved.

This file is part of eRCaGuy_TimerCounter.

I AM WILLING TO DUAL-LICENSE THIS SOFTWARE. HOWEVER, UNLESS YOU HAVE PAID FOR AND RECEIVED A
RECEIPT FOR AN ALTERNATE LICENSE AGREEMENT, FROM ME, THE COPYRIGHT OWNER, THIS SOFTWARE IS LICENSED
AS FOLLOWS:

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

#ifndef eRCaGuy_TimerCounterTimers_h
#define eRCaGuy_TimerCounterTimers_h

//----------------INSTRUCTIONS TO USER------------------
// CHOOSE YOUR TIMER YOU WANT TO USE FOR THIS LIBRARY.
// SIMPLY UNCOMMENT THE ONE YOU WANT TO USE, AND COMMENT
// OUT THE ONE CURRENTLY SET TO BE USED.
//------------------------------------------------------

// Note to self: TC stands for "TimerCounter" library

// For the Arduino Uno, Nano, Pro Mini, etc (Atmega328/168-based)
//-Timer0, 1, and 2 may also work with the Mega2560; not tested at this time
//-Timer0 & 1 may also work the Leonardo (ATmega32u4); not tested at this time

// #define TC_USE_TIMER0 //8-bit, normally controls micros(), millis(), etc <--use if you don't
// want any conflict with the servo library, and you are going to use eRCaGuy_TimerCounter in
// place of micros() and millis()
#define TC_USE_TIMER1  // 16-bit, used by the servo library, etc  <--use as the default for
                       // eRCaGuy_TimerCounter
// #define TC_USE_TIMER2 //8-bit, used by some libraries, including the core Arduino tone() function
// <--use if you don't want any conflict with the servo library, and you still want to have access
// to micros() and millis()

//-------------------------------------------------------
// NOTHING FOR USER TO CHANGE BELOW THIS POINT
//-------------------------------------------------------

// Necessary defines

#if defined(TC_USE_TIMER0)
    #define OVERFLOW_VECTOR TIMER0_OVF_vect
    #define TC_IS_8BIT_TIMER
    #define TCCRnA TCCR0A
    #define TCCRnB TCCR0B
    #define TCNTn TCNT0
    #define TIFRn TIFR0
    #define TIMSKn TIMSK0
#elif defined(TC_USE_TIMER1)
    #define OVERFLOW_VECTOR TIMER1_OVF_vect
    #define TC_IS_16BIT_TIMER
    #define TCCRnA TCCR1A
    #define TCCRnB TCCR1B
    #define TCNTn TCNT1
    #define TIFRn TIFR1
    #define TIMSKn TIMSK1
#elif defined(TC_USE_TIMER2)
    #define OVERFLOW_VECTOR TIMER2_OVF_vect
    #define TC_IS_8BIT_TIMER
    #define TCCRnA TCCR2A
    #define TCCRnB TCCR2B
    #define TCNTn TCNT2
    #define TIFRn TIFR2
    #define TIMSKn TIMSK2
#endif

#if defined(TC_IS_8BIT_TIMER)
    #define TC_RESOLUTION (256)
#elif defined(TC_IS_16BIT_TIMER)
    #define TC_RESOLUTION (65536)
#endif

// FOR SPEED PROFILING WITH OSCILLOSCOPE
//-Don't forget that a direct-port-access write like this takes 2 clock cycles, or 0.125 us, so
// you
// have to subtract 2 clock cycles (NOT 4) from whatever time period you profile with the
// oscilloscope.
#define PROFILE_PIN2_HIGH PORTD |= _BV(2)  // write Arduino pin D2 HIGH
#define PROFILE_PIN2_LOW PORTD &= ~_BV(2)  // write Arduino pin D2 LOW

/*==========================================================================================
Unused code at this time:
#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) ||
defined(__AVR_ATmega168P__) #endif
===========================================================================================*/

#endif
