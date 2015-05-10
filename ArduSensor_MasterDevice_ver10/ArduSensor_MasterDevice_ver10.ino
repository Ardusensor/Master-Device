#include <GSM.h>
#include <XBee.h>
#include "Common.h"
#include <avr/wdt.h>
#include <string.h>

#define LED 7 // Led on PIN 13, PB7

//This data should reflect your SIM card and your operator's settings. Refer to your operators website for the information.
#define PINNUMBER ""  // PIN Number

// APN data
#define GPRS_APN       "internet.tele2.ee" //GPRS APN
#define GPRS_LOGIN     "wap"    //GPRS login
#define GPRS_PASSWORD  "wap" //GPRS password

//How many updates to collect before uploading them to the server.
#define maxUpdates 20
#define WDTCOUNT 10

// Xbee RSSI pin
#define xbeeRssiPin 47
#define MODEMSLEEPPIN 34

//Initialize the library instances
GSMClient client;
GPRS gprs;
GSM gsmAccess(true); //true enables GSM debugging information in the console. 
GSMScanner scannerNetworks;

//Initialize XBee library instances.
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();

//Create objects for XBee responses
ZBRxResponse rx = ZBRxResponse();
ZBRxIoSampleResponse ioSample = ZBRxIoSampleResponse();

//Server details
char server[] = "www.ardusensor.com"; //IP address of server
int port = 18150; //Port for server. 18151 for logs, 18150 for data

/* ID!!!! */
int ID = 171; //The unique ID of this device.
/* ID!!!! */

//Global variables.
unsigned long prevUpdate = 1600000 - 120000; //Time since previous update in milliseconds. Set to 0 if immediate upload after boot is unwanted.
unsigned long delayTime = 1600000; //Minutes * seconds * milliseconds. Time between data uploads in milliseconds
unsigned long lastCheck = 0; //Used to check whether the first millis() overflow has occurred to keep track of restarts.
boolean firstOverflow = false;
int nrOfTries = 0; //Count the number of tries and successful uploads to the server for debugging.
int nrOfSuccess = 0;
int nrOfUpdates = 0; //Don't change this. This keeps count of the number of data packets recieved from End Devices.

int voltage; //Battery voltage
String tmp; //Holds an XBee packet while reading data from it.

int signalStrength;
unsigned long rssi[maxUpdates];

//NB! Size of buffer[Max collected data][Max data length]. Max data length needs to be long enough to hold the longest possible packet from the Sensors.
int buffer[maxUpdates][5]; //Array of updates, this is where the data from updates recieved from XBee are kept until a successful upload.
String xbeeAddressMsb[maxUpdates]; //Holds the MSB of the unique XBee addresses for all recieved packets. These are sent to the server
String xbeeAddressLsb[maxUpdates]; //LSB for addresses.

 /* wdt_cnt * 8 seconds gives us a timeout after which the device restarts itself. 
 This is to avoid all GSM hangups which tend to happen. WDT is enabled before starting data uploads and is 
 disabled after an upload has completed. wdt_cnt is then set back to it's default value defined
 in this file's header.
 The counter is decremented inside the WDT ISR vector. When wdt_cnt reaches 0, the WDT is configured to perform
 a system reset after which the modem hardware is reset by the Mega MCU.
 */
int wdt_cnt = 2;

int bytes_in = 0;
char buf[10];

void setup()
{
        Serial2.begin(9600); //Serial connection to XBee.
        xbee.setSerial(Serial2); //Set XBee to use Serial 2.
        Serial.begin(9600); //Start serial connection with PC.
        Serial.println(F("Starting up...")); //Greetings
        Serial.print(F("ID: "));
        Serial.println(ID);
        Serial.print(F("Upload time: "));
        Serial.println(delayTime);
        Serial.print(F("Max Updates: "));
        Serial.println(maxUpdates);
        Serial.println();

        initWatchdog();
        scannerNetworks.begin(); //For diagnostics info from the GSM module.
        Serial.println();
        pinMode(MODEMSLEEPPIN, OUTPUT); // Modem sleep pin
        pinMode(xbeeRssiPin, INPUT);
        DDRB |= bit(LED);
        pinMode(LED, OUTPUT); // Status LED, shows modem status as seen by CPU.  

        wdt_disable();
        wdt_cnt = WDTCOUNT; // Reset WDT counter.
}

void loop()
{
  //Recieve and handle XBee packets.
        xbee.readPacket();
        if (xbee.getResponse().isAvailable()) {
                bitRead(PORTB, LED) ? bitClear(PORTB, LED) : bitSet(PORTB, LED); // Flip LED
                if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
                        xbee.getResponse().getZBRxResponse(rx);
      
                        //Get Rssi of recieved packet
                        rssi[nrOfUpdates] = pulseIn(xbeeRssiPin, HIGH, 100000); //Higher value = better signal

                        XBeeAddress64 senderLongAddress = rx.getRemoteAddress64();
                        xbeeAddressMsb[nrOfUpdates] = String(senderLongAddress.getMsb(), HEX);
                        xbeeAddressLsb[nrOfUpdates] = String(senderLongAddress.getLsb(), HEX);
                        Serial.print(F("\nAddress: "));
                        Serial.print(xbeeAddressMsb[nrOfUpdates]);
                        Serial.println(xbeeAddressLsb[nrOfUpdates]);
                        //showFrameData(); //Prints the whole incoming frame and metadata. Good for debugging.
                        handleXbeeRxMessage(rx.getData(), rx.getDataLength()); //Write the contents of the recieved packet to buffer.
                        bitRead(PORTB, LED) ? bitClear(PORTB, LED) : bitSet(PORTB, LED);
                }
        }
  
        if(millis() - prevUpdate > delayTime || nrOfUpdates >= maxUpdates){ //Sends data if enough time has elapsed or enough data is available.
                nrOfTries++;
                initWatchdog(); // Configure and enable WDT.

                if(!client.connected()){ //If not connected, connect.
                        Serial.println("Starting connections!\n");
                        connectGSM();
                }
      
                if (client.connected()){ //If connected, send data.
                        voltage = analogRead(A0); //ADC value of battery voltage
            
                        //Send stuff
                        jCoordinatorData();
                        for(int i = 0; i < nrOfUpdates; i++){
                                jDeviceReadings(i);
                                if(i != nrOfUpdates - 1){ //The comma is needed for JSON formatting.
                                        client.print(",");
                                }
                        }
                        jUploadEnd();

                        Serial.println(F("\nSent!"));

                        nrOfUpdates = 0; 

                        Serial.print(F("Nr of tries: "));
                        Serial.println(nrOfTries);
                        Serial.print(F("Nr of successes: "));
                        Serial.println(nrOfSuccess);
                        prevUpdate = millis(); //Set time of previous update.
                        delay(200);

                        disconnectServer();
                        disconnectGSM();
                }

                wdt_disable(); // Disable watchdog after a successful upload.
                wdt_cnt = WDTCOUNT; // Reset counter.
        }

        if (Serial.available() > 0){
                buf[bytes_in++] = Serial.read();
                if (strcmp(buf, "ping!\r\n") == 0){
                        bytes_in = 0;
                        for (int i = 0; i < 10; ++i){
                                buf[i] = ' ';
                        }
                        Serial.println("pong!");
                }
                else if(bytes_in >= 10){
                        bytes_in = 0;
                }

        }
}

boolean checkFirstTimerOverflow() //Check whether lastCheck is bigger than millis(), this signals whether a restart has occurred or millis() has overflown
{
        if(lastCheck > millis()){
              firstOverflow = true;
        }
  
        lastCheck = millis();
        return(firstOverflow);
}

void initWatchdog()
{
        cli(); // Disable all interrupts during the configuration of WDT as a precaution.

        // Setting these bits gives us 4 cycles to modify WDT registers.
        SETBIT(WDTCSR, WDCE);
        SETBIT(WDTCSR, WDE);

        wdt_enable(WDTO_8S);

        // Set Watchdog to interrupt on timeout.
        SETBIT(WDTCSR, WDCE);
        SETBIT(WDTCSR, WDE);
        SETBIT(WDTCSR, WDIE);

        wdt_reset();
        sei(); // Enable interrupts again.
}

ISR(WDT_vect)
{
        /*
        Serial.print("WDT: ");
        Serial.println(--wdt_cnt);
        */
        if(--wdt_cnt <= 0){ /* Check whether enough time has passed to warrant a system reset. 
                If so, configure WDT to reset system instead of throwing an interrupt.*/
                SETBIT(WDTCSR, WDE);
        }
        else { // If wdt_cnt > 0, reset WDT flags to stay in interrupt mode.
                SETBIT(WDTCSR, WDIE);
                CLEARBIT(MCUSR, WDRF); // Clear WDT flag.
                CLEARBIT(WDTCSR, WDE);
        }
}
