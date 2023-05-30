# aTARi8modem
### this project is to create a simple, plug and play, no BS atari 8bit compatable ethernet modem.

## Hardware
We are using an arduino UNO and the ENC28J60 ethernet board as the initial test base.  Once this is solid, the idea is to start working on a circuit board to decrease cost and size.

### The connections
**NOTE:** *For the time being, this is text - hopefully, I'll have time to create a diagram to show the connectivity*

#### Arduino - ENC28J60
Connect the Ethernet module to the Arduino using the SPI interface.

* Arduino Uno 5V to ENC28J60 VCC
* Arduino Uno GND to ENC28J60 GND
* Arduino Uno Pin 13 (SCK) to ENC28J60 SCK
* Arduino Uno Pin 12 (MISO) to ENC28J60 SO
* Arduino Uno Pin 11 (MOSI) to ENC28J60 SI
* Arduino Uno Pin 10 (SS) to ENC28J60 CS

#### Atari SIO port to Arduino Uno
Here is the wiring scheme from Atari SIO to Arduino Uno:

* Atari SIO Pin 1 (Data Out) to Arduino Uno digital Pin 2 (with appropriate level shifting)
* Atari SIO Pin 3 (Data In) to Arduino Uno digital Pin 3 (with appropriate level shifting)
* Atari SIO Pin 4 (Clock In) to Arduino Uno digital Pin 4 (with appropriate level shifting)
* Atari SIO Pin 10 (Clock Out) to Arduino Uno digital Pin 5 (with appropriate level shifting)
* Atari SIO Pin 5 (+5V/Ready) to Arduino Uno digital Pin 6 (with appropriate level shifting)
* Atari SIO Pin 6 (Ground) to Arduino Uno GND

**NOTE:** *No level shifting is in this design yet.  DO NOT CONNECT THIS TO YOUR ATARI.  IT COULD FRY IT. Which would be sad. And kittens will cry. Think of the kittens. The voltage levels of Atari's SIO port are not necessarily 5V tolerant (they claim to be, but like seriously.. would'nt you rather be protected in case?), so you need to use level shifting circuitry to safely connect it to the Arduino Uno. Also, remember to always double-check your connections and make sure the Atari 800XL is powered off before making any connections.*

