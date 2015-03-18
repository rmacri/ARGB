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

#include "argb.h"

// buffers are ordered for fastest transfer by the ISR, see the ISR for detail
byte   framebuffer_1[ARGB_MAX_X * ARGB_MAX_Y * 3];   // main
byte   framebuffer_2[ARGB_MAX_X * ARGB_MAX_Y * 3];   // off-screen

byte * outbuf = framebuffer_1;

// Note: PORTD=D0..7, PORTB=D8..13, PORTC=AIn 0..5
//
//  Pin    Mode    Function
//  2-3    In      Button inputs for clock 
//  4-6    Out     Row Select (0 = top with connector on left)
//  7      Out     Display Enable
//  8      Out     MY9221 Clock
//  9      Out     MY9221 Data
//

// data and clock lines DDR
#define DDR_Data  DDRB
#define PORT_Data PORTB
#define BIT_Data  0x02  // digital 9

#define DDR_Clk   DDRB
#define PORT_Clk  PORTB
#define BIT_Clk   0x01  // digital 8

// 3-to-8 decoder and enable (on same port)
#define DDR_Lines   DDRD
#define PORT_Lines  PORTD
#define BIT_Lines   0x70    // line select mask
#define BIT_Enable  0x80    // display enable
#define SHIFT_Lines 0x04    // bit shift to get line # to its port bits

#define DDR_LED     DDRB
#define PORT_LED    PORTB
#define BIT_LED     0x20

// ADC registers
// ADMUX:
#define ADC_AVCCREF B01000000
#define ADC_LEFTADJ B00100000
#define ADC_MUX     B00000111
 
// ADCSRA:
#define ADC_ENABLE B10000000  // enable ADC
#define ADC_ADSC   B01000000  // start
#define ADC_ADATE  B00100000  // auto trigger
#define ADC_ADIF   B00010000  // conversion complete/interrupt
#define ADC_ADIE   B00001000  // interrupt enable
#define ADC_ADPS   B00000111  // prescale, 111 = 128

void RGBDisplay::init()
{
 // data output
 DDR_Data  |= BIT_Data;
 PORT_Data &= ~BIT_Data;

 // clock output
 DDR_Clk   |= BIT_Clk;
 PORT_Clk  &= ~BIT_Clk;

 // enable and row select output
 DDR_Lines  |=  BIT_Lines | BIT_Enable;
 PORT_Lines &= ~(BIT_Lines | BIT_Enable);

 // enable pin 13 cpu board LED
 DDR_LED   |= BIT_LED;
 PORT_LED  |= BIT_LED;
 
 // set default buffer we draw into
 outbuf      = framebuffer_1;

 // set framebuffer pointer for interrupt routine
 framebuffer = framebuffer_1;

 // Set up the interrupt timer
 // timer count = XTAL / (prescaler * wanted_freq) - 1
 // The entire display is updated at 100Hz
 // With 8 rows, we interrupt at 800Hz
 // Uno clock frequency is 16MHz
 //
 // 16000000 / (1 * framerate * 8) - 1
 // 100Hz: 19999
 // 125Hz: 15999
 //
 // This makes my 2 boards more accurate
#define CLOCKADJ    1

#define USECOUNTER (16000000L / (ARGB_FRAMERATE * 8) - 1 + CLOCKADJ)

 // clear control registers and ensure timer stopped
 TCCR1A  = 0;
 TCCR1B  = 0;
 TCNT1   = 0;

 OCR1A   = USECOUNTER;  // 15999 for 1ms for 125Hz display rate
 TCCR1B  = 0x08;        // CTC1 - clear on compare mode
 TIMSK1  = _BV(OCIE1A); // output compare A interrupt
 TCCR1B |= _BV(CS10);   // start timer, no prescale

 // set up direct analog 0 reads (done in the ISR) preventing
 // us having to wait for samples. This is used to sample at 800Hz
 // for beat detectors etc 
 ADMUX  = ADC_AVCCREF | ADC_LEFTADJ;         // 8 bit mode, ADC 0
 ADCSRB = B00000000;
 DIDR0  = B00111111;                         // disable ADC line digital inputs
 ADCSRA = ADC_ENABLE | ADC_ADSC | ADC_ADPS;  // enable conversion
 
 // enable global interrupt
 sei();                 
}

//
// These directly manipulate the framebuffer for speed
//

void RGBDisplay::Clear()
{
 memset(framebuffer,0,ARGB_MAX_X * ARGB_MAX_Y * 3);
}

void RGBDisplay::ScrollLeft(byte steps)
{
 // scroll pixels left a column
 // pixels are right to left in memory 

 // leftmost blue pixel of first row
 byte * pto = framebuffer + 3 * (ARGB_MAX_X - 1) + 2;

 byte line = ARGB_MAX_Y;
 while (line--)
  {
   // shifting n-steps pixels and setting the last 'steps' to black

   byte cnt = ARGB_MAX_X-steps;
   while (cnt--)
    {
     *pto = *(pto-3*steps);  pto--;  // B
     *pto = *(pto-3*steps);  pto--;  // G
     *pto = *(pto-3*steps);  pto--;  // R
    }
 
   // now pointing at righthand pixel, zero it
  
   cnt = steps;
   while (cnt--)
    {  
     *pto = 0; pto--;
     *pto = 0; pto--;
     *pto = 0; pto--;
    }

   pto += ARGB_MAX_X * 2 * 3;   // skip to LHS blue pixel of next row
  }
}

void RGBDisplay::Fade(byte alpha)
{
 byte * pb = framebuffer;
 byte cnt  = ARGB_MAX_X * ARGB_MAX_Y / 4; // unrolled

 while (cnt--)
  {
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;

   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;

   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;

   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
   *pb = ((unsigned int)alpha * (*pb)) >> 8;  pb++;
  }
}

void RGBDisplay::Fill(ARGB color)
{
 byte b   = color;
 byte r   = color >> 8;
 byte g   = color >> 16;

 byte * p = framebuffer;

 byte cnt = ARGB_MAX_X * ARGB_MAX_Y / 4;   // unrolled
 while (cnt--)
  {
   *(p++) = r; *(p++) = g; *(p++) = b;
   *(p++) = r; *(p++) = g; *(p++) = b;
   *(p++) = r; *(p++) = g; *(p++) = b;
   *(p++) = r; *(p++) = g; *(p++) = b;
  }
}

void RGBDisplay::CopyAltToMain()
{
 memcpy(framebuffer_1,framebuffer_2,ARGB_MAX_X * ARGB_MAX_Y * 3);
}

void RGBDisplay::CopyMainToAlt()
{
 memcpy(framebuffer_2,framebuffer_1,ARGB_MAX_X * ARGB_MAX_Y * 3);
}

void RGBDisplay::SetPixel(POINT x,POINT y,ARGB color)
{
 // NOTE: No range checking! Callers must take responsibility
 // pixels ordered row-wise top-right to top-left in RGB order

 // get blue pixel first
 byte * pb = framebuffer + 2 + 3 * ((ARGB_MAX_X - 1 - x) + (y * ARGB_MAX_X));

 // testing determiend its quicker to pluck the bytes rather than shift
 byte * pc = (byte*)(&color);
 byte c_b = *(pc++);
 byte c_g = *(pc++);
 byte c_r = *(pc++);
 byte a   = *(pc++);

 byte a1 = ~a;

 if (a1)
  {   
   // alpha blending
   *pb = ((unsigned int)a1 * (*pb) + (unsigned int)a * c_b) >> 8;
   pb--;
   *pb = ((unsigned int)a1 * (*pb) + (unsigned int)a * c_g) >> 8;
   pb--;
   *pb = ((unsigned int)a1 * (*pb) + (unsigned int)a * c_r) >> 8;
  }
 else
  {
   // opaque
   *(pb--) = c_b;
   *(pb--) = c_g;
   *pb     = c_r;
  }
}

void RGBDisplay::HLine(POINT x,POINT y,byte w,ARGB color)
{
 if (y >= 0 && y < ARGB_MAX_Y)
  {
   while (x < 0 && w)
    {
     x++;
     w--;
    }
   
   while (w-- && (x < ARGB_MAX_X))
    SetPixel(x++,y,color);
  }
}

void RGBDisplay::VLine(POINT x,POINT y,byte w,ARGB color)
{
 if (x >= 0 && x < ARGB_MAX_X)
  {
   while ((y < 0) && w)
    {
     y++;
     w--;
    }

   while (w-- && (y < ARGB_MAX_Y))
    SetPixel(x,y++,color);
  }
}

void RGBDisplay::DrawRect(POINT x1,POINT y1,POINT x2,POINT y2,ARGB color)
{
 HLine(x1,y1,x2-x1+1,color);
 HLine(x1,y2,x2-x1+1,color);
 if (y1 < y2)
  {
   VLine(x1,y1+1,y2-y1-1,color);
   VLine(x2,y1+1,y2-y1-1,color);
  }
}

void RGBDisplay::FillRect(POINT x,POINT y,byte w,byte h,ARGB color)
{
 while ((x < 0) && w)
  {
   x++;
   w--;
  }

 while ((y < 0) && h)
  {
   y++;
   h--;
  }

 while (h-- && (y < ARGB_MAX_Y))
  {
   POINT tx = x;
   byte  tw = w;

   while (tw >= 4 && tx < (ARGB_MAX_X-3))
    {
     SetPixel(tx++,y,color);
     SetPixel(tx++,y,color);
     SetPixel(tx++,y,color);
     SetPixel(tx++,y,color);
     tw -= 4;
    }
    
   while (tw-- && (tx < ARGB_MAX_X))
    SetPixel(tx++,y,color);
   y++;
  }
}

void RGBDisplay::DrawCircle(POINT poX, POINT poY, byte r, ARGB color)
{
 char x = -r, y = 0, err = 2-2*r, e2;
 do
  {
   SetPixel(poX-x, poY+y,color);
   SetPixel(poX+x, poY+y,color);
   SetPixel(poX+x, poY-y,color);
   SetPixel(poX-x, poY-y,color);
   e2 = err;
   if (e2 <= y)
    {
     err += ++y*2+1;
     if (-x == y && e2 <= x) e2 = 0;
    }
   if (e2 > x) err += ++x*2+1;
  }
 while (x <= 0);
}

void RGBDisplay::FillCircle(POINT poX, POINT poY, byte r, ARGB color)
{
 char x = -r, y = 0, err = 2-2*r, e2;
 do
  {
   VLine(poX-x,poY-y,2*y,color);
   VLine(poX+x,poY-y,2*y,color);

   e2 = err;
   if (e2 <= y) 
    {
     err += ++y*2+1;
     if (-x == y && e2 <= x) e2 = 0;
    }
   if (e2 > x) err += ++x*2+1;
  }
 while (x <= 0);
}

void RGBDisplay::DrawLine(POINT x0,POINT y0,POINT x1,POINT y1,ARGB color)
{
 char dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
 char dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
 char err = dx+dy, e2; /* error value e_xy */

 for (;;)
  {
   if (x0 >= 0 && x0 < ARGB_MAX_X && y0 >= 0 && y0 < ARGB_MAX_Y)
    SetPixel(x0,y0,color);
   e2 = 2*err;
   if (e2 >= dy) /* e_xy+e_x > 0 */
    { 
     if (x0 == x1)
      break;
     
     err += dy;
     x0 += sx;
    }
   if (e2 <= dx) // /* e_xy+e_y < 0 */
    {
     if (y0 == y1)
      break;
     err += dx;
     y0 += sy;
    }
  }
}

// 
// Small numbers, stored as vertical strips, lowest bit is top row
//
// 0   1   2   3   4   5   6   7   8   9   :   H    M
// ***.*...***.***.*.*.***.***.***.***.***.....*.*.*.*
// *.*.*.....*...*.*.*.*...*.....*.*.*.*.*..*..*.*.***
// *.*.*...***.***.***.***.***...*.***.***.....***.***
// *.*.*...*.....*...*...*.*.*...*.*.*...*..*..*.*.*.*
//.***.*...***.***...*.***.***...*.***.***.....*.*.*.*

const byte numberFont[][3] PROGMEM =
 {
  {0x1F,0x11,0x1F},
  {0x00,0x00,0x1F},
  {0x1D,0x15,0x17},
  {0x15,0x15,0x1F},
  {0x07,0x04,0x1F},
  {0x17,0x15,0x1D},
  {0x1F,0x15,0x1D},
  {0x01,0x01,0x1F},
  {0x1F,0x15,0x1F},
  {0x17,0x15,0x1F},
  {0x0A,0x00,0x00},
  {0x1F,0x04,0x1F},
  {0x1F,0x06,0x1F}
 };

void RGBDisplay::DrawDigit(byte digit,
                           POINT         px,
                           POINT         py,
                           ARGB          color)
{
 for (char i=0;i<3;i++)
  {
   POINT plot_x = px + i;

   if (plot_x >= 0 && plot_x < ARGB_MAX_X)
    {
     byte cdata = pgm_read_byte(&numberFont[digit][i]);

     for (byte y=0;y<8;y++)
      {
       POINT plot_y = py+y;

       if (plot_y >= 0 && plot_y < ARGB_MAX_Y)
        if (cdata & (1 << y))
         SetPixel(plot_x,plot_y,color);
      }
    }
  }
}

void RGBDisplay::BlendDigits(byte  digit1,
                             byte  digit2,
                             byte  blend,
                             POINT px,
                             POINT py,
                             ARGB  color)
{
 if (digit1 == digit2)
  blend = 0;

 // read the 2 character bitmasks from program memory
 byte shift = blend / 32; // 0..7

 if (digit1 != 255)
  DrawDigit(digit1,px,py-shift           ,color);

 if (blend && digit2 != 255)
  DrawDigit(digit2,px,py-shift+7,color);
}
 

byte RGBDisplay::DrawChar(byte ascii,
                          POINT         px,
                          POINT         py,
                          ARGB          color)
{
 if ((ascii < 0x20) || (ascii > 0x7e))
  ascii = '-';

 byte width = 0;

 for (char i=7;i>=0;i--)
  {
   extern byte simpleFont[][8];    // font.c

   byte col = pgm_read_byte(&simpleFont[ascii-0x20][i]);

   if (col)
    {
     if (!width)
      width = i;

     int plot_x = px + i;

     if (plot_x >= 0 && plot_x < ARGB_MAX_X)
      {
       for (byte f=0;f<8;f++)
        if (col & (1 << f))
         SetPixel(px+i,py+f,color);
      }
    }
  }

 return width;
}

//////////////////////////////////////////////////////////////////////////////

ARGB BlendARGB(ARGB c1,ARGB c2,byte ratio,byte fade)
{
 byte ratio1 = ratio;
 byte ratio2 = ~ratio1;

 if (fade != 255)
  {
   ratio1 = ((unsigned int)fade * ratio1) >> 8;
   ratio2 = ((unsigned int)fade * ratio2) >> 8;
  }

 byte * cp1 = (byte*)(&c1);
 byte * cp2 = (byte*)(&c2);

 byte b = ((unsigned int)ratio2 * *(cp1++) + (unsigned int)ratio1 * *(cp2++)) >> 8;
 byte g = ((unsigned int)ratio2 * *(cp1++) + (unsigned int)ratio1 * *(cp2++)) >> 8;
 byte r = ((unsigned int)ratio2 * *(cp1++) + (unsigned int)ratio1 * *(cp2++)) >> 8;

 // dont fade the alpha
 byte a = ((unsigned int)(~ratio) * *(cp1++) + (unsigned int)(ratio) * *(cp2++)) >> 8;
   
 return MakeARGB(a,r,g,b);
}

// colour primaries. Green adjusted due to its higher intensity
const ARGB BaseColors[] PROGMEM =
 {0xFFFF0000, 0xFFFF2000, 0xFFFF8000, 0xFF808000,
  0xFF008000, 0xFF008080, 0xFF0080FF, 0xFF0020FF,
  0xFF0000FF, 0xFF8000FF, 0xFFFF00FF, 0xFFFF0080};

ARGB GetBaseColor(byte color1)
{
 return pgm_read_dword(&BaseColors[color1 % 12]);
}

ARGB BlendBaseColors(byte color1,byte color2,byte blend,byte fade)
{
 return  BlendARGB(pgm_read_dword(&BaseColors[color1 % 12]),
                   pgm_read_dword(&BaseColors[color2 % 12]),
                   blend,
                   fade);
}

ARGB GetRandomColor()
{
 return pgm_read_dword(&BaseColors[random(12)]);
}

//////////////////////////////////////////////////////////////////////////////
//
// ISR and display support functions
//

volatile byte          ARGB_user_frame = 0;       // flags one display frame
volatile unsigned int  ARGB_clock_ms   = 0;       // counts ms within a second
volatile unsigned long ARGB_clock_tod  = 0;       // seconds within a day
volatile byte         ARGB_adcdata[ARGB_MAX_Y];   // analog samples

// enable for darker display (eg:night mode)
byte                   ARGB_dark      = 0;

static void Send16Bit(unsigned int data)
{
 // notes:
 // - shifting a 16 bit value is slower than a test and branch
 // - note now this isn't used for pixel data its been looped again

 byte i = 16 / 4;

 while (i--)
  {
   PORT_Data = (data&0x8000) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
   PORT_Data = (data&0x4000) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
   PORT_Data = (data&0x2000) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
   PORT_Data = (data&0x1000) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;

   data <<= 4;   
  }
}


static void SendPixel(byte data)
{
 // top byte is always 0 so send it quick.

 PORT_Data = (PORT_Data & ~BIT_Data);

 PORT_Clk ^= BIT_Clk;  PORT_Clk ^= BIT_Clk;  PORT_Clk ^= BIT_Clk;  PORT_Clk ^= BIT_Clk;
 PORT_Clk ^= BIT_Clk;  PORT_Clk ^= BIT_Clk;  PORT_Clk ^= BIT_Clk;  PORT_Clk ^= BIT_Clk;

 PORT_Data = (data&0x80) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
 PORT_Data = (data&0x40) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
 PORT_Data = (data&0x20) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
 PORT_Data = (data&0x10) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
 PORT_Data = (data&0x08) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
 PORT_Data = (data&0x04) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
 PORT_Data = (data&0x02) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
 PORT_Data = (data&0x01) ? (PORT_Data | BIT_Data) : (PORT_Data & ~BIT_Data); PORT_Clk ^= BIT_Clk;
}

#define SendRPixel(a)  SendPixel(a)
#define SendGPixel(a)  SendPixel(a)
#define SendBPixel(a)  SendPixel(a)

void ARGB_SetTime(unsigned long new_tod)
{
 // disable interrupts before setting the multiple byte clock values
 cli();

 ARGB_clock_ms   = 0;
 ARGB_clock_tod  = new_tod;
 
 sei();
}

// This ISR is called when TIMER1 matches the count  
ISR(TIMER1_COMPA_vect)          
{
 static byte line       = 0;  // line we are about to send

 // MY9221 commands
 // 0x0400 = hi speed (didn't help 12 bit mode)
 // 0x0100 = 12 bit (too slow to use)
 // 0x0010 = APDM waveform

 const unsigned int CmdMode = 0x0010;

 int panels = ARGB_PANELS;

 // disable early, make display darker
 if (ARGB_dark)
  PORT_Lines &= ~BIT_Enable;

 while (panels--)
  { 
   // Data is in a linear array (R,G,B) with pixels in right to left order
   // as expected by the Rainbow Block

   Send16Bit(CmdMode);
   SendRPixel(*(outbuf++));  SendGPixel(*(outbuf++));  SendBPixel(*(outbuf++));
   SendRPixel(*(outbuf++));  SendGPixel(*(outbuf++));  SendBPixel(*(outbuf++));
   SendRPixel(*(outbuf++));  SendGPixel(*(outbuf++));  SendBPixel(*(outbuf++));
   SendRPixel(*(outbuf++));  SendGPixel(*(outbuf++));  SendBPixel(*(outbuf++));
        
   Send16Bit(CmdMode);
   SendRPixel(*(outbuf++));  SendGPixel(*(outbuf++));  SendBPixel(*(outbuf++));
   SendRPixel(*(outbuf++));  SendGPixel(*(outbuf++));  SendBPixel(*(outbuf++));
   SendRPixel(*(outbuf++));  SendGPixel(*(outbuf++));  SendBPixel(*(outbuf++));
   SendRPixel(*(outbuf++));  SendGPixel(*(outbuf++));  SendBPixel(*(outbuf++));
  }

 // MY9221 datasheet specifies 220us before latching data just sent.
 // The delays below are less but working on 4 display boards I've tried
 // plus we spend some time doing other housekeeping before latching.

 delayMicroseconds(30);

 // Use this time to blank the display and select the row
 // for the data just clocked in.
 // note: the blanking is quicker than the line decoding
 // note: delay above and below is split up to minimise blank time

 PORT_Lines &= ~BIT_Enable;
 delayMicroseconds(10);
 PORT_Lines = (PORT_Lines & ~BIT_Lines) | (line << SHIFT_Lines);

 // save 1.25ms analog samples. we set up the next sample after reading
 // the previous so it will definitely be ready by the next interrupt
 // application can use these for CRO, beat detect, etc.

 ARGB_adcdata[line] = ADCH;   // 8 bit ADC read
 ADCSRA |= ADC_ADSC;          // start next read

 // latch data for the row we just clocked in

 PORT_Data ^= BIT_Data; PORT_Data ^= BIT_Data; PORT_Data ^= BIT_Data; PORT_Data ^= BIT_Data;
 PORT_Data ^= BIT_Data; PORT_Data ^= BIT_Data; PORT_Data ^= BIT_Data; PORT_Data ^= BIT_Data;

 // end of frame & timekeeping updates

 if (++line >= ARGB_MAX_Y)
  {
   line            = 0;                   // just sent out bottom line
   ARGB_user_frame = 1;                   // 1/framerate has passed
   outbuf          = framebuffer_1;       // back to top of framebuffer

   ARGB_clock_ms += 1000/ARGB_FRAMERATE;  // one frame has passed, 10ms or 8ms

   if (ARGB_clock_ms >= 1000)
    {
     // one second passed

     ARGB_clock_ms = 0;              // either 8ms or 10ms are integer factors of 1000
     ARGB_clock_tod++;
     if (ARGB_clock_tod >= 86400)    // roll over on day
      ARGB_clock_tod = 0;

     PORT_LED  |= BIT_LED;      // turn on board LED on the second
    }
   else
    if (ARGB_clock_ms >= 20)
     PORT_LED      &= ~BIT_LED; // turn off board LED after 50ms
  }

 // give new row data time to settle before renabling display
 // anything less than this creates ghosts
 // note: MY9221 in 8 bit mode latches quickly, in >= 12 bit mode
 //       it did not latch before the next interrupt.

 delayMicroseconds(30);
 PORT_Lines |= BIT_Enable;  
}

RGBDisplay Argb;
