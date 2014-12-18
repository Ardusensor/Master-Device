ArduSensor Master Device
========================
The current version of the Open Sensor platform.

To use, copy XBee to the 3rd party Arduino libraries folder and overwrite the default GSM library with the one included.

I have modified the GSM library to time out if the GSM module does not respond in time. Changed GSMSoftSerial TX and RX pins to the ones used on the custom board and enabled interrupts on the necessary pin.

pins_arduino.h has also been modified to make the pins used to communicate with the GSM modem Arduino Mega 2560 compatible.