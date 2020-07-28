/* read ds18s20 sensor
  Anthony LE CREN F4GOH@orange.fr
  Created 26/7/2020
  send dds outpout to zero to prevent conduction in the bs170
*/

#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SENSOR 7  //ds18s20 sensor
#define LED 8
#define W_CLK 13
#define FQ_UD 10
#define RESET 9  // or 10

long factor = -1500;    //adjust frequency

OneWire  ds(SENSOR);  // on pin 7 (a 3.3k or 4.7K resistor is necessary)

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&ds);

/*
 * The setup function. We only start the sensors here
 */
void setup(void)
{
  // start serial port
  Serial.begin(115200);
  Serial.println("Dallas Temperature IC Control Library");

  // Start up the library
  sensors.begin();
  initDds();

  setfreq(0, 0);
  setfreq(0, 0);
  
}

/*
 * Main function, get and show the temperature
 */
void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(sensors.getTempCByIndex(0));  
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
