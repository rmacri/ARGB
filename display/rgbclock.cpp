#include <argb.h>
#include "rgbclock.h"

#define SETTIMEOUT (60*125)  // in  frame updates, minute at 125 refreshes/second

RGBClock::RGBClock()
         :cmode(0),
          clock24(0),
          sethour(0),
          setmin(0),
          settimeout(0)
{
}

static const char * ReadDigits(const char * p,byte & v)
{
 v = 0;

 if (*p >= '0' && *p <= '9')
  {
   v = *p - '0';
   p++;
   if (*p >= '0' && *p <= '9')
    {
     v = v*10 + *p - '0';
     p++;
    }

   // at this point 'o' should be a : or \0   
   if (*p==':')  // skip delimiter
    p++;
   else
    p = 0;
  }
 else
  p = 0;

 return p;
}

void RGBClock::Set(const char * p)
{
 byte clock_hour,clock_min,clock_sec;

 p = ReadDigits(p,clock_hour);
 if (p)
  {
   p = ReadDigits(p,clock_min);
   if (p)
    {
     // need at least hour/min, sec is optional
     ReadDigits(p,clock_sec);

     if ((clock_hour < 24) && (clock_min < 60) && (clock_sec < 60))
      {
       ARGB_SetTime((unsigned long)clock_hour * 3600 + clock_min * 60 + clock_sec);
       cmode = 0;
      }
    }
  }
}

// hour of evening (max 23)
#define NIGHT_START   18
// sleep is like night but we use nightmode colours
#define SLEEP_START   23
#define SLEEP_END     07
// hour of morning
#define NIGHT_END     8

#define NIGHT_ALPHA 0x0c
#define DAY_ALPHA   0x10
#define OFFBITCOLOR 0xFF000002
#define ONBITCOLOR  0xFF040404

// "night mode" with blue removed
const ARGB BaseColorsNight[] PROGMEM =
 {
  0xFFFF0000,
  0xFFE01000,
  0xFFD02000,
  0xFF804000,
  0xFF406000,
  0xFF008000,
  0xFF208000,
  0xFF406000,
  0xFF804000,
  0xFFD04000,
  0xFFE02000,
  0xFFFF1000
 };

ARGB BlendBaseColorsNight(byte color1,byte color2,byte blend,byte fade)
{
 return  BlendARGB(pgm_read_dword(&BaseColorsNight[color1 % 12]),
                   pgm_read_dword(&BaseColorsNight[color2 % 12]),
                   blend,
                   fade);
}

ARGB RGBClock::GetColor(byte disp_hour,byte disp_sec,int ms)
{
 byte  ci;
 byte  blend;
 byte fade;

 // in night/sleep mode - blend between colours every 5 seconds  
 if ((disp_hour < SLEEP_END) || (disp_hour >= SLEEP_START))
  {
   ci    = disp_sec / 5;   
   blend = (ms + (disp_sec - ci*5) * 1000) / 40 * 256 / 125;   
   fade  = NIGHT_ALPHA;
   ARGB_dark = 1;
   return BlendBaseColorsNight(ci,ci+1,blend,fade);
  }
 else
 if ((disp_hour < NIGHT_END) || (disp_hour >= NIGHT_START))
  {
   ci    = disp_sec / 5;   
   blend = (ms + (disp_sec - ci*5) * 1000) / 40 * 256 / 125;   
   fade  = NIGHT_ALPHA;
   ARGB_dark = 1;
   return BlendBaseColors(ci,ci+1,blend,fade);
  }
 else
  {
   // day mode blend between new colours every seconed
   ci    = disp_sec;
   blend = ms / 8 * 256 / 125;  // == ms / 1000.0 * 256
   fade  = DAY_ALPHA;
   ARGB_dark = 0;
   return BlendBaseColors(ci,ci+1,blend,fade);
  }
}


void RGBClock::DecodeTime(unsigned long tod,byte & disp_hour,byte & disp_min,byte & disp_sec)
{
 disp_hour = tod / 3600;
 disp_min  = (tod - 3600L * disp_hour) / 60;
 disp_sec  = tod - 3600L * disp_hour - 60 * disp_min;
}

static char * PutDigits(char * p,byte b,byte leading0)
{
 byte b10 = b / 10;
 
 if (leading0 || b10)
  *(p++) = b10 + '0';
 
 *(p++) = (b - b10*10) + '0';
 
 return p;
}

void RGBClock::FormatInto(char * buf)
{
 // format time for scrolling during animation
 // note: this is not invoked often
 byte disp_hour,disp_min,disp_sec;
 DecodeTime(ARGB_clock_tod,disp_hour,disp_min,disp_sec);

 byte pm = 0;
 if (disp_hour >= 12)
  {
   if (disp_hour >= 13)
    disp_hour -= 12;
   pm         = 1;
  }
 else
  if (disp_hour == 0)
   disp_hour = 12;

 // warning:buf 8 characters
 char * p = buf;

 p = PutDigits(p,disp_hour,clock24);
 *p++ = ':';
 p = PutDigits(p,disp_min,1);
 
 if (!clock24)
  *p++ = pm?'P':'A';
 *p++ = '\0';
}

void RGBClock::ShowTime()
{
 // NOTE: this assumes no interrupt will  mess up reading multibyte
 // values from the ISR, this should be occuring just after an interrupt
 // caused main loop to loop around
 byte disp_hour,disp_min,disp_sec,next_hour,next_min,next_sec;

 DecodeTime(ARGB_clock_tod,disp_hour,disp_min,disp_sec);
 DecodeTime((ARGB_clock_tod+1) % 86400L,next_hour,next_min,next_sec);
            
 // Fade to the next seconds colour, changing colour every update
 ARGB tc = GetColor(disp_hour,                       // day/night test
                    disp_sec,                        // colours cycle every 12
                    ARGB_clock_ms);                  // time within second

 byte  clock_y = 1;
 char  clock_x = -1;   // 12 hour mode only show 1 in first digit so save pixel

 byte pm = 0;

 if (!clock24)  
  {
   // 12 hour moed display
   if (disp_hour >= 12)
    {
     if (disp_hour >= 13)
      disp_hour -= 12;
     pm         = 1;
    }
   else
    if (disp_hour == 0)
     disp_hour = 12;
   
   if (next_hour >= 13)
    next_hour -= 12;
   else
    if (next_hour == 0)
     next_hour = 12;
  }
 else
  clock_x = 0;   // 24 hour mode need all 3 pixels

 // center for 3 panels

 byte panel_offset = (ARGB_PANELS == 3) ? 4 : 0;

 clock_x += panel_offset;
 
 // old/new scroll, from 0 (old) to 255 (new)
 // do the scroll within the half second before a change such that
 // the digits settle in place on the second.

 byte r = 0;
 if (ARGB_clock_ms >= 489)             // 489 = 1000-511
  r =  (ARGB_clock_ms - 489) / 2;      // count from 0 to 255

 // first hour digit:show 1 or blank in 12 hour mode
 if (!clock24)  
  Argb.BlendDigits((disp_hour >= 10) ? 1 : 255,
                   (next_hour >= 10) ? 1 : 255,
                   r,clock_x,clock_y,tc);
 else
  Argb.BlendDigits(disp_hour / 10,next_hour / 10,r,clock_x,clock_y,tc);
 
 // second hour digit  
 clock_x += 4;
 Argb.BlendDigits(disp_hour % 10,next_hour % 10,r,clock_x ,clock_y,tc);
 clock_x += 4;

 if (!clock24)
  {
   // colon
   Argb.DrawDigit(10            ,clock_x ,clock_y,tc);
   clock_x += 2;
  }

 // minutes digits
 Argb.BlendDigits(disp_min / 10, next_min / 10,r ,clock_x,clock_y,tc);    
 clock_x += 4;
 Argb.BlendDigits(disp_min % 10, next_min % 10,r ,clock_x,clock_y,tc);

 // colour for PM indicator and seconds bits
 tc  = ONBITCOLOR;

 if (pm)
  Argb.SetPixel(panel_offset,7,tc);

 // show seconds in binary but hide just before a scroll
 if (disp_sec < 59 || ARGB_clock_ms < 200)
  {
   byte s1 = disp_sec / 10;
   byte s0 = disp_sec % 10;
   char  i;

   // colour for "off" bits for seconds
   ARGB bg = OFFBITCOLOR;

   // first "digit", 3 bits for 0..5
   clock_x = 8 + panel_offset;
   for (i=2;i>=0;i--)
    {
     ARGB c = tc;
     if (!(s1 & (1<<i)))
      c = bg;
     Argb.SetPixel(clock_x++,7,c);
    }

   clock_x++;

   // second digit, 4 bits for 0..9
   for (i=3;i>=0;i--)
    {
     ARGB c = tc;
     if (!(s0 & (1<<i)))
      c = bg;
     Argb.SetPixel(clock_x++,7,c);
    }
  } // show seconds
}

void RGBClock::SetHour()
{
 ARGB tc = 0xFF008000;

 POINT clock_y = 1;
 POINT clock_x = 0;

 Argb.DrawDigit(11,clock_x,clock_y,tc); // H
 clock_x += 4;
 Argb.DrawDigit(10,clock_x,clock_y,tc);
 clock_x += 2;

 byte c;
 c = sethour / 10;
 Argb.DrawDigit(c,clock_x,clock_y,tc);
 clock_x += 4;
 c = sethour % 10;
 Argb.DrawDigit(c,clock_x,clock_y,tc);
 clock_x += 4;
}

void RGBClock::SetMin()
{
 ARGB tc = 0xFF008000;

 POINT clock_y = 1;
 POINT clock_x = 0;

 Argb.DrawDigit(12,clock_x,clock_y,tc);  // M
 clock_x += 4;
 Argb.DrawDigit(10,clock_x,clock_y,tc);
 clock_x += 2;

 byte c;
 c = setmin / 10;
 Argb.DrawDigit(c,clock_x,clock_y,tc);
 clock_x += 4;
 c = setmin % 10;
 Argb.DrawDigit(c,clock_x,clock_y,tc);
 clock_x += 4;
}

void RGBClock::SetFine()
{
 ARGB tc = 0xFF208000;

 POINT clock_y = 1;
 POINT clock_x = 0;

 Argb.DrawDigit(13,clock_x,clock_y,tc);  // F
 clock_x += 4;
 Argb.DrawDigit(10,clock_x,clock_y,tc);
 clock_x += 2;

 byte c;
 c = ARGB_fine_adj / 10;
 Argb.DrawDigit(c,clock_x,clock_y,tc);
 clock_x += 4;
 c = ARGB_fine_adj % 10;
 Argb.DrawDigit(c,clock_x,clock_y,tc);
 clock_x += 4;
}

void RGBClock::Update()
{
 Argb.Clear();

 switch (cmode)
  {
   case CLOCK_TIME:
    ShowTime();
    break;
   case CLOCK_SETHOUR:
    if (!--settimeout)
     cmode = CLOCK_TIME;
    else
     SetHour();
    break;
   case CLOCK_SETMIN:
    if (!--settimeout)
     cmode = CLOCK_TIME;
    else
     SetMin();
    break;
   case CLOCK_FINEADJ:
    if (!--settimeout)
     cmode = CLOCK_TIME;
    else
     SetFine();
    break;
  } 
}

void  RGBClock::NextMode()
{
 settimeout = SETTIMEOUT;

 // just set fine adjust value and pressed next mode button
 // we don't write on timeout but change in value is used in RAM
 if (cmode == CLOCK_FINEADJ)
  Argb.WriteFineAdjust();   
 
 cmode = (cmode+1) % CLOCK_MODECOUNT;
 
 if (cmode == CLOCK_SETHOUR)
  {
   sethour = ARGB_clock_tod / 3600;
   setmin  = (ARGB_clock_tod - 3600L * sethour) / 60;
  }
 else
  if (cmode == CLOCK_TIME)
   ARGB_SetTime((unsigned long)sethour * 3600 + setmin * 60);    
}

void RGBClock::IncValue()
{
 settimeout = SETTIMEOUT;

 if (cmode == CLOCK_SETHOUR)
  sethour = (sethour+1) % 24;
 else
  if (cmode == CLOCK_SETMIN)
   setmin = (setmin+1) % 60;
  else
   if (cmode == CLOCK_FINEADJ)
    {
     // note this takes effect immediately even if time set isn't completed
     if (ARGB_fine_adj < 99)
      ARGB_fine_adj++;
     else
      ARGB_fine_adj = 0;
    }
   else
    clock24 = !clock24;
}
