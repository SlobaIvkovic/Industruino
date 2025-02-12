//#include <Arduino.h>
#include <avdweb_SAMDtimer.h>
#include <Indio.h>
#include <Wire.h>
#include <UC1701.h>
#include "Relays.h"

#define OFF_TIME 30 // change these lines to obtain different on/off times
#define ON_TIME 1

#define RELAY_1 5
#define RELAY_2 6
#define RELAY_3 7
#define RELAY_4 8

#define INPUT_1 4
#define INPUT_2 3
#define RELAY_5 2

bool relay5ON;

#define RELAY_1_SCHEDULE 60*60*6                        //60*60*4 real time 4h, 60*5 5mins for testing, last requirement 6h 60*60*6
#define BEFORE           30                             // relay 2 fires before relay1 this definition defines how long before it fires
#define RELAY_2_BEFORE   RELAY_1_SCHEDULE - BEFORE
#define AFTER            30                              // relay 2 stays on after relay 1 fires, this definition defines for how long it stays on
#define RELAY_2_ON_TIME  60                              // on it's schedule relay 2 stays on for 1min
#define RELAY_2_SCHEDULE 60*3                            //60*30 real time 30min off 1min on, 60*3 3mins for testing
#define RELAY_1_ON_TIME  2000                            // defined in miliseconds

volatile unsigned int r2Schedule;
bool r2ON1;         // if relay 2 is on due to relay 1 dependance
bool r2ON2;         // if relay 2 is on due to it's schedule
           
volatile bool r1sequence;             // execute relay 1 fire sequence
volatile bool r2sequence;             // execute relay 2 fire sequence

volatile unsigned int r1Schedule;
volatile unsigned int r2Before;
 unsigned int before;                   // it was volatile
 unsigned int after;                    // it was volatile
 unsigned int beforeAfter;
volatile unsigned int r2OnTime;
unsigned int secondsPrevious;
volatile bool relay2firedBefore;

// time betwen relay3 and 4 turn off
#define RELAY34_MINS 1
// CH1 reads sensor
#define ANALOG_MA_CHANNEL 1
// treshold temperature for driving relays3 and 4
#define TRESHOLD
// counting ON and OFF minutes and seconds of relays1 and 2
volatile int minutes;
volatile int seconds;
// minutes are reset in Relays.manageRelays
// seconds are reset in ISR
bool relays34OnOff;
bool relays12OnOff;
// counting time betwen relay3 and 4 turn off
int relays34minutes;
int relays34seconds;
// holds analog mili Amp reading on channel 1
float miliAmps;
float temp;
unsigned int miliAmpsInt;
// lcd driver object
static UC1701 lcd;

bool readingChanged = false;
float miliAmpsPrevious = 0;

//Relays Rels = Relays(OFF_TIME, ON_TIME, false);

/*
 If you are using an IND.I/O unit, please be aware that by default, the 'Serial' port (D0/D1) is connected to the IND.I/O's RS485 port.
 The current IND.I/O baseboards have a hardware switch to connect/disconnect the RS485 port to this Serial port (D0/D1).
 To use the 'Serial' port for RS232 or GSM, this switch should be in the upward position (away from the RS485 terminals).
*/

/**************************************************************
 *
 * TinyGSM Getting Started guide:
 *   http://tiny.cc/tiny-gsm-readme
 *
 **************************************************************/
#define TINY_GSM_MODEM_SIM800

#include <TinyGsmClient.h>
#include <ThingSpeak.h>

// Necesary to connect to thingspeak channel
unsigned long myChannelNumber = 714583;   // Change this to your channel number
const char * myWriteAPIKey = "7W8OMBW99RK2VESI"; // Change this to your write api key
// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "live.vodafone.com";          // change this to apn provided by your gsm network provider
const char user[] = "";     // change this with username provided by your gsm network provider
const char pass[] = "";         // change this with password provided by your gsm network provider
const int pwr_pin = 6;

// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1 //


TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

int thingSpeakTimeout = 20;

void ISR_timer5(struct tc_module *const module_inst)
{
  seconds++;
  
  r2Before++;  // count time when r2 should fire with respect to r1
  r2Schedule++;
  if(r2Before == RELAY_2_BEFORE)
  {
    r1sequence = true;
    }
  if(r2Schedule == RELAY_2_SCHEDULE)
  {
    r2sequence = true;
    }  
  
  
  relays34seconds++;
  thingSpeakTimeout++;
  if(seconds == 60)
  {
    minutes++;
    seconds = 0;  
  }
  if(relays34seconds == 60) // count seconds for measuring time between relay 3 and 4 turn off
  {                         // relay 4 should turn off 1min after relay 3, relay34seconds is updated every second
                            // once it reaches 60 relays34minutes is incremented, both variables are reset when relays 3 and 4 fire in the main
    relays34minutes++;
    relays34seconds = 0;  
  }
}

SAMDtimer timer5_1Hz = SAMDtimer(5, ISR_timer5, 1e6);


void setup() {
  // RELAYS 1 and 2 TIMES SETUP
  r2Before = 0;         // counts until r2 should fire with respect to r1
  secondsPrevious = 0;  // for measuring 1s time pass
  before = BEFORE;      // decremented in r1 sequence of the code, reset when relay1 fires
  after = 0;            // set it to zero, this variable is set after relay 1 turns off        
  r2OnTime = 0;         //  set it to zero this variable is set when relay 2 is turned on due to it's schedule
  r2Schedule = 0;       // this one increments every second in ISR, when it reaches RELAY_2_SCHEDULE it sets flag for r2sequence execution
  r2ON1 = false;        // initialy relay 2 is off for both reasons, dependance of relay 1 and it's schedule
  r2ON2 = false;
  relay2firedBefore = false;
  beforeAfter = 0;

  
  relays34OnOff = false;
  relays12OnOff = true;
  miliAmps = 0;
  miliAmpsInt = 0;
  
  relay5ON = false;

  pinMode(13, OUTPUT);  // lcd backlight pin
  analogWrite(13, 240); // backlight full intensity 0 full brightness, 255 off
  lcd.begin();
  delay(500);
  lcd.setCursor(40, 3);
  lcd.print("Wellcome");
  delay(1000);


  Indio.digitalMode(INPUT_1, INPUT);
  Indio.digitalMode(INPUT_2, INPUT);
  Indio.digitalMode(RELAY_5, OUTPUT);
  Indio.digitalMode(RELAY_1, OUTPUT);
  Indio.digitalMode(RELAY_2, OUTPUT);
  Indio.digitalMode(RELAY_3, OUTPUT);
  Indio.digitalMode(RELAY_4, OUTPUT);
  Indio.digitalWrite(RELAY_1, LOW); // start with rels 1 and 2 ON
  Indio.digitalWrite(RELAY_2, LOW); //
  Indio.digitalWrite(RELAY_3, LOW);
  Indio.digitalWrite(RELAY_4, LOW);

  Indio.setADCResolution(16);
  Indio.analogReadMode(ANALOG_MA_CHANNEL, mA);
  
  minutes = 0;
  seconds = 0;

   //turn on modem with 1 second pulse on D6. 
  pinMode(pwr_pin, OUTPUT);
  digitalWrite(pwr_pin, HIGH);
  delay(1000);
  digitalWrite(pwr_pin, LOW);
  
  // Set console baud rate
  SerialUSB.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialUSB.println("Initializing modem...");
  modem.restart();
  modem.simUnlock("0000");   // 3939

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");
  ThingSpeak.begin(client);  // Initialize ThingSpeak

}

void loop() {

// RELAY 1 AND 2 CONTROL BLOCK
/****************************************************************************/
   if(1) // implement switch condition also, replace 1 with switch condition
   {
      if(r1sequence) // if it is time to execute relay 1 sequence,
      {
        if(relay2firedBefore == false)
        {
          Indio.digitalWrite(RELAY_2, HIGH);  // first fire relay 2
          relay2firedBefore = !relay2firedBefore;
          beforeAfter = BEFORE+AFTER;
          r2ON1 = true;   
        }
   
        /******************************************************************/
        // count exactly one second, before variable is set in setup section
        if((seconds - secondsPrevious) >= 1)
        {
          secondsPrevious = seconds;
          before--;
        }
        if(before == 0)                     // when downcount reaches 0, before equals 0
        {
          Indio.digitalWrite(RELAY_1, HIGH); // fire r1
          delay(RELAY_1_ON_TIME);                       // wait 1s or 3s depending on Project requirements
          Indio.digitalWrite(RELAY_1, LOW);  // r1 off
                             // set it here to true, because now after starts to measure, not when relay 2 fires
          after = AFTER;                     // start counting time for turning r2 off
          r2Before = 0;                      // start counting for r1 schedule again
          r1sequence = false;                // to avoid firing r1 again,
          before = BEFORE;                   // reset before for next r1 sequence
        }
    }
   }    
  
  if(r2sequence)
  {
    Indio.digitalWrite(RELAY_2, HIGH);
    r2ON2 = true;
    r2sequence = false;
    r2OnTime = RELAY_2_ON_TIME;
    }
   
    // NOW check if both conditions for turning r2 off are satisfied
    if(r2ON1)
    {
      if((seconds - secondsPrevious) >= 1)
      {
        secondsPrevious = seconds;
        beforeAfter--;
        if(beforeAfter == 0)
        {
          r2ON1 = false;
        }
      }
    }  

      if(r2ON2)
      {
        if((seconds - secondsPrevious) >= 1)
        {
          secondsPrevious = seconds;
          r2OnTime--;
          if(r2OnTime == 0)
          r2ON2 = false;
        }
      }  

      // If both conditons are satisfied;
      if((!r2ON1) && (!r2ON2))
      {
        Indio.digitalWrite(RELAY_2, LOW);
        relay2firedBefore = false;
        }
 
    
// END OF RELAY 1 AND 2 CONTROL BLOCK
/***************************************************************************************/
      

// TEMPERATURE AND RELAY 3 AND 4 CONTROL BLOCK
/***************************************************************************************/
     
  miliAmps = Indio.analogRead(ANALOG_MA_CHANNEL);  // Read current value
  temp = 12.5 * miliAmps - 100;                    // translate current to temperature 

    // print reading on LCD
    lcd.setCursor(40, 3);
    lcd.print(temp, 1);
    lcd.print(" C");

  //miliAmpsInt = Indio.analogRead(ANALOG_MA_CHANNEL);
  if(temp < 33.0)// if temperature falls below 33, fire relays
  {
    Indio.digitalWrite(RELAY_3, HIGH);
    Indio.digitalWrite(RELAY_4, HIGH);
    relays34OnOff = true;
    relays34minutes = 0;
    relays34seconds = 0; // start cunting;  
  }
  else if(temp > 35.0) // if temperature goes above 35 turn rellays off
  {
    Indio.digitalWrite(RELAY_3, LOW);   // relay 3 off
    if(relays34minutes == RELAY34_MINS) // wait 1 minute then turn relay 4 off
    {
      Indio.digitalWrite(RELAY_4, LOW);
    }  
  }
// END OF TEMPERATURE AND RELAYS 3 AND 4 PORTION
/********************************************************************************/

// THING SPEAK PORTION OF MAIN LOOP
/********************************************************************************/
  
  if(thingSpeakTimeout >= 20)
  {
    int x = ThingSpeak.writeField(myChannelNumber, 1, temp, myWriteAPIKey);
    thingSpeakTimeout = 0;
    ThingSpeak.writeField(myChannelNumber, 1, temp, myWriteAPIKey);
  } 
// END OF THING SPEAK PORTION
/********************************************************************************/

if(Indio.digitalRead(INPUT_1))
{
 if(relay5ON == false)
 {
    Indio.digitalWrite(RELAY_5, HIGH);
    relay5ON = true;
  }  
}
if(Indio.digitalRead(INPUT_2))
{
  if(relay5ON == true)
  {
      Indio.digitalWrite(RELAY_5, LOW);
      relay5ON = false;
    }  
}

}
