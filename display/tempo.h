#ifndef TEMPO_H
#define TEMPO_H

//
// The beat tracker provides pulses for beats (BEAT_4) which
// will eventually will be detected from the analog input.
//
// It uses these to generate pulses ranging from 64th notes up to
// 8 bars (4 beats a bar) which the effects can use as triggers.

enum
 {
  BEAT_64      = 1,
  BEAT_32      = 2,
  BEAT_16      = 4,
  BEAT_8       = 8,
  BEAT_4       =16,
  BEAT_2       =32,
  BEAT_BAR     =64,
  BEAT_2BAR    =128,
  BEAT_4BAR    =256,
  BEAT_8BAR    =512
 };

#define BEAT_RESOLUTION 64  // 64th notes
#define BEATS_PER_BAR    4  // 4/4 time (for house music)
#define TEMPO_BPM      120  // initial tempo beats per minute

// we get updated every frame, get interval in ms for our calcs
#define TICK_INTERVAL  (1000/ARGB_FRAMERATE)

class TempoTracker
{

 public:

 unsigned int ticks;        // count timing ticks
 unsigned int bar_ticks;    // # of ticks for a bar
 unsigned int tick_count;   // current tick count within bar
 unsigned int bar_count;    // number of bars elapsed
 unsigned int last_beat;    // beat count (64th note counter)
 
 void         Reset();
 unsigned int TickToBeat(); // timing update, called every TICK_INTERVAL
 unsigned int BeatCount()   {return last_beat | ((bar_count-1) << 6);}
};

extern TempoTracker  tempotracker;

#endif
