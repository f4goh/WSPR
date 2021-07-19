/* FSQ with encode
  Anthony LE CREN F4GOH@orange.fr
  Created 16/7/2021
  the program send fsq sequence every 30 seconds
  Use # command according to https://www.qsl.net/zl1bpu/MFSK/TELEMweb.htm
  the payload are save file destination and temperature value.
  Serial Monitor commands
  key 'h' to set up RTC
  key 'w' to send fsq sequence n symbols (variable)
*/


#include <SPI.h>
#include <EEPROM.h>
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Wire.h>
#include <Time.h>
#include "SSD1306AsciiWire.h"
#include <JTEncode.h>
#include <OneWire.h>
#include <DallasTemperature.h>


SSD1306AsciiWire oled;  //afficheur oled;

JTEncode jtencode;

#define LED 8
#define W_CLK 13
#define FQ_UD 10
#define RESET 9
#define SENSOR 7  //ds18s20 sensor
#define FREQUENCY 3571350  // Base freq is 1350 Hz higher than dial freq in USB 3570000
#define FROM_CALL "KF4GOH"  //beacon call
#define TO_CALL "F4GOH"     //destionation call could be allcall for all stations
const char *file = "[data.tlm]";    //telemetry destination file (need tlm suffix)

#define FSQ_TONE_SPACING        8.79f
#define FSQ_2_DELAY             500          // Delay value for 2 baud FSQ
#define FSQ_3_DELAY             333          // Delay value for 3 baud FSQ
#define FSQ_4_5_DELAY           222          // Delay value for 4.5 baud FSQ
#define FSQ_6_DELAY             167          // Delay value for 6 baud FSQ
#define FSQ_DELAY               FSQ_4_5_DELAY


long factor = -1500;		//adjust frequency
int secPrec = 0;

tmElements_t tm;
OneWire  ds(SENSOR);  // on pin 7 (a 3.3k or 4.7K resistor is necessary)
DallasTemperature sensors(&ds);

uint8_t tx_buffer[255];
uint8_t symbol_count;


void setup() {


  Serial.begin(115200);
  Serial.print("hello");
  pinMode(LED, OUTPUT);
  sensors.begin();
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(TimesNewRoman16_bold);
  oled.clear();
  oled.setCursor(5, 0);
  oled.println(F("FSQ"));
  oled.setCursor(10,3);    //x y
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
      encodeTelemetry();
      sendFsq(FREQUENCY);
    }
  }
  RTC.read(tm);

  if (tm.Second % 30 == 0) {
    encodeTelemetry();
    sendFsq(FREQUENCY);
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

void encodeTelemetry()
{
  float temp;
  char tempStr[10];
  char buffer[100];
  sensors.requestTemperatures(); // Send the command to get temperatures
  temp = sensors.getTempCByIndex(0);
  Serial.print("temp : ");
  Serial.println(temp);
  dtostrf(temp, 5, 2, tempStr);   //convert float to char array with 2 decimal 
  memset(tx_buffer, 0, 255);   //clear memory
  sprintf(buffer, "%s%s",file,tempStr);   //make payload buffer file name and temperature value
  jtencode.fsq_dir_encode(FROM_CALL, TO_CALL, '#', buffer, tx_buffer); //Callsign from,Callsign to,cmd - Directed command,message 100 chars max,Array of channel symbols

  uint8_t j = 0;  
  while (tx_buffer[j++] != 0xff);
  symbol_count = j - 1;             //find symbol_count values (variable, depend payload length)

  int n, lf;
  lf = 0;
  for (n = 0; n < symbol_count; n++) {   //print symbols on serial monitor
    if (lf % 16 == 0) {
      Serial.println();
      Serial.print(n);
      Serial.print(": ");
    }
    lf++;
    Serial.print(tx_buffer[n]);
    Serial.print(',');
  }
  Serial.println();
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



void sendFsq(long freq) {

  int a = 0;
  for (int element = 0; element < symbol_count; element++) {    // For each element in the message
    a = int(tx_buffer[element]); //   get the numerical ASCII Code
    setfreq((double) freq + (double) a * FSQ_TONE_SPACING, 0);
    delay(FSQ_DELAY);
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
