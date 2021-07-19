/* hellschreiber
  Anthony LE CREN F4GOH@orange.fr
  Created 16/7/2021
  the program send temperature every 2 minutes in hellschreiber
  The sensor is a ds18s20 on pinout 7
  In serial Monitor
  key 'h' to set up RTC
  key 'w' to send test hellschreiber string "....hello 1234 test 1234 test....\n"
*/

#include <SPI.h>    //for ad9850
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Wire.h>     //for rtc and oled display
#include <Time.h>
#include "SSD1306AsciiWire.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define LED 8
#define W_CLK 13
#define FQ_UD 10
#define RESET 9
#define SENSOR 7  //ds18s20 sensor
#define FREQUENCY 3570000  //frequency on waterfall
#define TESTSTR "....hello 1234 test 1234 test....\n"


long factor = -1500;		//offset crystal frequency
int secPrec = 0;
tmElements_t tm;


SSD1306AsciiWire oled;  //afficheur oled;

OneWire  ds(SENSOR);  // on pin 7 (a 3.3k or 4.7K resistor is necessary)
DallasTemperature sensors(&ds);

void setup() {
  Serial.begin(115200);
  Serial.print("hello");
  pinMode(LED, OUTPUT);
  sensors.begin();
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(TimesNewRoman16_bold);
  oled.clear();
  oled.setCursor(5, 0);
  oled.println(F("Hellschreiber"));
  oled.setCursor(10, 3);   //x y
  oled.print(F("F4GOH 2021"));
  oled.setFont(fixednums15x31);
  delay(3000);
  oled.clear();
  initDds();
  setfreq(0, 0);
  setfreq(0, 0);
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
      Serial.println("hellschreiber");
      hellTx(TESTSTR);
    }
  }
  RTC.read(tm);

  if ((tm.Minute % 2) == 0 && tm.Second == 0) {   //every 2 minutes hell txing
    char buffer[100];
    char tempStr[10];
    float temp;
    sensors.requestTemperatures(); // Send the command to get temperatures
    temp = sensors.getTempCByIndex(0);
    Serial.print("temp : ");
    Serial.println(temp);
    dtostrf(temp, 5, 2, tempStr);
    sprintf(buffer, "....hello the temperature is %s degrees celcius...\n\n", tempStr);
    hellTx(buffer);
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
  digitalWrite(pin, LOW);
}


void setfreq(double f, uint16_t p) {    //frequency , phase
  uint32_t deltaphase;
  deltaphase = f * 4294967296.0 / (125000000 + factor);
  for (int i = 0; i < 4; i++, deltaphase >>= 8) {
    SPI.transfer(deltaphase & 0xFF);
  }
  SPI.transfer((p << 3) & 0xFF) ;
  pulse(FQ_UD);
}



/********************************************************
  RTC routines
 ********************************************************/


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

/***********************************************************************************
  Hellschreiber 122.5 bauds => 8,888ms
  http://brainwagon.org/2012/01/11/hellduino-sending-hellschreiber-from-an-arduino
************************************************************************************/

void hellTx(char * stringHell)
{
  const static word GlyphTab[59][8] PROGMEM = {
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x1f9c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x0330, 0x0ffc, 0x0330, 0x0ffc, 0x0330, 0x0000, 0x0000},
    {0x078c, 0x0ccc, 0x1ffe, 0x0ccc, 0x0c78, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x01e0, 0x0738, 0x1c0e, 0x0000, 0x0000, 0x0000, 0x0000}, {0x1c0e, 0x0738, 0x01e0, 0x0000, 0x0000, 0x0000, 0x0000}, {0x018c, 0x0198, 0x0ff0, 0x0198, 0x018c, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x001c, 0x001c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x001c, 0x0070, 0x01c0, 0x0700, 0x1c00, 0x0000, 0x0000},
    {0x07f8, 0x0c0c, 0x0c0c, 0x0c0c, 0x07f8, 0x0000, 0x0000}, {0x0300, 0x0600, 0x0ffc, 0x0000, 0x0000, 0x0000, 0x0000}, {0x061c, 0x0c3c, 0x0ccc, 0x078c, 0x000c, 0x0000, 0x0000}, {0x0006, 0x1806, 0x198c, 0x1f98, 0x00f0, 0x0000, 0x0000},
    {0x1fe0, 0x0060, 0x0060, 0x0ffc, 0x0060, 0x0000, 0x0000}, {0x000c, 0x000c, 0x1f8c, 0x1998, 0x18f0, 0x0000, 0x0000}, {0x07fc, 0x0c66, 0x18c6, 0x00c6, 0x007c, 0x0000, 0x0000}, {0x181c, 0x1870, 0x19c0, 0x1f00, 0x1c00, 0x0000, 0x0000},
    {0x0f3c, 0x19e6, 0x18c6, 0x19e6, 0x0f3c, 0x0000, 0x0000}, {0x0f80, 0x18c6, 0x18cc, 0x18cc, 0x0ff0, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, {0x1800, 0x1800, 0x19ce, 0x1f00, 0x0000, 0x0000, 0x0000},
    {0x01f0, 0x0208, 0x04e4, 0x0514, 0x0514, 0x03e0, 0x0000}, {0x07fc, 0x0e60, 0x0c60, 0x0e60, 0x07fc, 0x0000, 0x0000}, {0x0c0c, 0x0ffc, 0x0ccc, 0x0ccc, 0x0738, 0x0000, 0x0000}, {0x0ffc, 0x0c0c, 0x0c0c, 0x0c0c, 0x0c0c, 0x0000, 0x0000},
    {0x0c0c, 0x0ffc, 0x0c0c, 0x0c0c, 0x07f8, 0x0000, 0x0000}, {0x0ffc, 0x0ccc, 0x0ccc, 0x0c0c, 0x0c0c, 0x0000, 0x0000}, {0x0ffc, 0x0cc0, 0x0cc0, 0x0c00, 0x0c00, 0x0000, 0x0000}, {0x0ffc, 0x0c0c, 0x0c0c, 0x0ccc, 0x0cfc, 0x0000, 0x0000},
    {0x0ffc, 0x00c0, 0x00c0, 0x00c0, 0x0ffc, 0x0000, 0x0000}, {0x0c0c, 0x0c0c, 0x0ffc, 0x0c0c, 0x0c0c, 0x0000, 0x0000}, {0x003c, 0x000c, 0x000c, 0x000c, 0x0ffc, 0x0000, 0x0000}, {0x0ffc, 0x00c0, 0x00e0, 0x0330, 0x0e1c, 0x0000, 0x0000},
    {0x0ffc, 0x000c, 0x000c, 0x000c, 0x000c, 0x0000, 0x0000}, {0x0ffc, 0x0600, 0x0300, 0x0600, 0x0ffc, 0x0000, 0x0000}, {0x0ffc, 0x0700, 0x01c0, 0x0070, 0x0ffc, 0x0000, 0x0000}, {0x0ffc, 0x0c0c, 0x0c0c, 0x0c0c, 0x0ffc, 0x0000, 0x0000},
    {0x0c0c, 0x0ffc, 0x0ccc, 0x0cc0, 0x0780, 0x0000, 0x0000}, {0x0ffc, 0x0c0c, 0x0c3c, 0x0ffc, 0x000f, 0x0000, 0x0000}, {0x0ffc, 0x0cc0, 0x0cc0, 0x0cf0, 0x079c, 0x0000, 0x0000}, {0x078c, 0x0ccc, 0x0ccc, 0x0ccc, 0x0c78, 0x0000, 0x0000},
    {0x0c00, 0x0c00, 0x0ffc, 0x0c00, 0x0c00, 0x0000, 0x0000}, {0x0ff8, 0x000c, 0x000c, 0x000c, 0x0ff8, 0x0000, 0x0000}, {0x0ffc, 0x0038, 0x00e0, 0x0380, 0x0e00, 0x0000, 0x0000}, {0x0ff8, 0x000c, 0x00f8, 0x000c, 0x0ff8, 0x0000, 0x0000},
    {0x0e1c, 0x0330, 0x01e0, 0x0330, 0x0e1c, 0x0000, 0x0000}, {0x0e00, 0x0380, 0x00fc, 0x0380, 0x0e00, 0x0000, 0x0000}, {0x0c1c, 0x0c7c, 0x0ccc, 0x0f8c, 0x0e0c, 0x0000, 0x0000}
  };
  int val;
  char ch;
  word fbits ;
  ch = *stringHell++;
  while (ch != '\0')
  {
    ch = toupper(int(ch)); // Uppercase
    if (ch >= 32 && ch <= 90) // Character is in the range of ASCII space to Z
    {
      ch -= 32;  // Character number starting at 0
      for (int i = 0; i < 7; i++) // Scanning each 7 columns of glyph
      {
        fbits = int(pgm_read_word(&GlyphTab[int(ch)][i]));  // Get each column of glyph
        digitalWrite(LED, digitalRead(LED) ^ 1);
        for (int b = 0; b < 14; b++) // Scanning each 14 rows
        {
          val = bitRead(fbits, b); // Get binary state of pixel
          setfreq(FREQUENCY * val, 0); // Let's transmit (or not if pixel is clear)
          delayMicroseconds(4002) ;  // Gives the baud rate. 4045µs minus DDS loading time, 4002µs in SPI mode, 3438µs in software serial mode
        }
      }
    }
    ch = *stringHell++; // Next character in string
  }
  setfreq(0, 0);
}
