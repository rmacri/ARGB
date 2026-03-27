#ifndef EFFECT_H
#define EFFECT_H

class EffectBase
{
 protected:

 static ARGB  colors[4];
 static byte  fade_level;
 static byte  xoffset;
 static byte  yoffset;
 static byte  sub_mode;
 static byte  half_beats;  // some effects can double their rate

 public:

 virtual void Reset(int first)              {}
 virtual void Beat(unsigned int beatcount,       // 64th notes within bar
                   unsigned int beatbits)   {}   // triggered beat timing

 byte  FastFade()     {return 0x40;}
};

class HatchDisplay : public EffectBase
{
 byte  xdelta,ydelta;

 void DrawHatch(byte w,ARGB color1,ARGB color2,byte xoffset,byte yoffset);
 void PickColors();

 public:

 void Reset(int first);
 void Beat(unsigned int beatcount,unsigned int beatbits);
};

class TunnelDisplay : public EffectBase
{
 byte counter;
 byte direction;

 public:

 void Reset(int first);
 void Beat(unsigned int beatcount,unsigned int beatbits);
};

class SpinFade : public EffectBase
{
 public:

 void Reset(int first);
 void Beat(unsigned int beatcount,unsigned int beatbits);
};

class BoxBeat : public EffectBase
{
 public:

 void Reset(int first);
 void Beat(unsigned int beatcount,unsigned int beatbits);
};

class Wave : public EffectBase
{
 public:

 void Reset(int first);
 void Beat(unsigned int beatcount,unsigned int beatbits);
};

class Lissajous : public EffectBase
{
 int angle1,angle2; // degrees * 100
 int delta1,delta2;
 int incdelta;

 public:

 void Reset(int first);
 void Beat(unsigned int beatcount,unsigned int beatbits);
};

#endif
