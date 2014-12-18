ArduSensor Master Device
========================
The current version of the Open Sensor platform.

To use, add both the XBee and CustomGSM libraries found in /libraries/ in the Arduino IDE (Sketch -> Import Library -> Add Library..., this copies the folders to /Documents/Arduino/libraries). Then copy the included pins_arduino.h to Arduino\hardware\arduino\variants\mega

I have modified the GSM library to time out if the GSM module does not respond in time. Also changed GSMSoftSerial TX and RX pins to the ones used on the custom board and enabled interrupts on the necessary pin.

pins_arduino.h has also been modified to make the pins used to communicate with the GSM modem Arduino Mega 2560 compatible. The new pinout is included in the /libraries/ directory.