#include <Arduino.h>
#include "button.h"

void Button::Setup(unsigned char use_pin)
{
 pin        = use_pin;
 pinMode(pin,INPUT_PULLUP);
 statecount = 0;
}

char Button::Update()
{
 // invoke periodically, change DEBOUNCE_TICKS if not 10ms
 unsigned char current = !digitalRead(pin);               // active low
 unsigned char counter = statecount & ~DEBOUNCE_STATEMASK;
 unsigned char state   = (statecount & DEBOUNCE_STATEMASK) >> DEBOUNCE_STATESHIFT;

 if (counter)
  {
   // counting down
   counter--;
   statecount = (statecount & DEBOUNCE_STATEMASK) | counter;

   if (!state)
    {
     // doing the initial down debounce
     if (!counter)
      {
       // debounce just completed. If still down return event
       // otherwise stateocunt is already zeroed and we're idle
       if (current)
        {
         // set up initial repeat delay and return down event
         statecount = DEBOUNCE_STATEMASK | DEBOUNCE_REPEATDELAY;
         return DEBOUNCE_DOWN;
        }
      }
    }
   else
    {
     // currently in down state.
     // if button goes up reset immediatey, don't need to debounce
     // since down check will handle any up bounces
     if (!current)
      {
       statecount = 0;
       return DEBOUNCE_UP;
      }
     
     if (!counter)
      {
       // repeat event timeout
       statecount = DEBOUNCE_STATEMASK | DEBOUNCE_REPEATINT;
       return DEBOUNCE_REPEAT;
      }
    }
  }
 else
  {
   // counter currently 0
   if (current && !state)              // button just gone down
    statecount = DEBOUNCE_TICKS;       // set up debounce counter
   else
    if (!current && state)             // button gone up after being down
     {
      statecount = 0;                   // clear
      return DEBOUNCE_UP_NOREPEAT;      // button released in norepeat mode
     }
  }

 return DEBOUNCE_NOCHANGE;
}
