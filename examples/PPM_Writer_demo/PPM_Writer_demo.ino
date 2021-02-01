/*
PPM_Writer_demo.ino
By Gabriel Staples
http://www.ElectricRCAircraftGuy.com
My contact info is available by clicking the "Contact Me" tab at the top of my website.
Written: 3 July 2015
Updated: 5 July 2015

LICENSE: GNU GPLV3 or later (refer to .h file and attached license for details)

-outputs PPM signal on Arduino pin 9
*/

#include <eRCaGuy_PPM_Writer.h>

constexpr uint8_t CH1 = 0;
constexpr uint8_t CH2 = 1;
constexpr uint8_t CH3 = 2;
constexpr uint8_t CH4 = 3;
constexpr uint8_t CH5 = 4;
constexpr uint8_t CH6 = 5;
constexpr uint8_t CH7 = 6;
constexpr uint8_t CH8 = 7;

void setup()
{
    // manually set a few channels
    PPMWriter.setChannelVal(CH1, 900 * 2);   // set channel 1 (index 0) in the PPM train to 900us
    PPMWriter.setChannelVal(CH2, 1000 * 2);  // set channel 2 (index 2) in the PPM train to 1000us
    PPMWriter.setChannelVal(CH3, 1100 * 2);
    PPMWriter.setChannelVal(CH4, 1200 * 2);

    PPMWriter.begin();  // start the PPM train; default will be 1500us for all channels, 22ms for
                        // each PPM frame (I'm copying the Spektrum DX8 signal)
}

void loop()
{
}
