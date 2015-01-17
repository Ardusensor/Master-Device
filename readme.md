ArduSensor Master Device
========================
The current version of the Open Sensor platform.

Due to an oversight with Arduino IDE 1.0.6, it is recommended to use version 1.0.5 as the newer version breaks the (default) GSM library.

To use, add both the XBee and CustomGSM libraries found in /libraries/ in the Arduino IDE (Sketch -> Import Library -> Add Library..., this copies the folders to /Documents/Arduino/libraries).

Then copy and replace the included pins_arduino.h to the Arduino IDE install folder: Arduino\hardware\arduino\variants\mega.
On Mac OS X the direcory resides inside the Arduino.app package (Arduino -> Show Package Contents -> Contents/Resources/Java/Hardware/Arduino/Variants/Mega).

I have modified the GSM library to time out if the GSM module does not respond in time. Also changed GSMSoftSerial TX and RX pins to the ones used on the custom board and enabled interrupts on the necessary pin.

pins_arduino.h has also been modified to make the pins used to communicate with the GSM modem Arduino Mega 2560 compatible. The new pinout is included in the /libraries/ directory. New interrupts are enabled in GSM3SoftSerial.cpp.
