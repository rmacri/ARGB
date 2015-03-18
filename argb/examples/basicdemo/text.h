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

 void SetSpeed(byte t)         {step_ticks = step_count = t;}

 void SetColor(ARGB c)         {color = c;}
 ARGB Color()                  {return color;}
};


#endif
