/*
PPM_Writer_demo.ino
By Gabriel Staples
3 July 2015
*/

#include <eRCaGuy_PPM_Writer.h>

void setup()
{
//  Serial.begin(115200);
//  Serial.println(MAX_NUM_CHANNELS);
  
  PPMWriter.begin(); //start the PPM train; default will be 1500us for all channels, 22ms for each PPM frame (I'm copying the Spektrum DX8 signal)
  
  //FOR DEBUGGING/CODE SPEED MEASURING
//  pinMode(2,OUTPUT);
}

void loop()
{
  PPMWriter.setChannelValue(0,900*2); //set channel 1 (index 0) in the PPM train to 900us
  PPMWriter.setChannelValue(2,2100*2); //set channel 3 (index 2) in the PPM train to 2100us
//  delay(3000);
//  PPMWriter.end();
//  Serial.println(PPMWriter.readChannelFlag(3));
  
}



