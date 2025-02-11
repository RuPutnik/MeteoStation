//Служебные функции

float normalize(int value){
  return value/1024.0;  
}

float calcCheckSum(float* data, int sizeData){
  float sum=0;
  for(int i=0;i<sizeData-1;i++){
    sum += data[i]; //Можно считать с дробями т.к. на обоих модулях алгоритм подсчета одинаков и числа должны совпасть
  }
  return sum;   
}

float calcFullCheckSum(float** dataArray, int sizeData)
{
  return calcCheckSum(dataArray[0], sizeData) + calcCheckSum(dataArray[1], sizeData) + calcCheckSum(dataArray[2], sizeData);
}

void debugDataPacket(float** packet){
  Serial.println("=== DATA PACKET ===");
  for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
    for(int j = 0; j < DATA_SEGMENT_LENGTH; j++){
      Serial.print(packet[i][j] + (String)" | ");
    }
    Serial.println("\n------");
  }
}

void debugServicePacket(float* packet){
  Serial.println("=== SERVICE PACKET ===");
  for(int j = 0; j < DATA_SEGMENT_LENGTH; j++){
      Serial.print(packet[j] + (String)" | ");
  }
  Serial.print("\n");
}

void debugActionPacket(float* packet){
  Serial.println("=== ACTION PACKET ===");
  for(int j = 0; j < DATA_SEGMENT_LENGTH; j++){
      Serial.print(packet[j] + (String)" | ");
  }
  Serial.print("\n");
}

void fillIncomingActionPacket(float* incomingPacket, COMMANDS_TYPE command, float paramValue){
  incomingPacket[0] = MODULE_ID::EXTERNAL_MODULE_ID;
  incomingPacket[1] = MODULE_ID::CENTRAL_MODULE_ID;
  incomingPacket[2] = TYPE_PACKET::CONTROL;
  incomingPacket[3] = command;
  incomingPacket[4] = paramValue;
  incomingPacket[5] = 0;
  incomingPacket[6] = 0;
  incomingPacket[7] = calcCheckSum(incomingPacket, DATA_SEGMENT_LENGTH);
}
