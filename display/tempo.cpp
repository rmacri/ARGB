#include <argb.h>
#include "tempo.h"

TempoTracker  tempotracker;

void TempoTracker::Reset()
{
 ticks      = 0;
 bar_ticks  = (unsigned long)(BEATS_PER_BAR) * 1000 * 60 / (TICK_INTERVAL * TEMPO_BPM);
 bar_count  = 0;

 // prime so first tick is a beat and a bar
 tick_count = bar_ticks-1;
 last_beat  = BEAT_RESOLUTION-1;
}
  
unsigned int TempoTracker::TickToBeat()
{
 // Timing explanation - assumes 128BPM which will vary when audio triggered
 //
 // main loops every 10ms. This gives us about 3 10ms ticks per 64th note
 // at 128 BMP. Work out as follows:
 //  128BPM = 1000 * 60 / 128 = 468 ms per beat (1/4 note)
 //  16 64th notes per beat (4/4 time) = 29ms per 64th note
 //  29/10 = about 3 ticks per 64th note. 
 //
 // The fractional error evens out since we time over a whole bar.


 ticks++;
 tick_count++;

 // flags for bars set if its a new bar
 unsigned int new_bar = 0;

 if (tick_count >= bar_ticks)
  {
   tick_count = 0;

   new_bar    = BEAT_BAR;

   if (bar_count)  // only after the first bar has occured, not at start
    {
     if (!(bar_count & 0x1))
      new_bar |= BEAT_2BAR;
     if (!(bar_count & 0x3))
      new_bar |= BEAT_4BAR;
     if (!(bar_count & 0x7))
      new_bar |= BEAT_8BAR;
    }

   bar_count++;
  }

 unsigned int new_beat = tick_count * BEAT_RESOLUTION / bar_ticks;
 unsigned int beatbits = (new_beat ^ last_beat) | new_bar;
 last_beat             = new_beat;

 return beatbits;  // bits set according to beat, bar, 1/8th note etc
}
