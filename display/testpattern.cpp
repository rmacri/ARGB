#include <Argb.h>
#include "testpattern.h"

void ShowTest(int w)
{
 switch (w)
  {
   case ARGB_TESTBARS:
    {
     for (int x=0;x<ARGB_MAX_X;x++)
      {
       int c = x * 16;

       if (!c)
        c = 1;
       else
        if (c > 255)
         c = 255;

       ARGB c1 = MakeARGB(0xFF,c,0,0);
       ARGB c2 = MakeARGB(0xFF,0,c,0);
       ARGB c3 = MakeARGB(0xFF,0,0,c);
       ARGB c4 = MakeARGB(0xFF,c,c,c);
      
       Argb.SetPixel(x,0,c1); Argb.SetPixel(x,1,c1);
       Argb.SetPixel(x,2,c2); Argb.SetPixel(x,3,c2);
       Argb.SetPixel(x,4,c3); Argb.SetPixel(x,5,c3);
       Argb.SetPixel(x,6,c4); Argb.SetPixel(x,7,c4);
      }
    }
    break;
   case ARGB_TESTRED:
    Argb.Fill(0xFFFF0000);
    break;
   case ARGB_TESTGREEN:
    Argb.Fill(0xFF00FF00);
    break;
   case ARGB_TESTBLUE:
    Argb.Fill(0xFF0000FF);
    break;
   case ARGB_TESTBLACK:
    Argb.Fill(0xFF000000);
    break;
   case ARGB_TESTDARK:
    Argb.Fill(0xFF010101);
    break;
   case ARGB_TESTGREY:
    Argb.Fill(0xFF202020);
    break;
   case ARGB_TESTWHITE:
    Argb.Fill(0xFFFFFFFF);
    break;
   case ARGB_TESTVERT:
    {
     Argb.Fill(0xFFFFFFFF);
     for (int x=0;x<ARGB_MAX_X;x+=2)
      for (int y=0;y<ARGB_MAX_Y;y++)
       Argb.SetPixel(x,y,0xFF000000);
    }
    break;
   case ARGB_TESTHORZ0:
    {
     Argb.Fill(0xFFFFFFFF);
     for (int x=0;x<ARGB_MAX_X;x++)
      for (int y=0;y<ARGB_MAX_Y;y+=2)
       Argb.SetPixel(x,y,0xFF000000);
    }
    break;
   case ARGB_TESTHORZ1:
    {
     Argb.Fill(0xFFFFFFFF);
     for (int x=0;x<ARGB_MAX_X;x++)
      for (int y=1;y<ARGB_MAX_Y;y+=2)
       Argb.SetPixel(x,y,0xFF000000);
    }
    break;
   case ARGB_TESTYELLOW:
    Argb.Fill(0xFFF802000);
    break;
   case ARGB_TESTDARKYELLOW:
    Argb.Fill(0xFFF200800);
    break;
  }
}
