#ifndef SERVICE_FUNCS
#define SERVICE_FUNCS

//Служебные функции
void resetDetectorMap(bool* detectorMap){
  for(int i=0;i<COUNT_DETECTOR;i++){
    detectorMap[i]=true;
  }
}

float normalize(int value){
  return value/1024.0;  
}

int calcCheckSum(float* data, int sizeData){
  int sum=0;
  for(int i=0;i<sizeData-1;i++){
    sum+=(int)data[i];
  }
  return sum;   
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
}

void debugActionPacket(float* packet){
  Serial.println("=== ACTION PACKET ===");
  for(int j = 0; j < DATA_SEGMENT_LENGTH; j++){
      Serial.print(packet[j] + (String)" | ");
  }
}

void fillIncomingActionPacket(float* incomingPacket, COMMANDS_TYPE command, float paramValue){
  incomingPacket[0] = MODULE_ID;
  incomingPacket[1] = CENTRAL_MODULE_ID;
  incomingPacket[2] = TYPE_PACKET::CONTROL;
  incomingPacket[3] = command;
  incomingPacket[4] = paramValue;
  incomingPacket[5] = 0;
  incomingPacket[6] = 0;
  incomingPacket[7] = calcCheckSum(incomingPacket, DATA_SEGMENT_LENGTH);
}

#endif