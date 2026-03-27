#include <argb.h>
#include "text.h"

// margin to left for first pixel where text is definitely not visible plus
// a few pixels for "unfade" time
#define CHAR_TEST_WIDTH 8

void TextDisplay::Reset()
{
 *out_text  = 0;
 color      = 0xFFFFFFFF;

 step_ticks = 5;                 // how many ticks for each step
 step_count = step_ticks;     
 x_scroll = 32767;
}

void  TextDisplay::Tick()
{
 if (!--step_count)
  {
   step_count = step_ticks;
   x_scroll++;               // Update() handles stopping
  }
}

void TextDisplay::Set(const char * str)
{
 Reset();
 strncpy(out_text,str,MAX_TEXT);
 out_text[MAX_TEXT] = '\0';
 if (*out_text)
  x_scroll   = -4;                // start with text to the right of last pixel
}

void TextDisplay::Update()
{
 byte did_fade = 0;

 if (x_scroll < 32767)
  {
   Tick();

   char * c    = out_text;
   int text_px = ARGB_MAX_X-x_scroll;

   const byte text_fade = 0x40;  // darken background when text

   // progressively fade as text approaches RHS margin
   if ((text_px >= ARGB_MAX_X) && (text_px <= (ARGB_MAX_X + 4)))
    {
     did_fade = 1;
     Argb.CopyAltToMainFade(text_fade + 0x20 * (text_px - ARGB_MAX_X));
    }
   else
    while ((text_px < ARGB_MAX_X) && *c)
     {
      if (!did_fade && (text_px > -CHAR_TEST_WIDTH))  // will be visible
       {
        did_fade = 1;
        Argb.CopyAltToMainFade(text_fade);
       }
      
      text_px += 1 + Argb.DrawChar(*(c++),text_px,0,color);
     }

   // nothing shown - set end until we get reset
   if (text_px < -CHAR_TEST_WIDTH)
    x_scroll = 32767;
  }

 if (!did_fade)
  Argb.CopyAltToMain();
}
