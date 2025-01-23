//Служебные функции

float normalize(int value){
  return value/1024.0;  
}

float calcCheckSum(float* data, int sizeData){
  float sum = 0;
  for(int i = 0; i < sizeData - 1; i++){
    sum += data[i]; //Можно считать с дробями т.к. на обоих модулях алгоритм подсчета одинаков и числа должны совпасть
  }
  return sum;   
}

float calcFullCheckSum(float** dataArray, int sizeData)
{
  return calcCheckSum(dataArray[0], sizeData) + calcCheckSum(dataArray[1], sizeData) + calcCheckSum(dataArray[2], sizeData);
}

void debugSavedIncomingPacket()
{
  if(currPacketModuleId == INTERNAL_MODULE_ID){
    Serial.println("=== INTERNAL PACKET ===");    
  }else if(currPacketModuleId == EXTERNAL_MODULE_ID){
    Serial.println("=== EXTERNAL PACKET ===");
  }

  if(currPacketType == TYPE_PACKET::DATA){
    debugDataPacket(dataPacketInternal);
  }else if(currPacketType == TYPE_PACKET::SERVICE){
    debugServicePacket(servicePacketInternal);
  }
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

void fillIncomingActionPacket(float* incomingPacket, int module_id, COMMANDS_TYPE command, float paramValue){
  incomingPacket[0] = module_id;
  incomingPacket[1] = CENTRAL_MODULE_ID;
  incomingPacket[2] = TYPE_PACKET::CONTROL;
  incomingPacket[3] = command;
  incomingPacket[4] = paramValue;
  incomingPacket[5] = 0;
  incomingPacket[6] = 0;
  incomingPacket[7] = calcCheckSum(incomingPacket, DATA_SEGMENT_LENGTH);
}

float* getServicePacket(MODULE_ID moduleId){
  if(moduleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    return servicePacketInternal;
  }
  else if(moduleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    return servicePacketExternal;
  }
  else
  {
    return nullptr;
  }
}

float** getMeteoDataPacket(MODULE_ID moduleId){
  if(moduleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    return dataPacketInternal;
  }
  else if(moduleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    return dataPacketExternal;
  }
  else
  {
    return nullptr;
  }
}

void activateRadio(){
  digitalWrite(RADIO_CSN_PIN, LOW);
  digitalWrite(SD_CARD_CSN_PIN, HIGH);
}

void activateSdCard(){
  digitalWrite(RADIO_CSN_PIN, HIGH);
  digitalWrite(SD_CARD_CSN_PIN, LOW);
}
