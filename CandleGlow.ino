/*  CandleGlow.ino - Make the LEDs glow in warm, orange light like candles
    Copyright (c) 2014 Jarrod Eliason, Dancing LEDs, L.L.C.
    http://www.dancingleds.com    
    
    Developed using the OctoWS2811 library
    http://www.pjrc.com/teensy/td_libs_OctoWS2811.html
    Copyright (c) 2013 Paul Stoffregen, PJRC.COM, LLC

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

  Required Connections
  --------------------
    pin 2:  LED Strip #1    OctoWS2811 drives 8 LED Strips.
    pin 14: LED strip #2    All 8 are the same length.
    pin 7:  LED strip #3
    pin 8:  LED strip #4    A 47-100 ohm resistor should used
    pin 6:  LED strip #5    between each Teensy pin and the
    pin 20: LED strip #6    wire to the LED strip, to minimize
    pin 21: LED strip #7    high frequency ringining & noise.
    pin 5:  LED strip #8
    pin 15 & 16 - Connect together, but do not use
    pin 4 - Do not use
    pin 3 - Do not use as PWM.  Normal use is ok.

  Pixabulbs are typically connected as GRB, while other WS2811 based devices are often RGB.
  If your lights look green, you have the wrong WS2811_RGB, WS2811_GRB type selected.
*/

#include <OctoWS2811.h>

// These global parameters define the random behavior of glow or flicker patterns
#define HIGHDWELLMIN 0 // Minimum dwell time at high brightness 0-7 corresponds to 1-8 cycles
#define HIGHDWELLMAX 4 // Maximum dwell time at high brightness 1-8 (random max is exclusive)
#define DOWNSTEPMIN 5  // Minimum down step size 0-7 corresponds to step size of 1-8
#define DOWNSTEPMAX 8  // Maximum down step size 1-8 (random max is exclusive)
#define LOWBRTMIN 0    // Minimum low brightness 0-255
#define LOWBRTMAX 100  // Maximum low brightness 1-256, LOWBRTMAX > LOWBRTMIN (random max is exclusive)
#define LOWDWELLMIN 0  // Minimum dwell time at low brightness 0-7 corresponds to 1-8 cycles
#define LOWDWELLMAX 2  // Maximum dwell time at low brightness 1-8 (random max is exclusive)
#define UPSTEPMIN 3    // Minimum up step size 0-7 corresponds to step size of 1-8
#define UPSTEPMAX 7    // Maximum up step size 1-8 (random max is exclusive)
#define HIBRTMIN 160   // Minimum high brightness 0-255 (should be > LOWBRTMAX)
#define HIBRTMAX 256   // Maximum high brightness 1-256, HIBRTMAX > HIBRTMIN (random max is exclusive)


const int ledsPerStrip = 50;
const int ledsTotal = 100;

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
byte intensity[ledsTotal]; // Array for current 8-bit intensity value
byte statusByte[ledsTotal]; // Array for current status byte: 3 bit counter, 3 bit increment/decrement value, 2 bit state
byte targetIntensity[ledsTotal]; // Array for target intensity

const int config = WS2811_GRB | WS2811_800kHz;

// The 24-bit colors are reduced to an 8-bit color map to save RAM
// Every pixel uses 3 additional bytes of RAM for the state machine
const int candlePalette[256] = {
   0x050200, 0x060300, 0x080400, 0x0c0600, 0x100800, 0x140a00, 0x180c00, 0x1c0e00, 
   0x201000, 0x241200, 0x281400, 0x2c1600, 0x301800, 0x341a00, 0x381c00, 0x3c1e00, 
   0x402000, 0x442100, 0x482200, 0x4c2300, 0x502400, 0x542500, 0x582600, 0x5c2700, 
   0x602800, 0x642900, 0x682a00, 0x6c2b00, 0x702c00, 0x742d00, 0x782e00, 0x7c2f00, 
   0x803000, 0x823000, 0x843100, 0x863100, 0x883200, 0x8a3200, 0x8c3300, 0x8e3300, 
   0x903400, 0x923400, 0x943500, 0x963500, 0x983600, 0x9a3600, 0x9c3700, 0x9e3700, 
   0xa03800, 0xa23800, 0xa43900, 0xa63900, 0xa83a00, 0xaa3a00, 0xac3b00, 0xae3b00, 
   0xb03c00, 0xb23c00, 0xb43d00, 0xb63d00, 0xb83e00, 0xba3e00, 0xbc3f00, 0xbe3f00, 
   0xc04000, 0xc44300, 0xc74600, 0xcb4900, 0xce4c00, 0xd24f00, 0xd55200, 0xd95500, 
   0xdc5800, 0xe05b00, 0xe35e00, 0xe76100, 0xea6400, 0xee6700, 0xf16a00, 0xf56d00, 
   0xf87000, 0xf87100, 0xf87201, 0xf87301, 0xf87402, 0xf87502, 0xf87603, 0xf87703, 
   0xf87804, 0xf87904, 0xf87a05, 0xf87b05, 0xf87c06, 0xf87d06, 0xf87e07, 0xf87f07, 
   0xf88008, 0xf88008, 0xf88108, 0xf88108, 0xf88209, 0xf88209, 0xf88309, 0xf88309, 
   0xf8840a, 0xf8840a, 0xf8850a, 0xf8850a, 0xf8860b, 0xf8860b, 0xf8870b, 0xf8870b, 
   0xf8880c, 0xf8880c, 0xf8890c, 0xf8890c, 0xf88a0d, 0xf88a0d, 0xf88b0d, 0xf88b0d, 
   0xf88c0e, 0xf88c0e, 0xf88d0e, 0xf88d0e, 0xf88e0f, 0xf88e0f, 0xf88f0f, 0xf88f0f, 
   0xf89010, 0xf89010, 0xf89010, 0xf89010, 0xf89110, 0xf89110, 0xf89110, 0xf89110, 
   0xf89211, 0xf89211, 0xf89211, 0xf89211, 0xf89311, 0xf89311, 0xf89311, 0xf89311, 
   0xf89412, 0xf89412, 0xf89412, 0xf89412, 0xf89512, 0xf89512, 0xf89512, 0xf89512, 
   0xf89613, 0xf89613, 0xf89613, 0xf89613, 0xf89713, 0xf89713, 0xf89713, 0xf89713, 
   0xf89814, 0xf89814, 0xf89814, 0xf89814, 0xf89914, 0xf89914, 0xf89914, 0xf89914, 
   0xf89a15, 0xf89a15, 0xf89a15, 0xf89a15, 0xf89b15, 0xf89b15, 0xf89b15, 0xf89b15, 
   0xf89c16, 0xf89c16, 0xf89c16, 0xf89c16, 0xf89d16, 0xf89d16, 0xf89d16, 0xf89d16, 
   0xf89e17, 0xf89e17, 0xf89e17, 0xf89e17, 0xf89f17, 0xf89f17, 0xf89f17, 0xf89f17, 
   0xf8a018, 0xf8a018, 0xf8a018, 0xf8a018, 0xf8a119, 0xf8a119, 0xf8a119, 0xf8a119, 
   0xf8a21a, 0xf8a21a, 0xf8a21a, 0xf8a21a, 0xf8a31b, 0xf8a31b, 0xf8a31b, 0xf8a31b, 
   0xf8a41c, 0xf8a41c, 0xf8a41c, 0xf8a41c, 0xf8a51d, 0xf8a51d, 0xf8a51d, 0xf8a51d, 
   0xf8a61e, 0xf8a61e, 0xf8a61e, 0xf8a61e, 0xf8a71f, 0xf8a71f, 0xf8a71f, 0xf8a71f, 
   0xf8a820, 0xf8a820, 0xf8a820, 0xf8a820, 0xf8a921, 0xf8a921, 0xf8a921, 0xf8a921, 
   0xf8aa22, 0xf8aa22, 0xf8aa22, 0xf8aa22, 0xf8ab23, 0xf8ab23, 0xf8ab23, 0xf8ab23, 
   0xf8ac24, 0xf8ac24, 0xf8ac24, 0xf8ac24, 0xf8ad25, 0xf8ad25, 0xf8ad25, 0xf8ad25, 
   0xf8ae26, 0xf8ae26, 0xf8ae26, 0xf8ae26, 0xf8af27, 0xf8af27, 0xf8af27, 0xf8b028
};

/*const int starPalette[256] = {
   0x0, 0x10101, 0x20202, 0x30303, 0x40404, 0x50505, 0x60606, 0x70707, 
   0x80808, 0x90909, 0xa0a0a, 0xb0b0b, 0xc0c0c, 0xd0d0d, 0xe0e0e, 0xf0f0f, 
   0x101010, 0x111111, 0x121212, 0x131313, 0x141414, 0x151515, 0x161616, 0x171717, 
   0x181818, 0x191919, 0x1a1a1a, 0x1b1b1b, 0x1c1c1c, 0x1d1d1d, 0x1e1e1e, 0x1f1f1f, 
   0x202020, 0x212121, 0x222222, 0x232323, 0x242424, 0x252525, 0x262626, 0x272727, 
   0x282828, 0x292929, 0x2a2a2a, 0x2b2b2b, 0x2c2c2c, 0x2d2d2d, 0x2e2e2e, 0x2f2f2f, 
   0x303030, 0x313131, 0x323232, 0x333333, 0x343434, 0x353535, 0x363636, 0x373737, 
   0x383838, 0x393939, 0x3a3a3a, 0x3b3b3b, 0x3c3c3c, 0x3d3d3d, 0x3e3e3e, 0x3f3f3f, 
   0x404040, 0x414141, 0x424242, 0x434343, 0x444444, 0x454545, 0x464646, 0x474747, 
   0x484848, 0x494949, 0x4a4a4a, 0x4b4b4b, 0x4c4c4c, 0x4d4d4d, 0x4e4e4e, 0x4f4f4f, 
   0x505050, 0x515151, 0x525252, 0x535353, 0x545454, 0x555555, 0x565656, 0x575757, 
   0x585858, 0x595959, 0x5a5a5a, 0x5b5b5b, 0x5c5c5c, 0x5d5d5d, 0x5e5e5e, 0x5f5f5f, 
   0x606060, 0x616161, 0x626262, 0x636363, 0x646464, 0x656565, 0x666666, 0x676767, 
   0x686868, 0x696969, 0x6a6a6a, 0x6b6b6b, 0x6c6c6c, 0x6d6d6d, 0x6e6e6e, 0x6f6f6f, 
   0x707070, 0x717171, 0x727272, 0x737373, 0x747474, 0x757575, 0x767676, 0x777777, 
   0x787878, 0x797979, 0x7a7a7a, 0x7b7b7b, 0x7c7c7c, 0x7d7d7d, 0x7e7e7e, 0x7f7f7f, 
   0x808080, 0x818181, 0x828282, 0x838383, 0x848484, 0x858585, 0x868686, 0x878787, 
   0x888888, 0x898989, 0x8a8a8a, 0x8b8b8b, 0x8c8c8c, 0x8d8d8d, 0x8e8e8e, 0x8f8f8f, 
   0x909090, 0x919191, 0x929292, 0x939393, 0x949494, 0x959595, 0x969696, 0x979797, 
   0x989898, 0x999999, 0x9a9a9a, 0x9b9b9b, 0x9c9c9c, 0x9d9d9d, 0x9e9e9e, 0x9f9f9f, 
   0xa0a0a0, 0xa1a1a1, 0xa2a2a2, 0xa3a3a3, 0xa4a4a4, 0xa5a5a5, 0xa6a6a6, 0xa7a7a7, 
   0xa8a8a8, 0xa9a9a9, 0xaaaaaa, 0xababab, 0xacacac, 0xadadad, 0xaeaeae, 0xafafaf, 
   0xb0b0b0, 0xb1b1b1, 0xb2b2b2, 0xb3b3b3, 0xb4b4b4, 0xb5b5b5, 0xb6b6b6, 0xb7b7b7, 
   0xb8b8b8, 0xb9b9b9, 0xbababa, 0xbbbbbb, 0xbcbcbc, 0xbdbdbd, 0xbebebe, 0xbfbfbf, 
   0xc0c0c0, 0xc1c1c1, 0xc2c2c2, 0xc3c3c3, 0xc4c4c4, 0xc5c5c5, 0xc6c6c6, 0xc7c7c7, 
   0xc8c8c8, 0xc9c9c9, 0xcacaca, 0xcbcbcb, 0xcccccc, 0xcdcdcd, 0xcecece, 0xcfcfcf, 
   0xd0d0d0, 0xd1d1d1, 0xd2d2d2, 0xd3d3d3, 0xd4d4d4, 0xd5d5d5, 0xd6d6d6, 0xd7d7d7, 
   0xd8d8d8, 0xd9d9d9, 0xdadada, 0xdbdbdb, 0xdcdcdc, 0xdddddd, 0xdedede, 0xdfdfdf, 
   0xe0e0e0, 0xe1e1e1, 0xe2e2e2, 0xe3e3e3, 0xe4e4e4, 0xe5e5e5, 0xe6e6e6, 0xe7e7e7, 
   0xe8e8e8, 0xe9e9e9, 0xeaeaea, 0xebebeb, 0xececec, 0xededed, 0xeeeeee, 0xefefef, 
   0xf0f0f0, 0xf1f1f1, 0xf2f2f2, 0xf3f3f3, 0xf4f4f4, 0xf5f5f5, 0xf6f6f6, 0xf7f7f7, 
   0xf8f8f8, 0xf9f9f9, 0xfafafa, 0xfbfbfb, 0xfcfcfc, 0xfdfdfd, 0xfefefe, 0xffffff
};
*/

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

void setup() {
  leds.begin();
  leds.show();
  
  // Initialize new variables
  for (int i=0; i < ledsTotal; i++) {
     statusByte[i] = 0x38;
     intensity[i] = 0;
     targetIntensity[i] = 255; // Start all lights rising towards max
  }
}


void loop() {
  int microsec = 40000;  // 40 ms delay between string updates

  for (int i = 0; i < ledsTotal; i++) {
     updateIntensity(i, &intensity[i], &statusByte[i], &targetIntensity[i]);
     leds.setPixel(i, candlePalette[intensity[i]]);
  }
  leds.show();
  delayMicroseconds(microsec);
}

// This is the state machine for updating the intensity for every pixel on the string
// Only update this code if you truly understand how it works.  A wide range of effects
// can be achieved by changing the parameters and palette defined above.
void updateIntensity (int i, byte* pintensity, byte* pstatus, byte* ptarget)
{
  int newtest;
  byte curstate = (*pstatus & 0xC0) >> 6;
  byte incdec = (*pstatus & 0x30) >> 3;
  byte dcnt = (*pstatus & 0x7);
  
  
  switch (curstate)
  {
     case 0: // Rising intensity
     {
        newtest = *pintensity + incdec + 1;
        if (newtest >= *ptarget) { // maximum intensity reached
           *pintensity = *ptarget;
           // Update state, pick high dwell time and reset count
           curstate = 1;
           incdec = random (HIGHDWELLMIN,HIGHDWELLMAX) & 0x7; 
           dcnt = 0;
           *pstatus = (curstate << 6) | (incdec << 3) | dcnt;
        }
        else {
           *pintensity = byte(newtest);
        }
        break;
     }
     case 1: // Dwelling at max intensity
     {
        dcnt++;
        if (dcnt > incdec) {
        // Dwell at max over.  Update state, pick ramp down target and step
           curstate = 2;
           incdec = random (LOWDWELLMIN, LOWDWELLMAX) & 0x7;
           dcnt = 0;
           *ptarget = random (LOWBRTMIN, LOWBRTMAX);
        }
        *pstatus = (curstate << 6) | (incdec << 3) | dcnt;
        break;
     }
     case 2: // Falling intensity
     {
        newtest = *pintensity - incdec - 1;
        if (newtest <= *ptarget) { // minimum intensity reached
           *pintensity = *ptarget;
           // Update state, pick low dwell time and reset count
           curstate = 3;
           incdec = random (LOWDWELLMIN, LOWDWELLMAX) & 0x7;
           dcnt = 0;
           *pstatus = (curstate << 6) | (incdec << 3) | dcnt;
        }
        else {
           *pintensity = byte(newtest);
        }
        break;       
     }
     case 3: // Dwelling at min intensity
     {
        dcnt++;
        if (dcnt > incdec) {
        // Dwell at min over.  Update state, pick ramp up target and step
           curstate = 0;
           incdec = random (UPSTEPMIN, UPSTEPMAX) & 0x7;
           dcnt = 0;
           *ptarget = random (HIBRTMIN, HIBRTMAX);
        }
        *pstatus = (curstate << 6) | (incdec << 3) | dcnt;
        break;       
     }
     default:
        *pintensity = i*4;
  }
  
}

