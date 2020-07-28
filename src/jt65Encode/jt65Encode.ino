/* jt65 with encode
  Anthony LE CREN F4GOH@orange.fr
  Created 26/7/2020
  the program send jt65 sequence every 2 minutes  
  In serial Monitor
  key 'h' to set up RTC
  key 'w' to send jt65 sequence 126 symbols
*/


#include <SPI.h>
#include <EEPROM.h>
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Wire.h>
#include <Time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <JTEncode.h>


Adafruit_SSD1306 lcd;

JTEncode jtencode;

#define LED 8
#define W_CLK 13
#define FQ_UD 10
#define RESET 9
#define FREQUENCY 3570100  //base freq  
#define MESSAGE "CQ F4GOH JN07"
#define JT65_TONE_SPACING        2.6917f
#define JT65_DELAY               372


long factor = -1500;		//adjust frequency
int secPrec = 0;

tmElements_t tm;

uint8_t jt65Symb[JT65_SYMBOL_COUNT];

void setup() {


  Serial.begin(115200);
  Serial.print("hello");
  pinMode(LED, OUTPUT);
  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  lcd.clearDisplay();
  lcd.setTextSize(2);
  lcd.setTextColor(WHITE);
  lcd.setCursor(0, 0);
  lcd.println(F("JT65"));
  lcd.setCursor(0, 16);    //x y
  lcd.print(F("F4GOH 2019"));
  lcd.display();
  delay(1000);
  initDds();

  setfreq(0, 0);
  setfreq(0, 0);
  
    
  memset(jt65Symb, 0, JT65_SYMBOL_COUNT);   //clear memory
  jtencode.jt65_encode(MESSAGE, jt65Symb);  
  int n, lf;      
  lf = 0;
  for (n = 0; n < JT65_SYMBOL_COUNT; n++) {   //print symbols on serial monitor
    if (lf % 16 == 0) {
      Serial.println();
      Serial.print(n);
      Serial.print(": ");
    }
    lf++;
    Serial.print(jt65Symb[n]);
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
      sendjt65(FREQUENCY);
    }
  }
  RTC.read(tm);

  if ((tm.Minute % 2) == 0 && tm.Second == 0) {
    sendjt65(FREQUENCY);
  }

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



void sendjt65(long freq) {

  int a = 0;
  for (int element = 0; element < JT65_SYMBOL_COUNT; element++) {    // For each element in the message
    a = int(jt65Symb[element]); //   get the numerical ASCII Code
    setfreq((double) freq + (double) a * JT65_TONE_SPACING, 0);
    delay(JT65_DELAY);
    Serial.print(a);
    digitalWrite(LED, digitalRead(LED) ^ 1);
  }
  setfreq(0, 0);
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
