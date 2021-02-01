Readme Written: 2 July 2015  
Readme Last Updated: 16 July 2017

*NOTE: this code is essentially complete. It is not only partially done. It does not only sort-of work. This is one of my most professionally-written open source codes to date, and it works very well. Feel free to use it. Note the license used below.*

## Code License  
```
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
```

## Instructions  
Connect the PPM output signal (currently on Arduino pin D9) to the trainer port input on an RC Tx, through a 1k\~10k resistor for protection to the Arduino and Radio.  

## Basic Summary  
This is a very thorough and versatile RC PPM generator program to generate a Pulse Position Modulation signal for RC use. Since it utilizes sophisticated hardware regiser techniques, the output PPM signal is high-fidelity and jitter-free. 

## Description  
 * This is a C++ class-based implementation of a low-level driver/communication protocol used widely in commercial Radio Control (RC) equipment, known as Pulse Position Modulation (PPM). PPM is basically just a multiplexed version of a bunch of servo PWM (Pulse Width Modulation) signals all on one line, with special timing pulses between channels to signify where one ends and another begins. If you don't know, servo PWM signals are semi-analog-like signals where the width of the pulse, in microseconds, *is* the data, or servo command. A servo PWM pulse width of ~1000us corresponds to full servo deflection in one direction, while ~2000us corresponds to full servo deflection in the other direction, and ~1500us is the servo center position. In PPM, the pulse width concept is the same, but how it is measured and pulsed out is slightly different. Refer to the "PPM Signal Drawing" image I've included in the repository for a real example I drew from analysing commercial RC equipment on an oscilloscope. 
 * Note that my implementation here is a very sophisticated *hardware*-based approach using the 16-bit Timer1. I am *not* changing signal levels via software, but rather, I'm looking ahead to the next signal transition before we get there and I'm setting it in hardware registers to occur *automatically* without CPU intervention. Then, after each new output signal transition occurs, I interrupt the CPU at an Output Compare Register event to set the next automatic transition to occur in this fashion. In this way, the output truly is "jitter-free". 
 * Additionally, I've chosen a special timer/counter mode ("Normal" mode in this case) which allows the potential for dual-output PPM signals from a single timer/counter. Had I chosen "Clear Timer on Compare match" (CTC) mode (the other logical option to "Immediately" update OCR1x, as required), which is slightly easier to implement, dual output PPM would never be possible from a single timer/counter. 
 * FUTUTURE WORK: The 2nd PPM output on this timer could be implemented in future versions. With some additional work, and if some jitter is allowed, I could even account for overflows in the two 8-bit timer/counters, and have a total of 6 PPM outputs simultaneously from a single ATmega328 microcontroller. Not bad for a $2 8-bit microcontroller. :)

**For more information on this code see here:**  
(no link yet)  


I hope you find this useful.  
Sincerely,  
Gabriel Staples  
http://www.ElectricRCAircraftGuy.com  


_This library layout and structure conforms with the [Arduino Library specification 1.5 library format (rev. 2.2)](https://arduino.github.io/arduino-cli/latest/library-specification/)._
