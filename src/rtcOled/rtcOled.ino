/*rtcOled
  Anthony LE CREN F4GOH@orange.fr
  Created 16/7/2021
  RTC and oled clock test
  no ad9850 and no bs170 connected !
  for breadboard only
*/

//#define DEBUG
#include <Wire.h>
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Time.h>
#include "SSD1306AsciiWire.h"
#include <stdio.h>

#define LED 2

SSD1306AsciiWire oled;  //afficheur oled;

tmElements_t tm;
int secPrec = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("oled/rtc test");
  pinMode(LED, OUTPUT);
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(TimesNewRoman16_bold);
  oled.clear();
  oled.setCursor(5, 0);
  oled.println(F("RTC/OLED"));
  oled.setCursor(10, 3);   //x y
  oled.print(F("F4GOH 2021"));
  oled.setFont(fixednums15x31);
  delay(3000);
  oled.clear();
  delay(1000);
}


//main loop

void loop() {
  char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    if (c == 'h') {
      majRtc();
    }
  }
  digitalWrite(LED, digitalRead(LED) ^ 1);
  RTC.read(tm);
  digitalWrite(LED, digitalRead(LED) ^ 1);
  if (secPrec != tm.Second) {
    Serial.print(tm.Hour);
    Serial.print(":");
    Serial.print(tm.Minute);
    Serial.print(":");
    Serial.println(tm.Second);
    char heure[10];
    sprintf(heure, "%02d:%02d:%02d", tm.Hour, tm.Minute, tm.Second);
    oled.setCursor(0, 0);
    oled.print(heure);
    secPrec = tm.Second;
  }
  delay(10);
}




void majRtc()
{
  time_t t;
  //check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
  Serial.println(F("update time : format yy,m,d,h,m,s,"));
  Serial.println(F("exemple : 2016,6,18,16,32,30, "));

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
