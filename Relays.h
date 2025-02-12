#ifndef RELAYS_H
#define RELAYS_H

#include <Indio.h>
//#include <Arduino.h>

//#define OFF_TIME 1
//#define ON_TIME 1



class Relays
{
  private:
    bool offTimeCounting;
    int offTime;
    int onTime;
  public:
//    Relays();
    Relays(int off, int on, bool onOFF = true);
 //   void set(bool, int, int);
 //   void get(bool &, int &, int &);
    void manageRelays();
    
    
};

#endif //RELAYS_H
