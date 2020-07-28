/*rtcOled
  Anthony LE CREN F4GOH@orange.fr
  Created 8/2/2019
  RTC and oled clock test
  no ad9850 and no bs170 connected !
  for breadboard only
*/

//#define DEBUG
#include <Wire.h>
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdio.h>

#define LED 2

Adafruit_SSD1306 lcd;

tmElements_t tm;
int secPrec = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("WSPR sur breadboard");
  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  pinMode(LED, OUTPUT);
  lcd.clearDisplay();
  lcd.setTextSize(2);
  lcd.setTextColor(WHITE);
  lcd.setCursor(0, 0);
  lcd.println(F("WSPR"));
  lcd.setCursor(0, 16);    //x y
  lcd.print(F("F4GOH 2019"));
  lcd.display();
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
    lcd.clearDisplay();
    lcd.setCursor(0, 0);
    char heure[10];
    sprintf(heure, "%02d:%02d:%02d", tm.Hour, tm.Minute, tm.Second);
    lcd.print(heure);
    lcd.display();
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
