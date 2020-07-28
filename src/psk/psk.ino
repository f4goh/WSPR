/* PSK
  Anthony LE CREN F4GOH@orange.fr
  Created 26/7/2020
  the program send temperature every 2 minutes in BPSK 31
  The transmission mode is compatible PSK or QPSK at 31,63 or 125 bps
  The sensor is a ds18s20 on pinout 7
  In serial Monitor
  key 's' to set up RTC
  key 'w' to send test PSK string "....hello 1234 test 1234 test....\n" in each mode and speed
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
#define TESTSTR "....hello 1234 test 1234 test....\n"


long factor = -1500;		//offset crystal frequency
int secPrec = 0;
tmElements_t tm;


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
  lcd.println(F("PSK"));
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
      Serial.println("BPSK 31");
      pskTx(TESTSTR, 'B', 31);
      Serial.println("BPSK 64");
      pskTx(TESTSTR, 'B', 64);
      Serial.println("BPSK 125");
      pskTx(TESTSTR, 'B', 125);
      Serial.println("QPSK 31");
      pskTx(TESTSTR, 'Q', 31);
      Serial.println("QPSK 64");
      pskTx(TESTSTR, 'Q', 64);
      Serial.println("QPSK 125");
      pskTx(TESTSTR, 'Q', 125);
    }
  }
  RTC.read(tm);

  if ((tm.Minute % 2) == 0 && tm.Second == 0) {   //every 2 minutes psk txing
    char buffer[100];
    char tempStr[10];
    float temp;
    sensors.requestTemperatures(); // Send the command to get temperatures
    temp = sensors.getTempCByIndex(0);
    Serial.print("temp : ");
    Serial.println(temp);
    dtostrf(temp, 5, 2, tempStr);
    sprintf(buffer, "....hello the temperature is %s degrees celcius...\n\n", tempStr);
    pskTx(buffer, 'B', 31);
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
   psk 31 63 125 and qpsk 31 63 125
 ********************************************/

void pskTx(char * stringPsk, char modePsk, int baudsPsk)
{
  const static int PskVaricode[2][128] PROGMEM = {
    { 683, 731, 749, 887, 747, 863, 751, 765, 767, 239, 29, 879, 733, 31, 885, 939, 759, 757, 941, 943, 859, 875, 877,
      855, 891, 893, 951, 853, 861, 955, 763, 895, 1, 511, 351, 501, 475, 725, 699, 383, 251, 247, 367, 479, 117, 53,
      87, 431, 183, 189, 237, 255, 375, 347, 363, 429, 427, 439, 245, 445, 493, 85, 471, 687, 701, 125, 235, 173, 181,
      119, 219, 253, 341, 127, 509, 381, 215, 187, 221, 171, 213, 477, 175, 111, 109, 343, 437, 349, 373, 379, 685, 503,
      495, 507, 703, 365, 735, 11, 95, 47, 45, 3, 61, 91, 43, 13, 491, 191, 27, 59, 15, 7, 63, 447, 21, 23, 5, 55, 123, 107,
      223, 93, 469, 695, 443, 693, 727, 949
    },
    { 10, 10, 10, 10, 10, 10, 10, 10, 10, 8, 5, 10, 10, 5, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
      10, 1, 9, 9, 9, 9, 10, 10, 9, 8, 8, 9, 9, 7, 6, 7, 9, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 8, 9, 9, 7, 9, 10, 10, 7, 8, 8, 8, 7, 8, 8, 9, 7,
      9, 9, 8, 8, 8, 8, 8, 9, 8, 7, 7, 9, 9, 9, 9, 9, 10, 9, 9, 9, 10, 9, 10, 4, 7, 6, 6, 2, 6, 7, 6, 4, 9, 8, 5, 6, 4, 3, 6, 9, 5, 5, 3, 6,
      7, 7, 8, 7, 9, 10, 9, 10, 10, 10
    }
  };

  const static int QpskConvol[32] PROGMEM = {16, 8, -8, 0, -8, 0, 16, 8, 0, -8, 8, 16, 8, 16, 0, -8, 8, 16, 0, -8, 0, -8, 8, 16, -8, 0, 16, 8, 16, 8, -8, 0};

  int shreg = 0;  // Shift register qpsk
  int phase = 0;
  pskIdle(baudsPsk);  // A little idle on start of transmission for AFC capture

  byte nb_bits, val;
  int d, e;
  char car = *stringPsk++;
  while (car != '\0')
  {
    Serial.print(car);
    d = int(pgm_read_word(&PskVaricode[0][car]));    // Get PSK varicode
    nb_bits = int(pgm_read_word(&PskVaricode[1][car])); // Get PSK varicode length
    d <<= 2; //add 00 on lsb for spacing between caracters
    e = d;
    for (int b = nb_bits + 2; b >= 0; b--) //send car in psk
    {
      val = bitRead(e, b); //look varicode
      if (modePsk == 'B') // BPSK mode
      {
        if (val == 0)
        {
          phase = (phase ^ 16) & 16; // Phase reverted on 0 bit
        }
      }
      else if (modePsk == 'Q') {     // QPSK mode
        shreg = (shreg << 1) | val;  // Loading shift register with next bit
        d = (int)int(pgm_read_word(&QpskConvol[shreg & 31])); // Get the phase shift from convolution code of 5 bits in shit register
        phase = (phase + d) & 31;  // Phase shifting
      }
      setfreq(FREQUENCY, phase); // Let's transmit
      delay((961 + baudsPsk) / baudsPsk);  // Gives the baud rate
      digitalWrite(LED, digitalRead(LED) ^ 1);
    }
    car = *stringPsk++;  // Next caracter in string
  }
  pskIdle(baudsPsk); // A little idle to end the transmission
  setfreq(0, 0); // No more transmission
  Serial.println("EOT");
}

void pskIdle(int baudsIdle)
{
  int phaseIdle = 0;
  for (int n = 0; n < baudsIdle; n++)
  {
    phaseIdle = (phaseIdle ^ 16) & 16;  // Idle is a flow of zeroes so only phase inversion
    setfreq(FREQUENCY, phaseIdle);   // Let's transmit
    delay((961 + baudsIdle) / baudsIdle);  // Gives the baud rate
    digitalWrite(LED, digitalRead(LED) ^ 1);
  }
}
