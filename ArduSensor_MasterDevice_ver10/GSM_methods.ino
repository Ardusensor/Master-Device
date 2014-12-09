//Create GSM connection, then connect to GPRS.
void connectGSM(){ 
  Serial1.end(); //Stop XBee Serial just in case.
  Serial.println(F("Connecting to GSM network..."));
  bitRead(PORTB, LED) ? bitClear(PORTB, LED) : bitSet(PORTB, LED);
  theGSM3ShieldV1ModemCore.println("AT"); //Recommended AT commands to wake up modem. The GSM librabry has a bug that causes the program to hang on begin(), this should help alleviate the problem. A fix has also been implemented into the included GSM library.
  delay(100);
  theGSM3ShieldV1ModemCore.println("AT");
  delay(100);
  
 if(gsmAccess.begin(PINNUMBER)==GSM_READY){ //Start the GSM connection using the information provided in the main file.
     Serial.println(F("PIN OK"));
     signalStrength = scannerNetworks.getSignalStrength().toInt(); //Signal strength, 0-31 range.
     delay(1000);
  
    if(gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD)==GPRS_READY){ //Attach GPRS, information must be provided in the main file.
        Serial.println(F("GPRS OK"));
        delay(1000);
        connectServer();
    }
  }
}


//Connect to server, information must be provided in the main file. 
void connectServer(){ 
  Serial.println(F("Connecting to server..."));
    if (client.connect(server, port))
    {
      Serial.println(F("Connected"));
    } 
    else
    {
      Serial.println(F("Connection failed."));
      disconnectGSM(); //Try again in case of failure to connect.
      connectGSM();
  }
}

//Flush and disconnect. 
void disconnectServer(){
  client.flush();
   delay(100);
  client.stop();
  theGSM3ShieldV1ModemCore.println("AT+QICLOSE"); //Close the TCP/IP connection
  delay(2000); //Just in case
  Serial.println(F("Disconnected from server."));
}

//Power down the GSM module to save power.
void disconnectGSM(){
  theGSM3ShieldV1ModemCore.println("AT+QIDEACT");
  delay(2000);
  /*
  digitalWrite(34, LOW);//pin 34 LOW for <1s to reset modem
  delay(900);
  digitalWrite(34, HIGH);*/
  gsmAccess.shutdown();
  bitRead(PORTB, LED) ? bitClear(PORTB, LED) : bitSet(PORTB, LED);
  Serial1.begin(9600); //Start XBee Serial connection again.
  Serial.println(F("GSM shutdown."));
}
