/* wspr with encode
  Anthony LE CREN F4GOH@orange.fr
  Created 13/12/2020
  the program send wspr sequence 3 times per hour
  with TinyGPS library to have perfect even minutes
  cc-by-nc-sa
  WSPR  Symbol exemple
  0: 3,3,0,2,0,2,0,2,1,0,0,0,3,3,1,0,
  16: 2,0,3,2,0,3,2,3,1,1,1,0,2,0,0,2,
  32: 0,2,3,0,0,3,2,1,0,2,0,0,2,0,3,2,
  48: 1,1,2,2,3,3,2,1,2,0,2,3,3,0,1,0,
  64: 2,0,2,3,1,2,1,2,3,0,1,2,3,0,0,3,
  80: 2,0,1,2,1,1,0,0,0,1,3,2,1,0,1,0,
  96: 2,2,3,0,0,2,2,2,3,0,0,3,2,2,3,3,
  112: 1,0,3,3,0,0,1,3,0,1,2,0,0,3,3,1,
  128: 2,0,0,0,2,3,0,3,2,0,3,1,2,2,0,0,
  144: 2,2,2,1,3,0,1,0,1,3,2,0,0,3,1,0,
  160: 2,2,
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
#define W_CLK 13  //ad9850 clk
#define FQ_UD 10  //ad9850 update
#define RESET 9  // ad9850 reset

//#define debugGPS //enable print all nmea sentences on serial monitor
#define debugDDS //enable debug DDS with some key commands as 0,1,2,3 on serial monitor (see debug function)

//#define FREQUENCY 10140170  //30m
#define FREQUENCY 7040100
//#define FREQUENCY 3570100  //80m

#define CALL "KF4GOH"        //callsign
#define DBM 20              //power in dbm
#define GPS_BAUD_RATE 9600
#define PC_BAUD_RATE  115200
#define PWM_GAIN 110      //continous voltage ctrl for mosfet 110=1.2V

#define WSPR_TONE_SPACING       1.4548
#define WSPR_DELAY              682
#define ZONESEGMENT    20.0
#define FIELDSEGMENT  10.0
#define SQUARESEGMENT 10.0
#define NBSQUARE    24.0

SoftwareSerial ss(rxPin, txPin); // RX, TX for GPS
JTEncode jtencode;
TinyGPS gps;

byte tableMinutes[] = {0, 2, 4, 20, 22, 24, 40, 42, 44};   //minutes to transmit WSPR

long factor = -1500;		//adjust frequency to wspr band
uint8_t wsprSymb[WSPR_SYMBOL_COUNT];    //buffer memeory for WSPR encoding


void setup() {

  Serial.begin(PC_BAUD_RATE);
  Serial.print("Hello ");
  Serial.println(CALL );
  pinMode(LED, OUTPUT);
  pinMode(GAIN, OUTPUT);
  analogWrite(GAIN, 0);
  ss.begin(GPS_BAUD_RATE);
  delay(1000);
  initDds();
  setfreq(0, 0);
  setfreq(0, 0);
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

  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
#ifdef debugGPS
      Serial.write(c);
#endif
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }
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
      sprintf(sz, "%02d:%02d:%02d   ", hour, minute, second);
      Serial.println(sz);
      if ((syncMinutes(minute) == true)  && (second == 0)) {
        txWspr();
      }
    }
  }
}

boolean syncMinutes(byte m) {
  int i;
  boolean ret = false;
  for (i = 0; i < sizeof(tableMinutes); i++) {
    if (m == tableMinutes[i]) {
      ret = true;
    }
  }
  return ret;
}

void txWspr() {
  char loc[7];
  float flat, flon;
  unsigned long age;
  gps.f_get_position(&flat, &flon, &age);
  Serial.print("LAT=");
  Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  Serial.print(" LON=");
  Serial.println(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
  locator(flon, flat, 1).toCharArray(loc, 7);
  Serial.print("LOCATOR : ");
  Serial.println(loc);
  //wspr(FREQUENCY, "F4GOH", loc, "20");
  memset(wsprSymb, 0, WSPR_SYMBOL_COUNT);   //clear memory
  jtencode.wspr_encode(CALL, loc, DBM, wsprSymb); //encode WSPR
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
  sendWspr(FREQUENCY);
}

void sendWspr(long freqWspr) {

  int a = 0;
  analogWrite(GAIN, PWM_GAIN);
  for (int element = 0; element < 162; element++) {    // For each element in the message
    a = int(wsprSymb[element]); //   get the numerical ASCII Code
    setfreq((double) freqWspr + (double) a * WSPR_TONE_SPACING, 0);
    delay(WSPR_DELAY);
    Serial.print(a);
    digitalWrite(LED, digitalRead(LED) ^ 1);
  }
  analogWrite(GAIN, 0);
  setfreq(0, 0);
  Serial.println("EOT");
}

/********************************************************
  Debug function
 ********************************************************/


void debug(char c)
{
  switch (c)
  {
    case 'w' :
      txWspr();
      break;
    case '0' :  setfreq(0, 0);
      ddsPowerOff();
      break;
    case '1' :  ddsPowerOn();
      setfreq(FREQUENCY, 0);  //teste avec dds pw off
      break;
    case '2' :  factor += 100;
      Serial.println(factor);
      break;
    case '3' :  factor -= 100;
      Serial.println(factor);
      break;
  }
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

String locator( float longitude, float latitude, byte nb)
{
  double lo, la;
  int L1, L2, L3, L4, L5, L6;
  char locator[7];

  // "00011.0672","W","4759.7109","N"
  //  0123456789       012345678

  // compute longitude
  if (longitude > 0) {
    lo = (180 + longitude) / ZONESEGMENT; //ZONESEGMENT = 20.0
  }
  else {
    lo = (180 - longitude) / ZONESEGMENT;
    longitude *= -1;
  }

  // compute latitude
  if (latitude > 0) {
    la = (90 + latitude) / FIELDSEGMENT; //FIELDSEGMENT = 10.0
  }
  else
  {
    la = (90 - latitude) / FIELDSEGMENT;
    latitude *= -1;
  }

  L1 = int(lo);
  L2 = int(la);

  lo = (lo - L1) * SQUARESEGMENT; // SQUARESEGMENT = 10.0
  la = (la - L2) * SQUARESEGMENT;

  L3 = int(lo);
  L4 = int(la);
  L5 = int((lo - L3) * NBSQUARE); // NBSQUARE = 24.0
  L6 = int((la - L4) * NBSQUARE);

  locator[0] = L1 + 'A';
  locator[1] = L2 + 'A';
  locator[2] = L3 + '0';
  locator[3] = L4 + '0';
  if (nb == 0) {
    locator[4] = L5 + 'A';
    locator[5] = L6 + 'A';
    locator[6] = '\0';
  }
  else
  {
    locator[4] = '\0';
  }
  return String(locator);
}
