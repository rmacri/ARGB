/*
  ARGB library demo -  Riccardo Macri - March 2015

  Simple demo of alpha fading and offscreen drawing with scrolling text
*/

#include <argb.h>
#include "text.h"

TextDisplay fxtext;

void DrawHatch(byte w,
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

// for this demo, alternate hatches of colour/transparency
// have fun by changing these and also changing the xoffset/yoffset
// parameters to the DrawHatch call
// or set the colours using GetRandomColor()

ARGB color1 = 0xFF400000;   // ALPHA RED GREEN BLUE
ARGB color2 = 0x00000000;   //   AA   RR   GG   BB
ARGB color3 = 0x00000000;   // ALPHA=FF = opaque
ARGB color4 = 0xFF000040;   // ALPHA=00 = fully transparent

// fade amount, gets set in main loop
byte fade_level = 0;

// scrolling text display
ARGB textcolor = 0xFFFFFF80;
byte textspeed = 6;  // lower = faster

void setup()
{
 Argb.init();
 fxtext.Reset();
 fxtext.SetColor(textcolor);
 fxtext.SetSpeed(textspeed);
}

void loop()
{
 // this loops every frame (see bottom)

 static int frames = 0;

 if ((frames & 1) == 0)
  {
   // Every other frame, 62 frames a second

   // draw to "off screen" buffer
   Argb.SelectAltBuffer();

   // fade its previous contents a little (255 = minimum fade).
   Argb.Fade(fade_level);

   // alternate hatches, about twice a second 
   if (frames % 64 == 0)
    if (frames % 128 == 0)
     DrawHatch(2,
               color1,
               color2,
               0,0);
    else
     DrawHatch(2,
               color3,
               color4,
               0,0);

   // every 512
   if ((frames & 0x1FF) == 0)
    fade_level = random(0xe0,0xFF);

   Argb.SelectMainBuffer();
   Argb.CopyAltToMain();

   // clock_tod counts time of day, use it to re-trigger text
   // every 15 seconds here
   if (ARGB_clock_tod % 15 == 0)
    {
     // get a random colour and blend the text colour with it

     textcolor = GetRandomColor();
     fxtext.Set("Hello...");
    }

   // draw text - onto panel buffer not offscreen
   fxtext.Update();

   // cross blend text color towards the latest random color
   fxtext.SetColor(BlendARGB(fxtext.Color(),textcolor,0xD0));
  }

 // count frames
 frames++;

 // wait for the last scan line of the current frame. This 
 // regulates our speed irrespective of the processing time above
 // (as long as its less than a frame)

 while (!ARGB_user_frame)
  delay(1); // 1ms while we wait for timer interrupt to finish a frame

 // reset the frame flag for next time we loop around
 ARGB_user_frame = 0;
}
