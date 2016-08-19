void Upload()
{
        jCoordinatorData();
        for(int i = 0; i < nrOfUpdates; i++){
                jDeviceReadings(i);
                if(i != nrOfUpdates - 1){ //The comma is needed for JSON formatting.
                        client.print(",");
                }
        }
        jUploadEnd();
}

//Buffer layout: [[t_CPU, t_Sensor, voltage, cap, counter], [t_CPU, t_Sensor, voltage, cap, counter]...]
void jCoordinatorData()
{
        client.print(F("{\"coordinator\":{" ));
        client.print(F("\"coordinator_id\":"));
        client.print(ID);
        client.print(F(",\"gsm_coverage\":"));
        client.print(signalStrength);
        client.flush(); //Trust me, you want these.
        client.print(F(",\"battery_voltage\":"));
        client.print(voltage);
        client.print(",\"temperature\":");
        client.print(temperatureRead (MAIN_TEMPERATURE));
        client.print(F(",\"uptime\":"));
        client.print(millis() + totalTimeSlept);
        client.flush();
        client.print(F(",\"first_overflow\":"));
        client.print(checkFirstTimerOverflow());
        client.print(F(",\"tries\":"));
        client.print(nrOfTries);
        client.flush();
        client.print(F(",\"successes\":"));
        client.print(++nrOfSuccess);
        client.print(F(",\"sensor_readings\":["));
        client.flush();
}

void jDeviceReadings(int nr)
{
        client.print(F("{\"sensor_id\":\""));
        client.print(xbeeAddressMsb[nr]);
        client.print(xbeeAddressLsb[nr]);
        client.print(F("\",\"calib_sensor\":"));
        client.print(buffer[nr][2]);
        client.flush();
        client.print(F(",\"temperature\":"));
        client.print(postTemperature (nr));
        client.print(F(",\"battery_voltage\":"));
        client.print(postVoltage (nr));
        client.print(F(",\"moisture\":"));
        client.flush();
        client.print(buffer[nr][3]);
        client.print(F(",\"packet_rssi\":"));
        client.print(rssi[nr]);
        client.print(F(",\"sendcounter\":"));
        client.print(buffer[nr][4]);
        client.print(F("}"));
        client.flush();
}

void jUploadEnd()
{
        client.print(F("]}}"));
        client.flush();
}
