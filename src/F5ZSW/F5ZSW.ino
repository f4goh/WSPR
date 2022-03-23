/*sample code for F5ZSW
  Anthony LE CREN F4GOH@orange.fr
  Created 23/3/2022
  the program send cw and ft8 sequence 4 times per minutes
  with GPS to have perfect timing
*/


#include <SPI.h>
#include <JTEncode.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>


//NANO pinout
#define txPin 5 //tx pin into RX GPS connection
#define rxPin 4 //rx pin into TX GPS connection
#define LED 8   //output pin led
#define GAIN 6  //pwm output to adjust gain (mos polarization)
#define SENSOR 7  //ds18s20 sensor
#define PTT A0  //ds18s20 sensor
#define W_CLK 13  //ad9850 clk
#define FQ_UD 10  //ad9850 update
#define RESET 9  // ad9850 reset
#define MOSI 11  // 

//#define debugGPS //enable print all nmea sentences on serial monitor
#define debugDDS //enable debug DDS with some key commands as 0,1,2,3 on serial monitor (see debug function)
//#define debugSYMBOLS

#define CW_FREQUENCY 10133000  //same fequency for test
#define FT8_FREQUENCY 10133000  //

#define CW_TXT " VVV F5ZSW JN23MK"

#define CALL "F5ZSW"        //callsign
#define DBM 20              //power in dbm
#define GPS_BAUD_RATE 9600
#define PC_BAUD_RATE  115200
#define PWM_GAIN 140      //continous voltage ctrl for mosfet 110=1.2V  -> 1K and 4.7K (210)

#define WSPR_TONE_SPACING       1.4548
#define WSPR_DELAY              682
#define MESSAGE "CQ F5ZSW JN23"
#define FT8_TONE_SPACING        6.25f
#define FT8_DELAY               160

SoftwareSerial ss(rxPin, txPin); // RX, TX for GPS
JTEncode jtencode;
TinyGPS gps;


enum mode {
  CW,
  FT8_CQ,
};

byte tableSeconds[] = {0, 15, 30, 45}; //seconds to transmit
mode tableMode[] = {CW, CW, CW, FT8_CQ};

long factor = -1500;		//adjust frequency to wspr band
uint8_t symbols[WSPR_SYMBOL_COUNT];    //buffer memeory for WSPR or ft8 encoding

typedef struct
{
  char locator[9];   /*!< locator */
  uint8_t pwrDbm;
} position;

position pos;

int gain = PWM_GAIN;


void setup() {
  Serial.begin(PC_BAUD_RATE);
  Serial.print("Hello ");
  Serial.println(CALL );
  pinMode(LED, OUTPUT);
  pinMode(GAIN, OUTPUT);
  pinMode(MOSI, INPUT);
  pinMode(PTT, OUTPUT);
  digitalWrite(PTT, LOW);
  analogWrite(GAIN, 0);
  ss.begin(GPS_BAUD_RATE);
  delay(2000);
  initDds();
  setfreq(0, 0);
  setfreq(0, 0);
  //latlng2loc(48.605776, -4.550759, 0);
  //Serial.print("LOCATOR : ");
  //Serial.println(pos.locator);
  //Serial.println(pos.pwrDbm);


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
  lectGps();
}


/********************************************************
  WSPR loop with time sync
 ********************************************************/

void  lectGps()
{
  bool newData = false;

  // for (unsigned long start = millis(); millis() - start < 1000;)
  // {
  while (ss.available())
  {
    char c = ss.read();
#ifdef debugGPS
    Serial.write(c);
#endif
    if (gps.encode(c)) // Did a new valid sentence come in?
      newData = true;
  }
  // }
  if (newData)
  {
    unsigned long age;
    int year;
    byte month, day, hour, minute, second, hundredths;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    if (age == TinyGPS::GPS_INVALID_AGE)
      Serial.println("*******    *******    ");
    else
    {
      char sz[32];
      char index;
      sprintf(sz, "%02d:%02d:%02d   ", hour, minute, second);
      Serial.println(sz);
      digitalWrite(LED, digitalRead(LED) ^ 1);
      index = syncSeconds(second);
      if ((index >= 0)  && (second % 15 == 0)) {
        mode mtx = tableMode[index];
        switch (mtx) {
          case CW :
            txCW(CW_FREQUENCY, CW_TXT, 30);
            break;
          case FT8_CQ :
            txFt8(FT8_CQ);
            break;
        }
      }
    }
  }
}

char syncSeconds(int m) {
  int i;
  char ret = -1;
  for (i = 0; i < sizeof(tableSeconds); i++) {
    if (m == tableSeconds[i]) {
      ret = i;
    }
  }
  return ret;
}

/********************************************************
  Debug function
********************************************************/


void debug(char c)
{
  switch (c)
  {
    case 'c' :
      txCW(CW_FREQUENCY, CW_TXT, 30);
      break;
    case 'f' :
      txFt8(FT8_CQ);    //ft8 cq
      break;
    case '0' :
      analogWrite(GAIN, 0);
      setfreq(0, 0);
      ddsPowerOff();
      digitalWrite(PTT, LOW);
      break;
    case '1' :  ddsPowerOn();
      setfreq(CW_FREQUENCY, 0);  //test avec dds pw off
      analogWrite(GAIN, gain);
      digitalWrite(PTT, HIGH);
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

    case 'l' :
      latlng2loc(47.890242, 0.276770, 0);
      Serial.println(pos.locator);
      Serial.println(pos.pwrDbm);
      Serial.flush();
      break;
    case 'd' :
      initDds();
      break;
  }
}
/**************************
   cw
 **************************/
void txCW(long freqCw, char * stringCw, int cwWpm) {
  const static int morseVaricode[2][59] PROGMEM = {
    {0, 212, 72, 0, 144, 0, 128, 120, 176, 180, 0, 80, 204, 132, 84, 144, 248, 120, 56, 24, 8, 0, 128, 192, 224, 240, 224, 168, 0, 136, 0, 48, 104, 64, 128, 160, 128, 0, 32, 192, 0, 0, 112, 160, 64, 192, 128, 224, 96, 208, 64, 0, 128, 32, 16, 96, 144, 176, 192},
    {7, 6, 5, 0, 4, 0, 4, 6, 5, 6, 0, 5, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 0, 5, 0, 6, 6, 2, 4, 4, 3, 1, 4, 3, 4, 2, 4, 3, 4, 2, 2, 3, 4, 4, 3, 3, 1, 3, 4, 3, 4, 4, 4}
  };

  int tempo = 1200 / cwWpm; // Duration of 1 dot
  byte nb_bits, val;
  int d;
  int c = *stringCw++;
  Serial.println("CW TX");
  digitalWrite(PTT, HIGH);
  delay(10);
  analogWrite(GAIN, PWM_GAIN);
  while (c != '\0') {
    c = toupper(c); // Uppercase
    if (c == 32) {     // Space character between words in string
      setfreq(0, 0); // 7 dots length spacing
      delay(tempo * 7); // between words
    }
    else if (c > 32 && c < 91) {
      c = c - 32;
      d = int(pgm_read_word(&morseVaricode[0][c]));    // Get CW varicode
      nb_bits = int(pgm_read_word(&morseVaricode[1][c])); // Get CW varicode length
      if (nb_bits != 0) { // Number of bits = 0 -> invalid character #%<>
        for (int b = 7; b > 7 - nb_bits; b--) { // Send CW character, each bit represents a symbol (0 for dot, 1 for dash) MSB first
          val = bitRead(d, b); //look varicode
          setfreq(freqCw, 0); // Let's transmit
          delay(tempo + 2 * tempo * val);  // A dot length or a dash length (3 times the dot)

          setfreq(0, 0); // 1 dot length spacing
          delay(tempo);     // between symbols in a character
        }
      }
      setfreq(0, 0); // 3 dots length spacing
      delay(tempo * 3); // between characters in a word
    }
    c = *stringCw++;  // Next caracter in string
  }
  digitalWrite(PTT, LOW);
  analogWrite(GAIN, 0);
  setfreq(0, 0);
  Serial.println("EOT");
}



/**************************
   ft8
 **************************/

void txFt8(mode mtx) {
  char message[14];
  float flat, flon;
  unsigned long age;
  Serial.println("CW FT8");
  gps.f_get_position(&flat, &flon, &age);
  Serial.print("LAT=");
  Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  Serial.print(" LON=");
  Serial.println(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
  latlng2loc(flat, flon, 1);
  sprintf(message, "CQ %s %s", CALL, pos.locator);

  Serial.println(message);
  memset(symbols, 0, FT8_SYMBOL_COUNT);   //clear memory
  jtencode.ft8_encode(message, symbols);
#ifdef debugSYMBOLS
  int n, lf;
  lf = 0;
  for (n = 0; n < FT8_SYMBOL_COUNT; n++) {   //print symbols on serial monitor
    if (lf % 16 == 0) {
      Serial.println();
      Serial.print(n);
      Serial.print(": ");
    }
    lf++;
    Serial.print(symbols[n]);
    Serial.print(',');
  }
  Serial.println();
#endif
  sendFt8(FT8_FREQUENCY);
}

void sendFt8(long freq) {
  int a = 0;
  digitalWrite(PTT, HIGH);
  delay(10);
  analogWrite(GAIN, PWM_GAIN);
  for (int element = 0; element < FT8_SYMBOL_COUNT; element++) {    // For each element in the message
    a = int(symbols[element]); //   get the numerical ASCII Code
    setfreq((double) freq + (double) a * FT8_TONE_SPACING, 0);
    delay(FT8_DELAY);
#ifdef debugSYMBOLS
    Serial.print(a);
#endif
    digitalWrite(LED, digitalRead(LED) ^ 1);
  }
  digitalWrite(PTT, LOW);
  analogWrite(GAIN, 0);
  setfreq(0, 0);
  Serial.println("EOT");
}





/********************************************************
  AD9850 routines
********************************************************/


void initDds()
{
  pinMode(MOSI, OUTPUT);
  delay(10);
  SPI.begin();
  pinMode(W_CLK, OUTPUT);
  pinMode(FQ_UD, OUTPUT);
  pinMode(RESET, OUTPUT);

  SPI.setBitOrder(LSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  pulse(RESET);
  pulse(W_CLK);
  pulse(FQ_UD);
  setfreq(0, 0);
  setfreq(0, 0);
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

void ddsPowerOn() {
  setfreq(0, 0);
}

void ddsPowerOff() {
  pulse(FQ_UD);
  SPI.transfer(0x04);
  pulse(FQ_UD);
}

/********************************************************
  locator calculator
********************************************************/

void latlng2loc(float lat, float lng, byte nb)
{
  const uint8_t valid_dbm[19] = {0, 3, 7, 10, 13, 17, 20, 23, 27, 30, 33, 37, 40, 43, 47, 50, 53, 57, 60};
  lat += 90; // lat origin of Maidenhead is 90S
  lng += 180; // lng origin of Maidenhead is 180W
  double units = 43200; // 18 fields * 10 squares * 24 subsquares * 10 extended square
  int xLong, yLat, indice, dbm;
  // get the extended sq
  lat = floor(lat * units / 180.0);
  lng = floor(lng * units / 360.0);
  pos.locator[6] = (long(lng) % 10) + '0';
  pos.locator[7] = (long(lat) % 10) + '0';
  lat = floor(lat / 10);
  lng = floor(lng / 10);
  xLong = long(lng) % 24;
  yLat = long(lat) % 24;
  indice = (xLong / 4) * 3 + (yLat / 8);
  pos.pwrDbm = valid_dbm[indice];
  pos.locator[4] = xLong + 'A';
  pos.locator[5] = yLat + 'A';
  // get the sq.
  lat = floor(lat / 24);
  lng = floor(lng / 24);
  pos.locator[2] = (long(lng) % 10) + '0';
  pos.locator[3] = (long(lat) % 10) + '0';
  // get the fields
  lat = floor(lat / 10);
  lng = floor(lng / 10);
  pos.locator[0] = long(lng) + 'A';
  pos.locator[1] = long(lat) + 'A';
  if (nb == 0) {
    pos.locator[8] = '\0';
  }
  else
  {
    pos.locator[4] = '\0';
  }
}
