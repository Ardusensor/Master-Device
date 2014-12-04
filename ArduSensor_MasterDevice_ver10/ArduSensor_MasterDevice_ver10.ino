#include <GSM.h>
#include <XBee.h>

//This data should reflect your SIM card and your operator's settings. Refer to your operators website for the information.
#define PINNUMBER ""  // PIN Number

// APN data
#define GPRS_APN       "internet.tele2.ee" //GPRS APN
#define GPRS_LOGIN     "wap"    //GPRS login
#define GPRS_PASSWORD  "wap" //GPRS password

//How many updates to collect before uploading them to the server.
#define maxUpdates 20

// Xbee RSSI pin
#define xbeeRssiPin 47

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
int ID = 156; //The unique ID of this device. // 156 oli viimane mis Alarile laivi läks
/* ID!!!! */

//Global variables.4
unsigned long prevUpdate = 1600000 - 120000; //Time since previous update in milliseconds. - 5000 to make the first upload within 5 seconds of booting.
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
//boolean newAddress = false;

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
   scannerNetworks.begin(); //For diagnostics info from the GSM module.
   Serial.println();
   pinMode(34, OUTPUT);
   pinMode(xbeeRssiPin, INPUT);
}

void loop()
{
  //Recieve and handle XBee packets.
  xbee.readPacket();
  if (xbee.getResponse().isAvailable()) {
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
        
//        // If this is the first time this XBEE has sent us a packet, upload now.
//        if(IsAddressInArray(xbeeAddressMsb[nrOfUpdates], xbeeAddressLsb[nrOfUpdates]) == true){
//          prevUpdate = delayTime;
//         }
    }
 }
  
     if(millis() - prevUpdate > delayTime || nrOfUpdates >= maxUpdates){ //Sends data if enough time has elapsed or enough data is available.
     nrOfTries++;
     
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
      }
     
      // Siia tuleb laadimise pask!
}


boolean checkFirstTimerOverflow(){ //Check whether lastCheck is bigger than millis(), this signals whether a restart has occurred or millis() has overflown
  if(lastCheck > millis()){
    firstOverflow = true;
  }
  
  lastCheck = millis();
  return(firstOverflow);
}

// Test to see if a XBee has been seen before.
//boolean IsAddressInArray(String addressMSB, String addressLSB){
//  newAddress = false;
//  for(int i = 0; i < maxUpdates; i++){
//    if(addressMSB.equalsIgnoreCase(xbeeAddressMsb[i]) && addressLSB.equalsIgnoreCase(xbeeAddressLsb[i])){
//      newAddress = true;
//    }
//  }
//  return newAddress;
//}
