// 91 bytes

#include <avr/pgmspace.h>

const unsigned char usintab[] PROGMEM = 
 {
  0,   4,  9, 13, 18, 22, 27, 31,  35, 40, 44, 49, 53, 57, 62,
  66, 70, 75, 79, 83, 87, 91, 96, 100,104,108,112,116,120,124,
  128,131,135,139,143,146,150,153,157,160,164,167,171,174,177,
  180,183,186,190,192,195,198,201,204,206,209,211,214,216,219,
  221,223,225,227,229,231,233,235,236,238,240,241,243,244,245,
  246,247,248,249,250,251,252,253,253,254,254,254,255,255,255, 255
 };

int isin(int angle)
{
 while (angle<0) angle += 360;
 angle    %= 360;
 int sign  = 1;

 if (angle >= 180)
  {
   angle -= 180;
   sign   = -1;
  }

 if (angle > 90)
  angle = 180 - angle;

 return sign * (int)(pgm_read_byte(&usintab[angle]));
}

int icos(int angle)
{
 return isin(angle + 90);
}

/* Table generator:

#include <stdio.h>
#include <math.h>

main()
{
 const double M_PI = 3.14159;
 for (int i=0;i<=90;i++)
  {
   double a = double(i) / 360.0 * (2.0*M_PI);
   double s = sin(a);
   int si = s * 255 + 0.5;
   printf("%d,",si);
  }
}

*/
