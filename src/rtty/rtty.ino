/* Rtty
  Anthony LE CREN F4GOH@orange.fr
  Created 26/7/2020
  the program send temperature every 2 minutes
  The transmission mode is RTTY (45.45 bauds, 2 bits stop, shift 170Hz)
  The sensor is a ds18s20 on pinout 7
  In serial Monitor
  key 'h' to set up RTC
  key 'w' to send test RTTY string "....hello 1234 test 1234 test....\n"
*/

#include <SPI.h>    //for ad9850
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Wire.h>     //for rtc and oled display
#include <Time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define LED 8
#define W_CLK 13
#define FQ_UD 10
#define RESET 9
#define SENSOR 7  //ds18s20 sensor
#define FREQUENCY 3570000  //frequency on waterfall
#define SHIFT 170  //shift frequency for rtty

long factor = -1500;		//offset crystal frequency
int secPrec = 0;
tmElements_t tm;

enum etatRtty
{
  LETTERS,
  FIGURES
};

Adafruit_SSD1306 lcd;
OneWire  ds(SENSOR);  // on pin 7 (a 3.3k or 4.7K resistor is necessary)
DallasTemperature sensors(&ds);

void setup() {
  Serial.begin(115200);
  Serial.print("hello");
  pinMode(LED, OUTPUT);
  sensors.begin();
  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  lcd.clearDisplay();
  lcd.setTextSize(2);
  lcd.setTextColor(WHITE);
  lcd.setCursor(0, 0);
  lcd.println(F("RTTY"));
  lcd.setCursor(0, 16);    //x y
  lcd.print(F("F4GOH 2020"));
  lcd.display();
  delay(1000);
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
      rttyTx("....hello 1234 test 1234 test....\n");
    }
  }
  RTC.read(tm);

  if ((tm.Minute % 2) == 0 && tm.Second == 0) {   //every 2 minutes RTTY txing
    char buffer[100];
    char tempStr[10];
    float temp;
    sensors.requestTemperatures(); // Send the command to get temperatures
    temp = sensors.getTempCByIndex(0);
    Serial.print("temp : ");
    Serial.println(temp);
    dtostrf(temp, 5, 2, tempStr);
    sprintf(buffer, "....hello the temperature is %s degrees celcius...\n\n", tempStr);
    rttyTx(buffer);
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

/********************************************
   RTTY
 ********************************************/
void rttyTx(char * stringRtty)
{
  //                                        sp  !   "   #  $  %   &   '   (   )  *  +  ,   -  .   /   0   1   2   3   4   5   6  7   8  9   :   ;   <  =  >  ?   @  A  B   C   D   E   F   G   H   I   J   K   L   M   N   O   P   Q   R   S   T   U   V  W   X   Y   Z
  const static int TableRtty[59] PROGMEM = {4, 13, 17, 20, 9, 0, 26, 11, 15, 18, 0, 0, 12, 3, 28, 29, 22, 23, 19, 1, 10, 16, 21, 7,  6, 24, 14, 30, 0, 0, 0, 25, 0, 3, 25, 14, 9,  1 , 13, 26, 20, 6, 11, 15,  18, 28, 12, 24, 22, 23, 10, 5,  16, 7, 30, 19, 29, 21, 17};
  char car;
  etatRtty figlett = LETTERS;  // RTTY Baudot signs/letters toggle
  car = *stringRtty++;
  while ( car != '\0')
  {
    Serial.print(car);
    car = toupper(car); // Uppercase
    if (car == '\n')  // is Line Feed ?
    {
      txByte(8);
    }
    else if (car == '\r') // is Carriage Return ?
    {
      txByte(2);
    }
    else if (car >= ' ' && car <= 'Z') // is alphanumeric char
    {
      if (car == ' ') {   // is space
        txByte(4);
      }
      else {
        car = car - ' ';        //substract space char
        if (car < 33)
        {
          if (figlett == LETTERS)
          {
            figlett = FIGURES;      // toggle form letters to signs table
            txByte(27);
          }
        }
        else if (figlett == FIGURES)
        {
          figlett = LETTERS;          // toggle form signs to letters table
          txByte(31);
        }
        txByte(char(pgm_read_word(&TableRtty[int(car)])));  // Send the 5 bits word
      }
    }
    car = *stringRtty++;  // Next character in string
  }
  setfreq(0, 0); // No more transmission
  Serial.println("EOT"); //end of transmission
}

void txByte(char car) {
  int val;
  byte b;
  car = (car << 1) | 0xc0;    //add start bit and 2 stop bits
  for (b = 0; b < 8; b++)   // LSB first
  {
    val = bitRead(car, b); // Read 1 bit
    setfreq(FREQUENCY  + (SHIFT * val), 0); // Let's transmit (bit 1 is 170Hz shifted)
    delay(22); // Gives the baud rate 45.45
  }
  digitalWrite(LED, digitalRead(LED) ^ 1);
}
