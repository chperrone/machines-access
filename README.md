# machines-access

## Introduction
Machines are dangerous to use without proper education. The Artisan's Asylum will implement card swipe access to all of the more dangerous or expensive pieces of equipment. All members are required to carry around a Charlie card and thus have a unique identity tied to them. If your card number is on the approved list, the machine will be turned on. If not, the machine will remain without power.

## Getting Started

### Hardware

The project consists of 3 main components:

+ The Adafruit Huzzah microcontroller. This microcontroller utilizes the esp8266 WiFi Serial module for serial communication between the card database and the card reader module.

+ RFID RC522 Card Reader. Reading at 13.56mhz, this card reader is suitable for the Charlie Card.

+ Powerswitch Tail. Controlled by the Huzzah board, this module safely switches up to 120 vAC power on or off depending on various system states.

### Software

+ The latest Arduino IDE. Further instructions on how to establish communication between the Huzzah and your computer can be found [here](https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/overview)

+ Miguel Balboa's rfid protocol library helps in establishing meaningful communication between the Huzzah and the rc522. Add [this library](https://github.com/miguelbalboa/rfid) to Arduino's libraries folder.

#Acknowledgements

[Jorgen Viking God](https://github.com/Jorgen-VikingGod/ESP8266-MFRC522)