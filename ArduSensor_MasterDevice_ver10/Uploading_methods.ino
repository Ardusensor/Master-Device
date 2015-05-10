//Buffer layout: [[t_CPU, t_Sensor, voltage, cap, counter], [t_CPU, t_Sensor, voltage, cap, counter]...]
void jCoordinatorData()
{
        client.print(F("{\"coordinator\":{" ));
        client.print(F("\"coordinator_id\":"));
        client.print(ID);
        client.print(F(",\"gsm_coverage\":"));
        client.print(signalStrength);
        client.flush(); //Trust me, you want these.
        delay(100); //Trust me, you want these.
        client.print(F(",\"battery_voltage\":"));
        client.print(voltage);
        client.print(F(",\"uptime\":"));
        client.print(millis());
        client.flush();
        delay(100);
        client.print(F(",\"first_overflow\":"));
        client.print(checkFirstTimerOverflow());
        client.print(F(",\"tries\":"));
        client.print(nrOfTries);
        client.flush();
        delay(100);
        client.print(F(",\"successes\":"));
        client.print(++nrOfSuccess);
        client.print(F(",\"sensor_readings\":["));
        client.flush();
        delay(100);
}

void jDeviceReadings(int nr)
{
        client.print(F("{\"sensor_id\":\""));
        client.print(xbeeAddressMsb[nr]);
        client.print(xbeeAddressLsb[nr]);
        client.print(F("\",\"battery_voltage\":"));
        client.print(buffer[nr][2]);
        client.flush();
        delay(100);
        client.print(F(",\"cpu_temperature\":"));
        client.print(buffer[nr][0]);
        client.print(F(",\"sensor_temperature\":"));
        client.print(buffer[nr][1]);
        client.print(F(",\"moisture\":"));
        client.flush();
        delay(100);
        client.print(buffer[nr][3]);
        client.print(F(",\"packet_rssi\":"));
        client.print(rssi[nr]);
        client.print(F(",\"sendcounter\":"));
        client.print(buffer[nr][4]);
        client.print(F("}"));
        client.flush();
        delay(100);
}

void jUploadEnd()
{
        client.print(F("]}}"));
        client.flush();
        delay(100);
}