/* Wspr RTC with oled and Txing on Timer1 IRQ
  Anthony LE CREN F4GOH@orange.fr
  Created 16/7/2021
  need to test wsprencode with it
*/

#include <SPI.h>
#include <EEPROM.h>
#include <DS3232RTC.h>
#include <Wire.h>
#include <Time.h>
#include "SSD1306AsciiWire.h"
#include <stdio.h>

//https://github.com/khoih-prog/TimerInterrupt
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0
#define USE_TIMER_1     true
#define USE_TIMER_2     false
#define USE_TIMER_3     false
#define USE_TIMER_4     false
#define USE_TIMER_5     false
#include "TimerInterrupt.h"

#define LED 8
#define W_CLK 13
#define FQ_UD 10
#define RESET 9
#define GAIN 6  //pwm output pinout to adjust gain (mos polarization)
#define RTC_1HZ_PIN 2
#define debugDDS //enable debug DDS with some key commands as 0,1,2,3 on serial monitor (see debug function)


//#define FREQUENCY 475700  //630m = 474200+1500
//#define FREQUENCY 3570100  //80m
//#define FREQUENCY 7040100
//#define FREQUENCY 10140170  //30m
#define FREQUENCY 14097100   //dial 14.0956

#define CALL "F4GOH"        //callsign
#define LOCATOR "JN07"
#define DBM 20              //power in dbm
#define PC_BAUD_RATE  115200
#define PWM_GAIN 120      //continous voltage ctrl for mosfet R3=1K and R4=4.7K
#define WSPR_TONE_SPACING       1.4548
#define WSPR_DELAY              682
#define TIMER1_INTERVAL_MS      WSPR_DELAY

long factor = +100;    //adjust frequency to wspr band
int gain = PWM_GAIN;

int wsprSymb[] = {3, 3, 0, 0, 0, 2, 0, 2, 1, 2, 0, 2, 3, 3, 1, 0, 2, 0, 3, 0, 0, 1, 2, 1, 1, 3, 1, 0, 2, 0, 0, 2, 0, 2, 3, 2, 0, 1, 2, 3, 0, 0, 0, 0,
                  2, 2, 3, 0, 1, 3, 2, 0, 3, 3, 2, 3, 2, 2, 2, 1, 3, 0, 1, 0, 2, 0, 2, 1, 1, 2, 1, 0, 3, 2, 1, 2, 3, 0, 0, 1, 2, 0, 1, 0, 1, 3, 0, 0,
                  0, 1, 3, 2, 1, 2, 1, 2, 2, 2, 3, 0, 0, 2, 2, 2, 3, 2, 0, 1, 2, 0, 3, 3, 1, 2, 3, 3, 0, 2, 1, 3, 0, 3, 2, 2, 0, 3, 3, 1, 2, 0, 0, 0,
                  2, 1, 0, 1, 2, 0, 3, 3, 2, 2, 0, 2, 2, 2, 2, 1, 3, 2, 1, 0, 1, 1, 2, 0, 0, 3, 1, 2, 2, 2
                 };

int tableMinutes[] = {0, 2, 4, 10, 12, 14, 20, 22, 24, 30, 32, 34, 40, 42, 44, 50, 52, 54}; //minutes to transmit WSPR

enum ddsModel {
  AD9850,
  AD9851,
};

ddsModel ddsSelect = AD9850; //choose dds model

int secPrec = 0;
volatile int ptrElement = 0;

tmElements_t tm;

SSD1306AsciiWire oled;  //afficheur oled;


void setup() {

  Serial.begin(PC_BAUD_RATE);
  while (!Serial);
  Serial.print("Hello ");
  Serial.println(CALL );
  pinMode(LED, OUTPUT);
  pinMode(GAIN, OUTPUT);
  pinMode(RTC_1HZ_PIN, INPUT_PULLUP);
  analogWrite(GAIN, 0);
  Serial.println(F("\nStarting IRQ timer 1 for WSPR beacon"));
  Serial.println(TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));

  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(TimesNewRoman16_bold);
  oled.clear();
  oled.setCursor(5, 0);
  oled.println(F("WSPR"));
  oled.setCursor(10, 3);   //x y
  oled.print(F("F4GOH 2021"));
  delay(3000);
  oled.clear();

  // Select Timer 1-2 for UNO, 0-5 for MEGA
  // Timer 2 is 8-bit timer, only for higher frequency
  ITimer1.init();
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS, TimerHandler1))
  {
    Serial.println(F("Starting  IQT imer1 OK"));
  }
  else
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));

  initDds();
  setfreq(0, 0);
  setfreq(0, 0);
  ITimer1.stopTimer();
}


//main loop
void loop() {

#ifdef debugDDS
  char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.println(c);
    debug(c);
  }
#endif
  clock();
}

void clock() {
  RTC.read(tm);
  if (secPrec != tm.Second) {
    displayTime(false);
    digitalWrite(LED, digitalRead(LED) ^ 1);
    if ((syncMinutes(tm.Minute) == true)  && (tm.Second == 0)) {
      txWspr();
    }
    secPrec = tm.Second;
  }
  delay(10);
}

void displayTime(bool tx) {
  Serial.print(tm.Hour);
  Serial.print(":");
  Serial.print(tm.Minute);
  Serial.print(":");
  Serial.println(tm.Second);
  char heure[10];
  sprintf(heure, "%02d:%02d:%02d", tm.Hour, tm.Minute, tm.Second);
  oled.setFont(fixednums15x31);
  oled.setCursor(0, 0);
  oled.print(heure);
  if (tx) {
    oled.setFont(TimesNewRoman16_bold);
    oled.setCursor(10, 6);
    oled.print("TX: ");    
    oled.print(FREQUENCY);
  }  
}

boolean syncMinutes(int m) {
  int i;
  boolean ret = false;
  for (i = 0; i < sizeof(tableMinutes); i++) {
    if (m == tableMinutes[i]) {
      ret = true;
    }
  }
  return ret;
}

/********************************************************
  Wspr function
 ********************************************************/
void txWspr() {
  analogWrite(GAIN, PWM_GAIN);
  ptrElement = 0;
  ITimer1.restartTimer();
  while (ptrElement < 162) {
    RTC.read(tm);
    if (secPrec != tm.Second) {
      Serial.print(ptrElement);
      Serial.print(',');
      displayTime(true);
      secPrec = tm.Second;
    }
  }
  analogWrite(GAIN, 0);
  setfreq(0, 0);
  ITimer1.stopTimer();
  Serial.println("End of TX");
}

void TimerHandler1(void)
{
  static int a;
  a = int(wsprSymb[ptrElement]);
  setfreq((double) FREQUENCY + (double) a * WSPR_TONE_SPACING, 0);
  ptrElement++;
  digitalWrite(LED, digitalRead(LED) ^ 1);
}


/********************************************************
  Debug function
 ********************************************************/


void debug(char c)
{
  switch (c)
  {
    case '0' :  setfreq(0, 0);
      ddsPowerOff();
      analogWrite(GAIN, 0);
      break;
    case '1' :  ddsPowerOn();
      setfreq(FREQUENCY, 0);  //teste avec dds pw off
      analogWrite(GAIN, PWM_GAIN);
      break;
    case '2' :  factor += 100;
      Serial.println(factor);
      break;
    case '3' :  factor -= 100;
      Serial.println(factor);
      break;
    case '4' :  gain += 10;
      Serial.println(gain);
      analogWrite(GAIN, gain);
      break;
    case '5' :  gain -= 10;
      Serial.println(gain);
      analogWrite(GAIN, gain);
      break;
    case 'h' :   majRtc();
      break;
    case 'w' :
      txWspr();
      break;
  }
}


/********************************************************
  AD9850 routines
 ********************************************************/


void initDds()
{
  SPI.begin();
  pinMode(W_CLK, OUTPUT);
  pinMode(FQ_UD, OUTPUT);
  pinMode(RESET, OUTPUT);

  SPI.setBitOrder(LSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  pulse(RESET);
  pulse(W_CLK);
  pulse(FQ_UD);

}

void pulse(int pin) {
  digitalWrite(pin, HIGH);
  // delay(1);
  digitalWrite(pin, LOW);
}


void setfreq(double f, uint16_t p) {
  uint32_t deltaphase;
  if (ddsSelect == AD9850) {
    deltaphase = f * 4294967296.0 / 125000000;
  }
  else
  {
    deltaphase = f * 4294967296.0 / 180000000;
  }
  for (int i = 0; i < 4; i++, deltaphase >>= 8) {
    SPI.transfer(deltaphase & 0xFF);
  }
  if (ddsSelect == AD9850) {
    SPI.transfer((p << 3) & 0xFF) ;
  }
  else
  {
    SPI.transfer((p << 3) & 0xFF | 1) ;
  }
  pulse(FQ_UD);
}

void ddsPowerOn() {
  setfreq(0, 0);
}

void ddsPowerOff() {
  pulse(FQ_UD);
  SPI.transfer(0x04);
  pulse(FQ_UD);
}



void majRtc()
{
  time_t t;
  //check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
  Serial.println(F("update time : format yy,m,d,h,m,s"));
  Serial.println(F("exemple : 2016,6,18,16,32,30,"));

  while (Serial.available () <= 12) {
  }
  //note that the tmElements_t Year member is an offset from 1970,
  //but the RTC wants the last two digits of the calendar year.
  //use the convenience macros from Time.h to do the conversions.
  int y = Serial.parseInt();
  if (y >= 100 && y < 1000)
    Serial.println(F("Error: Year must be two digits or four digits!"));
  else {
    if (y >= 1000)
      tm.Year = CalendarYrToTm(y);
    else    //(y < 100)
      tm.Year = y2kYearToTm(y);
    tm.Month = Serial.parseInt();
    tm.Day = Serial.parseInt();
    tm.Hour = Serial.parseInt();
    tm.Minute = Serial.parseInt();
    tm.Second = Serial.parseInt();
    t = makeTime(tm);
    RTC.set(t);        //use the time_t value to ensure correct weekday is set
    setTime(t);
    while (Serial.available () > 0) Serial.read();
  }
}
