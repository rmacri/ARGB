#ifndef BITSLICE_H
#define BITSLICE_H


class BitSlice
{
 byte   buffer[ARGB_MAX_X];
 ARGB   color;
 byte   text_fade;
 byte   active;
 
 public:

 void  Reset();
 void  Update();
 void  AddAndShift(byte c);
 void  SetActive()             {active=1;}
 void  SetInactive()           {active=0;}
 
 // not externally inactive until fade completes
 byte  Active()                {return active || text_fade;}
};


#endif
