#ifndef ARGB_H
#define ARGB_H

/*

 ARGB Graphics Library by Riccardo Macri

 An 8x8 (or 16x8, 24x8) RGB matrix driver for the MY9221 based "Rainbow Block"
 board(s) and MakerDuino from Maker Studio, featuring high speed and support for
 alpha blending.

 (C) 2015 Riccardo Macri   riccardo.macri@gmail.com

 Key features:
  - RGB + Alpha (transparency) support
  - Fast fill/hline/vline/fade/scroll
  - double buffer framebuffer (flicker free and alpha blend effects)
  - highly optimised MY9221 interface, ghost and flicker free
  - fast ISR gives more time for animation effects
  - ISR provides real time clock counter (very accurate with a MakerDuino which
    has a crystal oscillator)
  - proportional width text output plus separate narrow numerics for clocks
  - analog sampling at panel line update rate

 Don't steal my work. If you profit from this, give value for value.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 ACKNOWLEDGEMENTS:

 This is a substantial rewrite of the RainbowDuino library. Acknowledgements to
 the code I based it on:
  - Seeed Technology Inc
  - Albert.Miao, Visweswara R

 In turn based on the rainbowduino, modified by www.elecrow.com 

 This library uses line, circle and other shape drawing algorithms and code
 as presented in paper "The Beauty of Bresenham's Algorithm" by Alois Zingl,
 Vienna, Austria. The original author has kindly released these example
 programs with no copyright.

*/

#include <Arduino.h>
#include <avr/pgmspace.h>

// The library works with 1 to 3 panels. 2 are expected for the clock.
// With 3, the double buffers use significant RAM. you could comment out the
// off screen buffer bits if you need more.
///

#define ARGB_PANELS            1

// Frame rate: for integral counter, either 100Hz or 125Hz.
// 100Hz gives more time for processing  (1.25ms per line)
// 125Hz results in less display flicker (1ms per line)
// don't use other values unless you understand the math this affects
#define ARGB_FRAMERATE 125

#define ARGB_MAX_X        (8*ARGB_PANELS)
#define ARGB_MAX_Y        8

typedef int           POINT;
typedef uint32_t      ARGB;

// real time clock/timing info from ISR
extern volatile byte          ARGB_user_frame;           // ISR sets every frame, user clears
extern volatile unsigned int  ARGB_clock_ms;             // counts ms
extern volatile byte          ARGB_adcdata[ARGB_MAX_Y];  // analog samples
extern byte                   ARGB_dark;                 // set for dimmer display

// time of day updated by interrupt. Clock takes care in reading this
// as reading it is not atomic
extern volatile unsigned long ARGB_clock_tod;
// set the time of day
extern void ARGB_SetTime(unsigned long new_tod);

// framebuffer_1 is sent to the panel. framebuffer_2 can be used
// for compositing and merged/faded into buffer_1
extern byte   framebuffer_1[ARGB_MAX_X * ARGB_MAX_Y * 3];
extern byte   framebuffer_2[ARGB_MAX_X * ARGB_MAX_Y * 3];

class RGBDisplay
{
 byte * framebuffer;   // points to buffer for drawing below

 public:

 void init();

 // double buffering:
 void SelectMainBuffer() {framebuffer = framebuffer_1;}
 void SelectAltBuffer()  {framebuffer = framebuffer_2;}
 void CopyAltToMain();
 void CopyMainToAlt();

 // this suppors partial off screen for x position, returns width in pixels
 byte DrawChar(byte  ascii,
               POINT px,
               POINT py,
               ARGB  color);

 // this supports partial off screen for x position
 void DrawDigit(byte  digit, // 0..9, 10 = colon
                POINT px,    // top left
                POINT py,
                ARGB  color);

 // this scrolls numeric digits
 void BlendDigits(byte  digit1,  // top digit, 0..9, 10 or 255 for none
                  byte  digit2,  // bottom digit 0..9, 10 or 255 for none
                  byte  blend,   // 0..255 for shift up percentage
                  POINT px,
                  POINT py,
                  ARGB  color);
  
 void Clear();
 void SetPixel(POINT x,POINT Y,ARGB color);
 void Fill(ARGB); 
 void HLine(POINT x,POINT y,byte w,ARGB color);
 void VLine(POINT x,POINT y,byte w,ARGB color);
 void DrawRect(POINT x1,POINT y1,POINT x2,POINT y2,ARGB color);
 void FillRect(POINT x,POINT y,byte w,byte h,ARGB color);
 void DrawCircle(POINT poX, POINT poY, byte r, ARGB color);
 void FillCircle(POINT poX, POINT poY, byte r, ARGB color);
 void DrawLine(POINT x0,POINT y0,POINT x1,POINT y1,ARGB color);
 void Fade(byte alpha);
 void ScrollLeft(byte steps);
};

inline ARGB MakeARGB(byte a,byte r,byte g,byte b)
{
 ARGB c;
 byte * p = (byte*)(&c);
 *(p++) = b; *(p++) = g; *(p++) = r; *p = a;
 return c;
}

inline ARGB SetAlpha(ARGB color,byte alpha)
{
 // note: bit shifting of multi-bytes slower than direct stuffing
 byte * p = (byte*)(&color);
 *(p+3)   = alpha;
 return color;
}

inline void swap(ARGB & a,ARGB & b)
{ ARGB temp = a; a = b; b = temp; }

// primary color table, pure R,G,B at 0,4,8 and blends are 1-3,5-7,9-11
extern ARGB GetBaseColor(byte ci);

// smoothly blend two colors from the color table
extern ARGB BlendBaseColors(byte ci1,         // first color index, 0-11
                            byte ci2,         // second color index, 0-11                            
                            byte blend,       // blending, 0=first, 255=second
                            byte fade = 255); // result fade, 0 = total black

// smoothly blend two arbitrary ARGBs and optionally darken result
extern ARGB BlendARGB(ARGB c1,          // first color
                      ARGB c2,          // second color
                      byte ratio,       // blending factor 0=first, 2255=second
                      byte fade = 255); // result fade, 0 = total blacken

extern ARGB GetRandomColor();

extern RGBDisplay Argb;

#endif
