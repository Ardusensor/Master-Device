//When packet is recieved, copy the contents into buffer.
//Buffer layout: [[t_CPU, t_Sensor, voltage, cap, counter], [t_CPU, t_Sensor, voltage, cap, counter]...]
void handleXbeeRxMessage(uint8_t *data, uint8_t length){
  //Read packet contents into temporary buffer
  tmp = 0;
  for (int i = 0; i < length; i++){
    tmp += char(data[i]);
  }
  Serial.print(F("Packet: "));
  Serial.println(tmp);
  
  //Check packet integrity and filter data
  if(tmp.startsWith("<") && tmp.endsWith(">")){
    
    buffer[nrOfUpdates][0] = tmp.substring(1, tmp.indexOf(";")).toInt();
    //Serial.println(tmp);
    //Serial.println(buffer[nrOfUpdates][0]);
    clearRead();
    
    buffer[nrOfUpdates][1] = tmp.substring(tmp.lastIndexOf("x") + 1).toInt();
    //Serial.println(tmp);
    //Serial.println(buffer[nrOfUpdates][1]);
    clearRead();
    
    buffer[nrOfUpdates][2] = tmp.substring(tmp.lastIndexOf("x") + 1).toInt();
    //Serial.println(tmp);
    //Serial.println(buffer[nrOfUpdates][2]);
    clearRead();
    
    buffer[nrOfUpdates][3] = tmp.substring(tmp.lastIndexOf("x") + 1).toInt();
    //Serial.println(tmp);
    //Serial.println(buffer[nrOfUpdates][3]);
    clearRead();
    
    buffer[nrOfUpdates][4] = tmp.substring(tmp.lastIndexOf("x") + 1).toInt();
    //Serial.println(tmp);
    //Serial.println(buffer[nrOfUpdates][4]);
    
    Serial.print(F("Number Of Updates: "));
    Serial.println(++nrOfUpdates);
  
    tmp = 0;
  }
}


//Clears read data from packet for simpler formatting.
void clearRead(){
    int firstSemi = tmp.indexOf(";");
    for (int i = 0; i <= firstSemi; i++){
      tmp.setCharAt(i, 'x');
    }
}


//Print the contents of revieced frame.
void showFrameData(){
  Serial.println(F("Incoming frame data:"));
  for (int i = 0; i < xbee.getResponse().getFrameDataLength(); i++) {
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
void print32Bits(uint32_t dw){
  print16Bits(dw >> 16);
  print16Bits(dw & 0xFFFF);
}

//Print 16 bits with formatting.
void print16Bits(uint16_t w){
  print8Bits(w >> 8);
  print8Bits(w & 0x00FF);
}

//Print 8 bits with formatting.
void print8Bits(byte c){
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
