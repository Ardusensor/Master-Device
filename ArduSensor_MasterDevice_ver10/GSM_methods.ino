//Create GSM connection, then connect to GPRS.
void connectGSM()
{
        Serial.println(F("Connecting to GSM network..."));

        theGSM3ShieldV1ModemCore.println("AT"); //Recommended AT commands to wake up modem. The GSM librabry has a bug that causes the program to hang on begin(), this should help alleviate the problem. A fix has also been implemented into the included GSM library.
        theGSM3ShieldV1ModemCore.println("AT");
  
        if(gsmAccess.begin(PINNUMBER)==GSM_READY){ //Start the GSM connection using the information provided in the main file.
                Serial.println(F("PIN OK"));
                scannerNetworks.begin(); //For diagnostics info from the GSM module.                
                signalStrength = scannerNetworks.getSignalStrength().toInt(); //Signal strength, 0-31 range.
  
                if(gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD)==GPRS_READY){ //Attach GPRS, information must be provided in the main file.
                        Serial.println(F("GPRS OK"));
                        connectServer();
                }
        }
}

//Connect to server, information must be provided in the main file. 
void connectServer()
{
        Serial.println(F("Connecting to server..."));
        if (client.connect(server, port)){
                Serial.println(F("Connected"));
        }
        else{
                Serial.println(F("Connection failed."));
                disconnectGSM(); // Try again in case of failure to connect.
                connectGSM();
        }
}

//Flush and disconnect. 
void disconnectServer()
{
        client.flush();
        client.stop();
        Serial.println(F("Disconnected from server."));
}

//Power down the GSM module to save power.
void disconnectGSM()
{
        gsmAccess.shutdown();
        Serial.println(F("GSM shutdown."));
        digitalWrite(MODEM_POWER_PIN, LOW); // Disable power to modem
}