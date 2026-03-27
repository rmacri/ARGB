#ifndef RGBCLOCK_H
#define RGBCLOCK_H

enum
 {
  CLOCK_TIME,
  CLOCK_SETHOUR,
  CLOCK_SETMIN,
  CLOCK_FINEADJ,
  CLOCK_MODECOUNT
 };

class RGBClock
{
 byte cmode;
 byte sethour;
 byte setmin;
 byte clock24;
 int  settimeout;

 // note: time is stored and counted in the ARGB library ISR

 void ShowTime();
 void SetHour();
 void SetMin();
 void SetFine();
 
 ARGB GetColor(byte hour,byte sec,int blend);

 public:

 RGBClock();

 void Set(const char * s);
 void Update();

 byte  Mode()      {return cmode;}
 void  NextMode();
 void  IncValue();

 void DecodeTime(unsigned long tod,byte & disp_hour,byte & disp_min,byte & disp_sec) ;
 void FormatInto(char*buf);  // 8 characters
 void Toggle24()       {clock24 = !clock24;}
};

#endif
