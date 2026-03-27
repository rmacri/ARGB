#include <argb.h>
#include <isin.h>
#include "tempo.h"
#include "effect.h"


ARGB  EffectBase::colors[4]  = {0xFFC00000,0xFF000040,0xFF00FF00,0xFF808000};
byte EffectBase::fade_level = 0xc8;
byte EffectBase::xoffset    = 0;
byte EffectBase::yoffset    = 0;
byte EffectBase::half_beats = 0;
byte EffectBase::sub_mode   = 0;

/////////////////////////////////////////////////////////////////////////////////////////////
//
// Implementation of effects
//

void HatchDisplay::DrawHatch(byte w,
                             ARGB color1,
                             ARGB color2,
                             byte xoffset,
                             byte yoffset)
{
 // draw hatch patterns, Saturday Night Fever style :)
 POINT x = -xoffset,y = -yoffset;

 if (!w)
  w = ARGB_MAX_X;

 while (y < ARGB_MAX_Y)
  {
   POINT x1 = x;

   while (x1 < ARGB_MAX_X)
    {
     Argb.FillRect(x1,y,w,w,color1);
     x1 += w;
     Argb.FillRect(x1,y,w,w,color2);
     x1 += w;
    }
   y += w;
   swap(color1,color2);
  }  
}

void HatchDisplay::Reset(int first)
{
 xdelta  = 1;
 ydelta  = 0;

 if (!first)
  PickColors();
}


void HatchDisplay::PickColors()
{
 // stick to primaries and mix
 // 0 = red
 // 4 = green
 // 8 = blue

 // choose 2 different primaries
 byte c1 = random(3) * 4;
 byte c2 = c1 + random(1,3) * 4;
            
 byte r = random(100);

 // sometimes make one a blend
 if (r > 90)
  if (r & 1)
   c2 += 2;
  else
   c2 -= 2;
 
 colors[0] = GetBaseColor(c1);
 colors[1] = GetBaseColor(c2);

 colors[0] = BlendARGB(colors[0],0,0,random(0x80,0xe0));
 colors[1] = BlendARGB(colors[1],0,0,random(0x00,0xa0));

 r = random(100);
 if (r < 50)
  sub_mode = !sub_mode;

 r = random(100);

 if (sub_mode)
  {
   if (r < 10)
    xdelta = -xdelta;
   else
    if (r < 20)
     ydelta = -ydelta;
    else
     if (r < 40)
      do
       xdelta = random(3)-1;
      while (!xdelta && !ydelta);
     else
      if (r < 50)
       do
        ydelta = random(3)-1;
       while (!xdelta && !ydelta);
  }
 else
  {
   fade_level = random(0xb0,0xf0);

   if (r < 30)
    half_beats = !half_beats;
  }
}

 void HatchDisplay::Beat(unsigned int beatcount,unsigned int beatbits)
 {
  if (beatbits & BEAT_4BAR)
   PickColors();
 
  if (!sub_mode)
   {
    Argb.Fade(fade_level);
 
    byte bar_4 = (beatcount & 0xc0) >> 6;  // count bars 0..3
    if (beatbits & BEAT_4)
     {
      DrawHatch(bar_4 + 1,colors[0],colors[1],0,0); 
      swap(colors[0],colors[1]);
     }
    else
     if (half_beats && (beatbits & BEAT_8))
      DrawHatch(bar_4 + 1,colors[1],colors[0],0,0);
   }
  else
   {
    if (beatbits & BEAT_32)
     {
      // note other effects use xoffset but we can cope with it
      DrawHatch(4,colors[1],colors[0],xoffset,yoffset);
      xoffset = (xoffset + xdelta) & 7;
      yoffset = (yoffset + ydelta) & 7;
     }
   }
}

void TunnelDisplay::Reset(int first)
{
 if (first)
  {
   counter     = 0;
   direction   = 1;
   xoffset     = 0;
  }
 else
  fade_level = 0x60;
}

void TunnelDisplay::Beat(unsigned int beatcount,unsigned int beatbits)
{
 if (beatbits & BEAT_32)
  {
   if (beatbits & BEAT_4)
    {
     colors[0] = GetRandomColor();
     if (ARGB_MAX_X > 8)
      xoffset = random(ARGB_MAX_X-4);
    }

   if (beatbits & BEAT_2BAR)
    sub_mode = random(50) > 30;

   if (beatbits & BEAT_2BAR)
    direction = !direction;

   if (beatbits & BEAT_4)
    {
     if (sub_mode)
      fade_level = random(0x90,0xf0);
     else
      fade_level = random(0x40,0x60);
    }

   Argb.Fade(fade_level);

   byte phase = (beatcount / 2) & 7;
   if (phase < 4)
    {
     if (direction)
      phase = 3-phase;

     if (sub_mode)
      {
       Argb.DrawCircle(3 + xoffset,3,3-phase,colors[0]);

       if (ARGB_MAX_X > 8)
        Argb.DrawCircle(ARGB_MAX_X - (3 + xoffset),3,3-phase,colors[0]);
      }
     else
      Argb.DrawRect(phase,phase,ARGB_MAX_X-phase-1,ARGB_MAX_Y-phase-1,colors[0]); 
    }
  }
}

void SpinFade::Reset(int first)
{
 if (!first)
  fade_level = random(0x80,0xd0);
}


void SpinFade::Beat(unsigned int beatcount,unsigned int beatbits)
{
 Argb.Fade(fade_level);

 if (beatbits & BEAT_BAR)
  Argb.Fill(GetRandomColor());

 int   range = 32;
 int   angle = 360 * (beatcount & 0x3F) / range;

 POINT x1 =  ARGB_MAX_X/2 + icos(angle) / 16;
 POINT y1 =  ARGB_MAX_Y/2 + isin(angle) / 16;
 POINT x2 =  ARGB_MAX_X/2 - icos(angle) / 16;
 POINT y2 =  ARGB_MAX_Y/2 - isin(angle) / 16;

 Argb.DrawLine(x1,y1,x2,y2,colors[0]);
 colors[0] = BlendARGB(colors[0],colors[1],0x0a);

 if (beatbits & BEAT_BAR)
  {
   colors[0] = colors[1];
   colors[1] = GetRandomColor();
  }
 
 if (beatbits & BEAT_2BAR)
  fade_level = random(0x80,0xd0);
}

//////////////////////////////////////////////////////////////////////////////

void BoxBeat::Reset(int first)
{
}

void BoxBeat::Beat(unsigned int beatcount,unsigned int beatbits)
{
 Argb.Fade(fade_level);

 if (beatbits & BEAT_4BAR)
  half_beats = (random(100) > 80) ? 1 : 0;

 if ((beatbits & BEAT_4) || (half_beats && (beatbits & BEAT_8)))
  {
   POINT x1 = random(ARGB_MAX_X / 2);
   POINT y1 = random(ARGB_MAX_Y / 2);
   POINT x2 = ARGB_MAX_X - random(ARGB_MAX_X / 2) - 1;
   POINT y2 = ARGB_MAX_Y - random(ARGB_MAX_Y / 2) - 1;

   ARGB color = SetAlpha(GetRandomColor(),random(0x80,0xd0));

   Argb.FillRect(x1,y1,x2-x1+1,y2-y1+1,color);
   fade_level = random(0xD0,0xF0);
  }
}

//////////////////////////////////////////////////////////////////////////////

void Wave::Reset(int first)
{
 if (!first)
  fade_level = FastFade();
}


void Wave::Beat(unsigned int beatcount,unsigned int beatbits)
{
 if (beatbits)
  {
   Argb.Clear();

   if (beatbits & BEAT_4)
    {
     byte p = (beatcount / 64) % 3;
     for (byte i=0;i<3;i++)
      colors[i] = GetBaseColor((p + i) * 4);   
    }

   for (byte i=0;i<3;i++)
    {
     int phase = (beatcount & 0x3F) * (1<<i) * 360 / (BEAT_4 * 4);
     byte gain = 4-i;

     ARGB ca = SetAlpha(BlendARGB(colors[i],0,0,0x10),0xb0 - i * 0x8);

     for (int x = 0;x < ARGB_MAX_X;x++)
      {
       int angle = x * 270 / (ARGB_MAX_X-1) + phase;
     
       // using  trunc(3.5 + 0.5 + 3.5 * gain * isin(angle) / 256.0)
       POINT y = (4 * 512 + 7 * gain * isin(angle) / 4) / 512;
     
       Argb.SetPixel(x,y,colors[i]);

       if (y < ARGB_MAX_Y-1)
        Argb.VLine(x,y+1,ARGB_MAX_Y-(y+1),ca);
      }
    }
  }
}

void Lissajous::Reset(int first)
{
 if (first)
  {
   angle1   = angle2 = 0;
   delta1   = 2800;
   delta2   = 3000;
   incdelta = 1;
  }
}

void Lissajous::Beat(unsigned int beatcount,unsigned int beatbits)
{
 Argb.Fade(0x60);

 if (beatbits & BEAT_4)
  colors[0] = BlendARGB(colors[0],GetRandomColor(),random(0xa0,0xf0));

 delta1 += incdelta;
 if (delta1 > 3500)
  delta1 = 2800;

 if (beatbits & BEAT_4)
  delta1 += random(50,500);

 int last_x = (ARGB_MAX_X / 2 * 512 + (ARGB_MAX_X - 1) * isin(angle1/100)) / 512;
 int last_y = (ARGB_MAX_Y / 2 * 512 + (ARGB_MAX_Y - 1) * icos(angle2/100)) / 512;

 for (byte i=0;i<12;i++)
  {
   angle1 += delta1;
   angle2 += delta2;

   if (angle1 >= 18000)
    angle1 -= 36000;
   if (angle2 >= 18000)
    angle2 -= 36000;

   int x = (ARGB_MAX_X / 2 * 512 + (ARGB_MAX_X - 1) * isin(angle1/100)) / 512;
   int y = (ARGB_MAX_Y / 2 * 512 + (ARGB_MAX_Y - 1) * icos(angle2/100)) / 512;
 
   Argb.DrawLine(last_x,last_y,x,y,colors[0]);

   last_x = x;
   last_y = y;
  }
}
