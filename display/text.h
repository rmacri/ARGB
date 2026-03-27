#ifndef TEXT_H
#define TEXT_H

#define MAX_TEXT 64
 
class TextDisplay
{
 char   out_text[MAX_TEXT+1];
 ARGB   color;
 byte   step_ticks;
 byte   step_count;
 int    x_scroll;

 void   Tick();
 
 public:

 void Reset();
 void Update();
 void Set(const char * str);
 int  Active()         {return x_scroll != 32767;}
};


#endif
