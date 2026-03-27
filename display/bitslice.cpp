#include <argb.h>
#include "bitslice.h"

#define TARGET_FADE 0x20
#define FADE_STEP   0x08

void BitSlice::Reset()
{
 memset(buffer,0,sizeof(buffer));
 color      = 0xFFB0FFB0;
 text_fade  = 0x00;
 active     = 0;
}

void BitSlice::AddAndShift(byte b)
{
 memmove(buffer,buffer+1,ARGB_MAX_X-1);
 buffer[ARGB_MAX_X-1] = b;
}

void BitSlice::Update()
{
 // fade background when we start

 if (active)
  {
   if (text_fade != TARGET_FADE)
    text_fade -= FADE_STEP;

   Argb.CopyAltToMainFade(text_fade);

   if (active)
    {
     for (byte x = 0; x < ARGB_MAX_X; x++)
      {
       char b = buffer[x];

       // unrolled for speed and shifting is slow
  
       if (b & 0x01)
        Argb.SetPixel(x,0,color);
       if (b & 0x02)
        Argb.SetPixel(x,1,color);
       if (b & 0x04)
        Argb.SetPixel(x,2,color);
       if (b & 0x08)
        Argb.SetPixel(x,3,color);
       if (b & 0x10)
        Argb.SetPixel(x,4,color);
       if (b & 0x20)
        Argb.SetPixel(x,5,color);
       if (b & 0x40)
        Argb.SetPixel(x,6,color);
       if (b & 0x80)
        Argb.SetPixel(x,7,color);
      }
    }
  }
 else
  {
   if (text_fade)
    text_fade += FADE_STEP;

   if (text_fade)
    Argb.CopyAltToMainFade(text_fade);
   else
    Argb.CopyAltToMain();
  }  
}
