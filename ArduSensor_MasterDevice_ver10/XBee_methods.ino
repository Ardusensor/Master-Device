//When packet is recieved, copy the contents into buffer.
//Buffer layout: [[t_CPU, t_Sensor, voltage, cap, counter], [t_CPU, t_Sensor, voltage, cap, counter]...]

void handleXbeeRxMessage(uint8_t *data, uint8_t length)
{
        if (nrOfUpdates >= maxUpdates){
                return;
        }

        char tmp_1[30]; // Max packet size shouldn't be more than 22 characters, including \0

        for (int i = 0; i < length; ++i){
                tmp_1[i] = (char)data[i];
        }

        tmp_1[length] = '\0'; // Append nullcharacter to mark end of string

        Serial.print(F("Packet: "));
        Serial.println(tmp_1);

        if(tmp_1[0] == '<' && tmp_1[length - 1] == '>'){
                sscanf(tmp_1, "<%d;%d;%d;%d;%d>",
                        &buffer[nrOfUpdates][0], &buffer[nrOfUpdates][1],
                        &buffer[nrOfUpdates][2], &buffer[nrOfUpdates][3],
                        &buffer[nrOfUpdates][4]);

                Serial.print(F("RSSI: "));
                Serial.println(rssi[nrOfUpdates]);
                Serial.print(F("Number Of Updates: "));
                Serial.println(++nrOfUpdates);
        }
}

//Print the contents of revieced frame.
void showFrameData()
{
        Serial.println(F("Incoming frame data:"));
        for (int i = 0; i < xbee.getResponse().getFrameDataLength(); i++){
                print8Bits(xbee.getResponse().getFrameData()[i]);
                Serial.print(' ');
        }
        Serial.println();

        for (int i= 0; i < xbee.getResponse().getFrameDataLength(); i++){
                Serial.write(' ');
                if (iscntrl(xbee.getResponse().getFrameData()[i]))
                        Serial.write(' ');
                else
                        Serial.write(xbee.getResponse().getFrameData()[i]);
                Serial.write(' ');
        }

        Serial.println(); 
}

//Print 32 bits with formatting.
void print32Bits(uint32_t dw)
{
        print16Bits(dw >> 16);
        print16Bits(dw & 0xFFFF);
}

//Print 16 bits with formatting.
void print16Bits(uint16_t w)
{
        print8Bits(w >> 8);
        print8Bits(w & 0x00FF);
}

//Print 8 bits with formatting.
void print8Bits(byte c)
{
        uint8_t nibble = (c >> 4);
        if (nibble <= 9)
                Serial.write(nibble + 0x30);
        else
                Serial.write(nibble + 0x37);
      
        nibble = (uint8_t) (c & 0x0F);
        if (nibble <= 9)
                Serial.write(nibble + 0x30);
        else
                Serial.write(nibble + 0x37);
}