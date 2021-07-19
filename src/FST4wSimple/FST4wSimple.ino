/* fst4w test beacon
  Anthony LE CREN F4GOH@orange.fr
  Created 16/7/2021
  the program send fst4w sequence every 2 minutes
  In serial Monitor
  key 'h' to set up RTC
  key 'f' to send fst4w sequence 160 symbols

  use fst4sim.exe in C:\WSJT\wsjtx\bin to generate symbols under console
  or into wspr/software directory (https://github.com/f4goh/WSPR)
  exemple : 
  fst4sim "F4GOH JN07 20" 60 1500 0.0 0.1 1.0 10 -15 F > FST4W_message.txt"
  result in FST4W_message.txt
  
  Message: F4GOH JN07 20                         W: F iwspr: 1
  f0: 1500.000   DT:  0.00   TxT:  51.8   SNR: -15.0

  Message bits:
  00001000110111010011000000101000100011000110011000101110101000101101010010000 000000000000000000000000

  Channel symbols: add comma in FST4Wsymbols array below 
  0132102300
  3021210200
  0330302013
  1303233323
  1032010321
  1032021303
  2102011002
  1322330132
  1023123000
  1102202303
  1212001213
  0020231032
  0101320332
  3012011111
  3002222131
  1101321023

FST4W_MODE  (only 120 to 1800) in beacon
        0 = FST4W 120
        1 = FST4W 300
        2 = FST4W 900
        3 = FST4W 1800

T/R period (s)  Symbol length (s) Tone Spacing (Hz) Occupied Bandwidth (Hz) FST4 SNR (dB) FST4W SNR (dB)
15              0.060             16.67             67.7                    -20.7
30              0.140             7.14              28.6                    -24.2
60              0.324             3.09              12.4                    -28.1
120             0.683             1.46              5.9                     -31.3         -32.8
300             1.792             0.56              2.2                     -35.3         -36.8
900             5.547             0.180             0.72                    -40.2         -41.7
1800            11.200            0.089             0.36                    -43.2         -44.8
*/


#include <SPI.h>
#include <EEPROM.h>
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Wire.h>
#include <Time.h>
#include "SSD1306AsciiWire.h"

SSD1306AsciiWire oled;  //afficheur oled;

#define LED 8
#define W_CLK 13
#define FQ_UD 10
#define RESET 9  // or 10
#define FREQUENCY 3570100  //base freq  
#define FST4W_MODE                0       //120           
#define FSTW4_INTERVAL            120
#define FST4W_SYMBOL_COUNT        160

long factor = -1500;		//adjust frequency
int secPrec = 0;

tmElements_t tm;

const uint8_t  FST4Wsymbols[FST4W_SYMBOL_COUNT] = {
  0, 1, 3, 2, 1, 0, 2, 3, 0, 0,
  3, 0, 2, 1, 2, 1, 0, 2, 0, 0,
  0, 3, 3, 0, 3, 0, 2, 0, 1, 3,
  1, 3, 0, 3, 2, 3, 3, 3, 2, 3,
  1, 0, 3, 2, 0, 1, 0, 3, 2, 1,
  1, 0, 3, 2, 0, 2, 1, 3, 0, 3,
  2, 1, 0, 2, 0, 1, 1, 0, 0, 2,
  1, 3, 2, 2, 3, 3, 0, 1, 3, 2,
  1, 0, 2, 3, 1, 2, 3, 0, 0, 0,
  1, 1, 0, 2, 2, 0, 2, 3, 0, 3,
  1, 2, 1, 2, 0, 0, 1, 2, 1, 3,
  0, 0, 2, 0, 2, 3, 1, 0, 3, 2,
  0, 1, 0, 1, 3, 2, 0, 3, 3, 2,
  3, 0, 1, 2, 0, 1, 1, 1, 1, 1,
  3, 0, 0, 2, 2, 2, 2, 1, 3, 1,
  1, 1, 0, 1, 3, 2, 1, 0, 2, 3
};

float toneSpacing[4] = {
  1.46,  // 120
  0.56,  // 300
  0.180, // 900
  0.089  // 1800
};

// Load WSPR/FST4W symbol length
unsigned int symbolLength[4] = {
  683,    // 120
  1792,   // 300
  5547,   // 900
  11200   // 1800
};


void setup() {


  Serial.begin(115200);
  Serial.print("hello");
  pinMode(LED, OUTPUT);
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(TimesNewRoman16_bold);
  oled.clear();
  oled.setCursor(5, 0);
  oled.println(F("FST4W"));
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
    if (c == 'f') {
      sendfst4w(FREQUENCY);
    }
  }
  RTC.read(tm);

  if ((tm.Minute % (FSTW4_INTERVAL/60)) == 0 && tm.Second == 0) {
    sendfst4w(FREQUENCY);
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



void sendfst4w(long freq) {

  int a = 0;
  for (int element = 0; element < FST4W_SYMBOL_COUNT; element++) {    // For each element in the message
    a = int(FST4Wsymbols[element]); //   get the numerical ASCII Code
    setfreq((double) freq + (double) toneSpacing[FST4W_MODE] * a, 0);
    delay(symbolLength[FST4W_MODE]);
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
/*
Available fonts
Adafruit5x7
font5x7
lcd5x7
Stang5x7
X11fixed7x14
X11fixed7x14B
ZevvPeep8x16
System5x7
SystemFont5x7
Iain5x7
Arial14
Arial_bold_14
Corsiva_12
Verdana_digits_24
Callibri10
Callibri11
Callibri11_bold
Callibri11_italic
Callibri14
Callibri15
Cooper19
Cooper21
Cooper26
TimesNewRoman13
TimesNewRoman13_italic
TimesNewRoman16
TimesNewRoman16_bold
TimesNewRoman16_italic
Verdana12
Verdana12_bold
Verdana12_italic
Roosewood22
Roosewood26
fixednums7x15
fixednums8x16
fixednums15x31
CalBlk36
CalLite24
lcdnums12x16
lcdnums14x24
fixed_bold10x15
Wendy3x5
newbasic3x5
font8x8
cp437font8x8
utf8font10x16
 */
