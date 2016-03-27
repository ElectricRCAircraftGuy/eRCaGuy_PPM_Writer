/*
PPM_Writer_demo2.ino
By Gabriel Staples
http://www.ElectricRCAircraftGuy.com 
My contact info is available by clicking the "Contact Me" tab at the top of my website.
Written: 26 March 2016
Updated: 26 March 2016

LICENSE: GNU GPLV3 or later (refer to .h file and attached license for details)

-outputs PPM signal on Arduino pin 9
*/

#include <eRCaGuy_PPM_Writer.h>

const byte CH1 = 0,
           CH2 = 1,
           CH3 = 2,
           CH4 = 3,
           CH5 = 4,
           CH6 = 5,
           CH7 = 6,
           CH8 = 7;


void setup()
{
  Serial.begin(115200);
  Serial.println(F("\n\nbegin"));
  
  //manually set a few channels
  PPMWriter.setChannelVal(CH1,900*2); //set channel 1 (index 0) in the PPM train to 900us
  PPMWriter.setChannelVal(CH2,1000*2); //set channel 2 (index 2) in the PPM train to 1000us
  PPMWriter.setChannelVal(CH3,1100*2);
  PPMWriter.setChannelVal(CH4,1200*2);
  
  PPMWriter.setPPMPeriod(20000*2UL); //currently you cannot set above 65535 since Timer1 is a 16-bit timer/counter, and I'm not taking into account rollovers past this value, *yet*; NB: the UL is *mandatory* to force the constant/literal multiplication here to follow unsigned long rules *instead of* signed int rules, which I believe are default otherwise here in this compiler's C++ handling; take out the UL and you'll see weird results/errors for any values where the result of the multiplication is > 32767, which is the max value storable in a signed int.
  Serial.print(F("PPMPeriod(0.5us) = ")); Serial.println(PPMWriter.getPPMPeriod());
  Serial.print(F("PPMFreq(Hz) = ")); Serial.println(PPMWriter.getPPMFrequency());
  
  PPMWriter.begin(); //start the PPM train; default will be 1500us for all channels, 22ms for each PPM frame (I'm copying the Spektrum DX8 signal)
}

void loop()
{
  static unsigned long loopCount = 0;
  
  static byte ch_i = 0;
  if (PPMWriter.readChannelFlag(ch_i)==true)
  {
    unsigned long t_now = micros(); //us 
    unsigned long t_now2 = PPMWriter.getMicros(); //us 
    unsigned long count_now = PPMWriter.getCount(); //0.5us units
    static unsigned long t_now_old = t_now; //us 
    static unsigned long t_now2_old = t_now2_old; //us 
    static unsigned long count_now_old = count_now; //0.5us 
    
    //print data 
    Serial.print(F("ch_i = ")); Serial.print(ch_i); Serial.print(F(", frameNum = ")); 
    Serial.print(PPMWriter.getFrameNumber());
    Serial.print(F(", t_now(us) = ")); Serial.print(t_now); Serial.print(F(", t_now2(us) = ")); Serial.print(t_now2);
    Serial.print(F(", count_now(0.5us) = ")); Serial.print(count_now);
    //deltas:
    Serial.print(F(", delta times = ")); Serial.print(t_now - t_now_old); Serial.print(", "); 
    Serial.print(t_now2 - t_now2_old); Serial.print(", "); Serial.println(count_now - count_now_old); 
    
    //updates
    t_now_old = t_now; //us 
    t_now2_old = t_now2; //us 
    count_now_old = count_now; //0.5us 
      
    // ch_i++;
    // if (ch_i >= PPMWriter.getNumChannels())
      // ch_i = 0; //reset 
  }

}



