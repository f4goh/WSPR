**WSPR beacon on breadboard**

*f4goh@orange.fr*

![wspr](img/IMG_20190323_135115.jpg  "wspr finsished")

## Wiring ##

- Wire the schematics below

![sch](img/schema.png  "sch")

- Adjust output filter as filter design calculation

[filter design](http://tonnesoftware.com/svcfilter.html) 

![filter](img/filtre.png  "filter")

- Use mini tore calculator to find inductor turns

[mini tore calculator](https://constructions.f6fkn.com/downloads/minirk12-install.exe) 

![tore](img/1_5uh.png  "tore")

![tore](img/100uh.png  "tore")

- Check filter with a VNA (optional)

![vna](img/filtrewspr40m.png  "vna")

## Programming ##

- Copy libraries files into Arduino/library folder

- run WSPR encoder.exe put your callsign, locator and power and generate symbols.

![symbols](img/symbols.png  "symbols")

- Paste symbols into WsprSimple.ino

```c++
	int wsprSymb[] = {3, 3, 0, 0, 0, 2, 0, 2, 1, 2, 0, 2, 3, 3, 1, 0, 2, 0, 3, 0, 0, 1, 2, 1, 1, 3, 1, 0, 2, 0, 0, 2, 0, 2, 3, 2, 0, 1, 2, 3, 0, 0, 0, 0,
	                  2, 2, 3, 0, 1, 3, 2, 0, 3, 3, 2, 3, 2, 2, 2, 1, 3, 0, 1, 0, 2, 0, 2, 1, 1, 2, 1, 0, 3, 2, 1, 2, 3, 0, 0, 1, 2, 0, 1, 0, 1, 3, 0, 0,
	                  0, 1, 3, 2, 1, 2, 1, 2, 2, 2, 3, 0, 0, 2, 2, 2, 3, 2, 0, 1, 2, 0, 3, 3, 1, 2, 3, 3, 0, 2, 1, 3, 0, 3, 2, 2, 0, 3, 3, 1, 2, 0, 0, 0,
	                  2, 1, 0, 1, 2, 0, 3, 3, 2, 2, 0, 2, 2, 2, 2, 1, 3, 2, 1, 0, 1, 1, 2, 0, 0, 3, 1, 2, 2, 2
	                 };
```

- Adjust frequency

```c++
#define frequence 7040100
long factor = -1500;
```

## Usage notes ##
	
- Program Arduino Nano
- Adjust RTC clock by typing "h" key on serial monitor

![clock](img/clock.png  "clock")

- Connect any HF antenna

Beacon transmit every 2 min, monitor transmission with your receiver and 
adjust frequency "factor" offset if there is outside the WSPR band

- Enjoy WSPR beacon.

![map](img/wspr.png  "map")

73

Anthony

