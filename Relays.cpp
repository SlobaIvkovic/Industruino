#include <Arduino.h>
#include "Relays.h"


/*#ifndef OFF_TIME
#define OFF_TIME 30
#endif
#ifndef ON_TIME
#define ON_TIME 1
#endif
*/

#define RELAY_1 5
#define RELAY_2 6

/*Relays::Relays()
{
  Indio.digitalMode(5, OUTPUT);
  Indio.digitalMode(6, OUTPUT);
  offTimeCounting = true;
  offTime = OFF_TIME;
  onTime = ON_TIME;
}
*/

Relays::Relays(int off, int on, bool onOFF) : offTime(off), onTime(on), offTimeCounting(onOFF)
{

}

/*void Relays::set(bool onOFF, int off, int on)
{
  offTimeCounting = onOFF;
  offTime = off;
  onTime = on;
}
*/

extern int minutes;
extern int seconds;


/*void Relays::get(bool &onOFF, int &off, int &on)
{
  onOFF = offTimeCounting;
  off = offTime;
  on = onTime;
}
*/

void Relays::manageRelays()
{
  if(offTimeCounting == true) // If off time is counted
  {
    if((int)minutes == offTime )      // Ako je dovoljno dugo relej iskljucen
    {
      Indio.digitalWrite(RELAY_1, HIGH); // turn on relays
      Indio.digitalWrite(RELAY_2, HIGH);
      offTimeCounting = false;  // from now on relay on time is measured
      minutes = 0;                  // reset minute conter
      seconds = 0; // reset seconds counter so that first minute of on time starts from 0 seconds
      
    }
  
  }
    else if(minutes == onTime)  // If on time is measured and relay was on enough
    {
      Indio.digitalWrite(RELAY_1, LOW);
      Indio.digitalWrite(RELAY_2, LOW);
      offTimeCounting = true;  // from now on off time is measured
      minutes = 0;                  // reset minute counter
      seconds = 0; // reset seconds counter so tha first minute of off time starts from 0 seconds
    }
}
