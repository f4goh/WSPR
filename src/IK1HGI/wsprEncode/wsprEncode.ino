/* wspr with encode
  Anthony LE CREN F4GOH@orange.fr
  Created 16/7/2021
  the program send wspr sequence every 2 minutes
  In serial Monitor
  key 'h' to set up RTC
  key 'w' to send wspr sequence 162 symbols
*/


#include <SPI.h>
#include <EEPROM.h>
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Wire.h>
#include <Time.h>
#include "SSD1306AsciiWire.h"
#include <JTEncode.h>

SSD1306AsciiWire oled;  //afficheur oled;

JTEncode jtencode;

#define LED 8
#define W_CLK 13
#define FQ_UD 10
#define RESET 9
#define GAIN 6  //pwm output pinout to adjust gain (mos polarization)
//#define FREQUENCY 475700  //base freq  
//#define FREQUENCY 1838100  //base freq  
//#define FREQUENCY 3570100  //base freq  
#define FREQUENCY 7040100
#define CALL "IK1HGI"
#define LOCATOR "JN45"
#define DBM 20
#define WSPR_TONE_SPACING       1.4548
#define WSPR_DELAY              682
#define PWM_GAIN 170      //continous voltage ctrl for mosfet R3=1K and R4=4.7K

long factor = -1500;		//adjust frequency to wspr band
int secPrec = 0;

int gain = PWM_GAIN;
tmElements_t tm;

uint8_t wsprSymb[WSPR_SYMBOL_COUNT];

void setup() {


  Serial.begin(115200);
  Serial.print("hello");
  pinMode(LED, OUTPUT);
  pinMode(GAIN, OUTPUT);
  analogWrite(GAIN, 0);
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(TimesNewRoman16_bold);
  oled.clear();
  oled.setCursor(5, 0);
  oled.println(F("WSPR"));
  oled.setCursor(10,3);    //x y
  oled.print(F("F4GOH 2021"));
  oled.setFont(fixednums15x31);  
  delay(3000);
  oled.clear();  
  initDds();

  setfreq(0, 0);
  setfreq(0, 0);


  memset(wsprSymb, 0, WSPR_SYMBOL_COUNT);   //clear memory
  jtencode.wspr_encode(CALL, LOCATOR, DBM, wsprSymb); //encode WSPR
  int n, lf;
  lf = 0;
  for (n = 0; n < WSPR_SYMBOL_COUNT; n++) {   //print symbols on serial monitor
    if (lf % 16 == 0) {
      Serial.println();
      Serial.print(n);
      Serial.print(": ");
    }
    lf++;
    Serial.print(wsprSymb[n]);
    Serial.print(',');
  }
  Serial.println();
}


void loop() {
  char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.println(c);
    if (c == 'h') {
      majRtc();
    }
    if (c == 'w') {
      sendWspr(FREQUENCY);
    }
  }
  RTC.read(tm);

  if ((tm.Minute % 2) == 0 && tm.Second == 0) {
    sendWspr(FREQUENCY);
  }

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
    if (tm.Second % 2 == 0) digitalWrite(LED, digitalRead(LED) ^ 1);
  }
  delay(10);
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

  deltaphase = f * 4294967296.0 / (125000000 + factor);
  for (int i = 0; i < 4; i++, deltaphase >>= 8) {
    SPI.transfer(deltaphase & 0xFF);
  }
  SPI.transfer((p << 3) & 0xFF) ;
  pulse(FQ_UD);
}



void sendWspr(long freqWspr) {
  analogWrite(GAIN, PWM_GAIN);
  int a = 0;
  for (int element = 0; element < 162; element++) {    // For each element in the message
    a = int(wsprSymb[element]); //   get the numerical ASCII Code
    setfreq((double) freqWspr + (double) a * WSPR_TONE_SPACING, 0);
    delay(WSPR_DELAY);
    Serial.print(a);
    digitalWrite(LED, digitalRead(LED) ^ 1);
  }
  setfreq(0, 0);
  analogWrite(GAIN, 0);
  Serial.println("EOT");
}

void majRtc()
{
  time_t t;
  //check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
  Serial.println(F("update time : format yy,m,d,h,m,s"));
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
