#include <argb.h>
#include <isin.h>
#include "rgbclock.h"
#include "tempo.h"
#include "button.h"
#include "effect.h"
#include "text.h"
#include "bitslice.h"
#include "testpattern.h"

RGBClock      clock;
TextDisplay   textdisplay;
Button        btn_mode;
Button        btn_value;
BitSlice      bitslice;

HatchDisplay  fxhatch;
TunnelDisplay fxtunnel;
SpinFade      fxspinfade;
BoxBeat       fxboxbeat;
Wave          fxwave;
Lissajous     fxlissajous;

#define EFFECT_COUNT 6
EffectBase * const EffectArray[EFFECT_COUNT] PROGMEM =
 {
  &fxhatch,
  &fxtunnel,
  &fxspinfade,
  &fxboxbeat,
  &fxwave,
  &fxlissajous  
 };

EffectBase * GetEffect(byte w)
{
 return (EffectBase*)pgm_read_word(EffectArray+w);
}

byte HexToByte(byte h)
{
 if (h >= '0' && h <= '9')
  return h - '0';
 else
  if (h >= 'A' && h <= 'F')
   return h - 'A' + 10;
  else
   if (h >= 'a' && h <= 'f')
    return h - 'a' + 10;
   else
    return 0;
}

void setup()
{
 Argb.init();
 tempotracker.Reset();
 for (int i=0;i<EFFECT_COUNT;i++)
  GetEffect(i)->Reset(1);

 textdisplay.Reset();
 bitslice.Reset();
 Serial.begin(9600);
 Serial.println(F("RGB Display\n"));

 // pin numbers for button debouncer
 btn_mode.Setup(3);
 btn_value.Setup(2);
}

enum
 {
  MODE_CLOCK,
  MODE_ANIMATE,
  MODE_SERIALRGB,
  MODE_SERIALRGBIDLE,
  MODE_TEST
 };

byte main_mode = MODE_CLOCK;
byte test_mode = 0;

static byte effect            = 0;
static byte effect_mode_lock  = 0;
static byte last_display_min  = 0; // animation minute update
static byte clock_holdoff     = 0;

#define  TIME_UPDATE_DISABLE 99

byte CheckSetLastMinute(byte holdoff = 0)
{
 if (last_display_min != TIME_UPDATE_DISABLE)
  {
   byte new_display_min = (ARGB_clock_tod / 60) % 60;

   if (holdoff)
    {
     clock_holdoff = holdoff;
     last_display_min = new_display_min;
    }
   else
    if (new_display_min != last_display_min)
     {
      last_display_min = new_display_min;

      if (clock_holdoff)
       clock_holdoff--;
      else
       return 1;
     }
  }

 return 0;
}

static inline void DisableMinuteUpdates()
{
 last_display_min = TIME_UPDATE_DISABLE;
}

static inline void EnableMinuteUpdates()
{
 if (last_display_min == TIME_UPDATE_DISABLE)
  {
   last_display_min = 0;
   CheckSetLastMinute(1);   // delay before first time update
  }
}

void SetRandomSeed()
{
 randomSeed((unsigned long)ARGB_clock_ms * 100 + ARGB_clock_tod + ARGB_adcdata[0]);
}

void SetEffectLock(byte new_lock,int quiet = 0)
{
 effect_mode_lock = new_lock;
 if (!quiet)
  textdisplay.Set(effect_mode_lock?"Lock":"Unlock");

 // prevent itme appearing immediately over animation
 CheckSetLastMinute(1);
}

void SetEffect(byte new_effect)
{
 effect = new_effect % EFFECT_COUNT;
 GetEffect(effect)->Reset(0);
}

void SendSerialHexByte(byte b)
{
 byte bh = b >> 4;
 if (bh <= 9)
  bh += '0';
 else
  bh += 'A' - 10;
 Serial.write(bh);
 
 b &= 0x0f;
 if (b <= 9)
  b += '0';
 else
  b += 'A' - 10;
 Serial.write(b);
}
 
void HandleSerial()
{
 static char in_text[MAX_TEXT+1];
 static int  in_pos = 0; 
 
 static char   serrgb_color;
 static char   serrgb_row;
 static byte * serrgb_ptr;

 while (Serial.available())
  {
   char ch = Serial.read();
   
   if (main_mode == MODE_SERIALRGB)
    {
     *(serrgb_ptr++) = ch;
     if (!--serrgb_color)
      {
       serrgb_row--;
       if (!serrgb_row)
        {
         main_mode = MODE_SERIALRGBIDLE;
         Argb.ScrollLeft(1);
        }
       else
        {
         serrgb_color = 3;
         serrgb_ptr-=3;
         serrgb_ptr += ARGB_MAX_X * 3;
        }
      }         
    }
   else
    if ((ch == '\0') ||
        (ch == '\n') ||
        (ch == '\r') ||
        (in_pos == MAX_TEXT))
     {
      // line terminator

      if (in_pos)
       {
        in_text[in_pos] = '\0';

        switch (in_text[0])
         {
          // animation mode:          
          case 'a':
           main_mode = MODE_ANIMATE;
           EnableMinuteUpdates();
           break;           
          case 'r':
           SendSerialHexByte(ARGB_adcdata[7]);
           Serial.write('\n');                             
           break;
          case 'l':
           SetEffectLock(!effect_mode_lock);
           break;
          case 'n':
           if (main_mode == MODE_ANIMATE)
            SetEffect(effect + 1);
           break;

           // clock mode:           
          case 'c':
           main_mode = MODE_CLOCK;
           break;
          case 'h':
           clock.Toggle24();
           break;
          case 'd': // aDjust fine
           main_mode = MODE_CLOCK;
           ARGB_fine_adj = (HexToByte(in_text[1]) << 4) | HexToByte(in_text[2]);
           Argb.WriteFineAdjust();
           break;

           // raw RGB mode
          case '#':
           serrgb_color = 3;
           serrgb_row   = ARGB_MAX_Y;
           main_mode    = MODE_SERIALRGB;
           serrgb_ptr   = framebuffer_1;    // top right pixel
           break;
          case 't':
           if (main_mode != MODE_TEST)
            main_mode = MODE_TEST;
           else
            test_mode = (test_mode + 1) % ARGB_TESTCOUNT;

           // set test up now, it will remain in alt buffer
           Argb.SelectAltBuffer();
           ShowTest(test_mode);
           Argb.SelectMainBuffer();
           break;
          case 'm':
           bitslice.Reset();
           textdisplay.Set(in_text+1);
           break;
          case '|': // vertical bit slice text transfer mode
           {
            DisableMinuteUpdates();
            textdisplay.Reset();
            if (!bitslice.Active())
             bitslice.SetActive();
            
            byte c = (HexToByte(in_text[1]) << 4) | HexToByte(in_text[2]);             
            bitslice.AddAndShift(c);
           }
           break;
          case '$':
           EnableMinuteUpdates();
           textdisplay.Reset();
           bitslice.SetInactive();
           break;
          default:
           if (isdigit(in_text[0]))
            {
             clock.Set(in_text);
             main_mode = MODE_CLOCK;
             SetRandomSeed();
            }
         }
       
        in_pos = 0;
       }
     }
    else
     in_text[in_pos++] = ch;
  }
}

void loop()
{
 // this loops every frame update (see bottom of function)

 // detect mode changes
 static byte last_main_mode = ~0;
 byte   changed_mode        = last_main_mode != main_mode;
 last_main_mode             = main_mode;

 // reset night mode set by clock
 if (changed_mode)
  ARGB_dark = 0;

 if (main_mode != MODE_SERIALRGB)
  {
   // tempotracker for the effects
   unsigned int beatbits  = tempotracker.TickToBeat();   // beat trigger bits
   unsigned int beatcount = tempotracker.BeatCount();    // 64th note within bar

   // read and debounce buttons
   byte b_mode  = btn_mode.Update();
   byte b_value = btn_value.Update();

   if (!effect_mode_lock && (beatbits & BEAT_4BAR) && random(100) > 30)
    SetEffect(effect + random(4));

   switch (main_mode)
    {
     case MODE_CLOCK:
      Argb.SelectAltBuffer();
      clock.Update();
      Argb.SelectMainBuffer();

      if (clock.Mode() != CLOCK_TIME)
       {
        // setting hour or minute
        if (b_mode == DEBOUNCE_DOWN)
         {
          // respond to mode button down, dont send button up
          clock.NextMode();
          btn_mode.StopRepeat();   // no up message
         }
        else
         if ((b_value == DEBOUNCE_DOWN) ||
             (b_value == DEBOUNCE_REPEAT))
          clock.IncValue();
       }
      else
       {
        // normal - clock displaying time
        if (b_mode == DEBOUNCE_UP)
         {
          main_mode = MODE_TEST;
          // leave test in the alt buffer
          test_mode = 0;
          Argb.SelectAltBuffer();
          ShowTest(test_mode);
          Argb.SelectMainBuffer();
         }
        else
         if (b_mode == DEBOUNCE_REPEAT)
          {
           btn_mode.StopRepeat();
           clock.NextMode();  // go into setup
          }
         else
          if (b_value == DEBOUNCE_DOWN)
           clock.IncValue();   // toggle display mode
       }
      break;
     case MODE_TEST:
      {
       if (b_mode == DEBOUNCE_UP)
        {
         // prevent an immediate time update
         CheckSetLastMinute(1);
         main_mode = MODE_ANIMATE;
        }
       else
        if (b_value == DEBOUNCE_DOWN)
         {
          test_mode = (test_mode + 1) % ARGB_TESTCOUNT;
          // set test up now, it will remain in alt buffer
          Argb.SelectAltBuffer();
          ShowTest(test_mode);
          Argb.SelectMainBuffer();
         }
      }
      break;
     case MODE_ANIMATE:
      {
       if (beatbits)
        {
         // animations only update on a multiple of the beat
         // otherwise alt buffer left unchanged
         Argb.SelectAltBuffer();
         GetEffect(effect)->Beat(beatcount,beatbits);
         Argb.SelectMainBuffer();
        }

       if (b_mode == DEBOUNCE_UP)
        main_mode = MODE_CLOCK;

       if (b_value == DEBOUNCE_UP)
        {
         SetEffect(effect + 1);
         if (!effect_mode_lock)
          SetEffectLock(1,1);
        }
       else
        if (b_value == DEBOUNCE_REPEAT)
         {
          btn_value.StopRepeat();
          SetEffectLock(!effect_mode_lock);
         }
      }
      break;
    } // main_mode case

   if (tempotracker.ticks & 1)
    {
     // every 16ms (63 frames per second)

     if (!bitslice.Active())
      {
       if (main_mode == MODE_ANIMATE &&
           CheckSetLastMinute() &&
           !textdisplay.Active())
        {
         char fmttime[8];
         clock.FormatInto(fmttime);
         textdisplay.Set(fmttime);
        }

       // this always copys Alt to main buffer
       textdisplay.Update();
      }
     else
      bitslice.Update();
    } // every alternate interrupt
  } // not serialRGB which directly updates main buffer

 // above can take time depending on complexity. So instead of
 // a fixed delay we wait for the interrupt to set ARGB_user_frame

 do
  HandleSerial();
 while (!ARGB_user_frame);

 // an interrupt has just occured, set up so we detect next
 // even if we overrun in code above.
 ARGB_user_frame = 0;
}
