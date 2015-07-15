#include <GSM.h>
#include <XBee.h>
#include "Common.h"
#include <avr/wdt.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/power.h>

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


#define ID 208 //The unique ID of this device

#define xbeeRssiPin 47
#define MODEMSLEEPPIN 34
#define STATUSPIN 10
#define XBEE_RTS 46
#define XBEE_RESET 45
#define MODEM_POWER_PIN 24
#define MODEM_STATUS_PIN 35
#define BATTERY_VOLTAGE_PIN A14

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

//Global variables.
unsigned long prevUpdate = 1600000 - 120000; //Time since previous update in milliseconds. Set to 0 if immediate upload after boot is unwanted.
unsigned long delayTime = 300000; // 1600000; //Minutes * seconds * milliseconds. Time between data uploads in milliseconds
unsigned long lastCheck = 0; //Used to check whether the first millis() overflow has occurred to keep track of restarts.
unsigned long timeSpentSleeping = 0;
unsigned long totalTimeSlept = 0;

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
int wdt_cnt = WDTCOUNT;

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

        pinMode(MODEMSLEEPPIN, OUTPUT); // Modem sleep pin
        pinMode(xbeeRssiPin, INPUT);
        pinMode(BATTERY_VOLTAGE_PIN, INPUT);
        pinMode(STATUSPIN, OUTPUT);
        pinMode(XBEE_RTS, OUTPUT);
        pinMode(XBEE_RESET, OUTPUT);
        DDRB |= bit(LED);

        pinMode(MODEM_POWER_PIN, OUTPUT);
        pinMode(MODEM_STATUS_PIN, INPUT);

        digitalWrite(STATUSPIN, HIGH);
        digitalWrite(XBEE_RTS, HIGH);
        digitalWrite(MODEM_POWER_PIN, HIGH);
        digitalWrite(XBEE_RESET, HIGH);

        if(prevUpdate == 0){
                digitalWrite(MODEM_POWER_PIN, LOW); // Disable power to modem
        }

        analogReference(INTERNAL2V56);
        initPowerSaving();

        //digitalWrite(XBEE_RTS, LOW); // Ask for data from xbee
}

void loop()
{
        bool dosleep = true;

        digitalWrite(XBEE_RTS, LOW);
        delay(2); // wait for at least one byte
        if (Serial2.available()) {
                dosleep = false;
        }
        
        //Recieve and handle XBee packets.
        while (1) {
                xbee.readPacket();
                if (!xbee.getResponse().isAvailable()) break;

                if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
                        xbee.getResponse().getZBRxResponse(rx);

                        //Get Rssi of recieved packet, higher value = better signal
                        rssi[nrOfUpdates] = pulseIn(xbeeRssiPin, HIGH, 100000);

                        XBeeAddress64 senderLongAddress = rx.getRemoteAddress64();
                        xbeeAddressMsb[nrOfUpdates] = String(senderLongAddress.getMsb(), HEX);
                        xbeeAddressLsb[nrOfUpdates] = String(senderLongAddress.getLsb(), HEX);
                        Serial.print(F("\nAddress: "));
                        Serial.print(xbeeAddressMsb[nrOfUpdates]);
                        Serial.println(xbeeAddressLsb[nrOfUpdates]);
                        //showFrameData(); //Prints the whole incoming frame and metadata. Good for debugging.
                        handleXbeeRxMessage(rx.getData(), rx.getDataLength()); //Write the contents of the received packet to buffer.
                }
        }

        if ((millis() + totalTimeSlept - prevUpdate) > delayTime || nrOfUpdates >= maxUpdates) { //Sends data if enough time has elapsed or enough data is available.
                digitalWrite(MODEM_POWER_PIN, HIGH); // Enable modem power, power is disabled again in disconnectGSM()
                
                nrOfTries++;
                initWatchdog(); // Configure and enable WDT.
                power_adc_enable(); // Used to get battery voltage, enabled here to give time to turn on and stabilize

                if(!client.connected()){ //If not connected, connect.
                        Serial.println("Starting connections!\n");
                        connectGSM();
                }
      
                if (client.connected()){ //If connected, send data.
                        // ADC value of battery voltage, real voltage = ((read_voltage * 2.56 * 23) / 1023) / 13)
                        voltage = analogRead(BATTERY_VOLTAGE_PIN); 

                        power_adc_disable(); // Disable ADC again as we won't be using it for a while.
						
                        Upload(); //Send data

                        Serial.println(F("\nSent!"));

                        nrOfUpdates = 0; 

                        Serial.print(F("Nr of tries: "));
                        Serial.println(nrOfTries);
                        Serial.print(F("Nr of successes: "));
                        Serial.println(nrOfSuccess);
			
                        prevUpdate = millis() + totalTimeSlept; //Set time of previous update.

                        timeSpentSleeping = 0;
                        //delay(200);

                        disconnectServer();
                        disconnectGSM();
                        //delay(200);
                }

                wdt_disable(); // Disable watchdog after a successful upload.
                wdt_cnt = WDTCOUNT; // Reset counter.
        }

        if (dosleep){
                digitalWrite(XBEE_RTS, HIGH);
                delay(2);
                Serial.flush();
                initSleep();
        }
        else{
                delay(2);
        }
}

void initPowerSaving()
{
        power_twi_disable(); // Disable TWI
        power_spi_disable(); // Disable SPI

        // Disable unused USARTs
        power_usart1_disable();
        power_usart3_disable();

        // Disable timers. Timer0 is used by Arduino, Timer2 is used to wake up device
        power_timer1_disable();
        power_timer3_disable();
        power_timer4_disable();
        power_timer5_disable();

        power_adc_disable(); // Disable ADC until needed
}

/*
        Set to wake up on timer2 compare.
*/
void initSleep()
{
        set_sleep_mode(SLEEP_MODE_PWR_SAVE);
        cli();        

        power_timer0_disable(); // Stop Arduino timer
        power_usart2_disable(); // Stop xbee uart
        power_usart0_disable(); // Stop Serial0
   

        init_timer2_sleep(); // Set timer2 to run with 1024 prescaling, interrupt on overflow
        sei();
        digitalWrite(STATUSPIN, LOW);
        for (uint16_t i = 0; i < 10; ++i){
                sleep_mode(); // Go to sleep
                sleep_disable(); // Program continues from here.
        }

        TCCR2B = 0; // Stop timer2 counter
        totalTimeSlept += 300;
        digitalWrite(STATUSPIN, HIGH);
        
        power_timer0_enable(); // Start Arduino timer

        power_usart0_enable();
        power_usart2_enable(); // Start xbee uart
}


void init_timer2_sleep()
{
        TCNT2 = 0; // Zero timer/counter value
        TIMSK2 = 1; // Enable overflow interrupt vector
        TCCR2B = 7; // Set 1024 prescaling
}

boolean checkFirstTimerOverflow() //Check whether lastCheck is bigger than millis(), this signals whether a restart has occurred or millis() has overflown
{
        if(lastCheck > millis() + timeSpentSleeping){
              firstOverflow = true;
        }
  
        lastCheck = millis() + timeSpentSleeping;
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

ISR(TIMER2_OVF_vect)
{
        ;
}

ISR(WDT_vect)
{
        /*
        Serial.print("WDT: ");
        Serial.println(wdt_cnt);
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
