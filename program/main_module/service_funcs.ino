//Служебные функции

void debugSavedIncomingPacket()
{
  if(currPacketModuleId == INTERNAL_MODULE_ID){
    Serial.println(F("=== INTERNAL PACKET ==="));    
  }else if(currPacketModuleId == EXTERNAL_MODULE_ID){
    Serial.println(F("=== EXTERNAL PACKET ==="));
  }

  if(currPacketType == TYPE_PACKET::DATA){
    MeteoDataPacket* const dataPacket = getMeteoDataPacket(currPacketModuleId);
    debugDataPacket(dataPacket);
  }else if(currPacketType == TYPE_PACKET::SERVICE){
    ActionServicePacket* const servicePacket = getServicePacket(currPacketModuleId);
    debugServicePacket(servicePacket);
  }
}

void debugDataPacket(MeteoDataPacket* packet){
  Serial.println("=== DATA PACKET ===");
  Serial.println(static_cast<String>("type = ") + packet->type);
  Serial.println(static_cast<String>("dest = ") + packet->dest);
  Serial.println(static_cast<String>("sender = ") + packet->sender);
  Serial.println(static_cast<String>("numPacket = ") + packet->numPacket);
  Serial.println(static_cast<String>("valueParam1 = ") + packet->val1);
  Serial.println(static_cast<String>("valueParam2 = ") + packet->val2);
  Serial.println(static_cast<String>("valueParam3 = ") + packet->val3);
  Serial.println(static_cast<String>("valueParam4 = ") + packet->val4);
  Serial.println(static_cast<String>("valueParam5 = ") + packet->val5);
  Serial.println(static_cast<String>("valueParam6 = ") + packet->val6);
  Serial.println(static_cast<String>("ckSum = ") + packet->ckSum);
}

void debugServicePacket(ActionServicePacket* packet){
  Serial.println("=== SERVICE PACKET ===");
  Serial.println(static_cast<String>("type = ") + packet->type);
  Serial.println(static_cast<String>("dest = ") + packet->dest);
  Serial.println(static_cast<String>("sender = ") + packet->sender);
  Serial.println(static_cast<String>("numPacket = ") + packet->numPacket);
  Serial.println(static_cast<String>("serviceId = ") + packet->id);
  Serial.println(static_cast<String>("valueParam = ") + packet->valueParam);
  Serial.println(static_cast<String>("ckSum = ") + packet->ckSum);
}

void debugActionPacket(ActionServicePacket* packet){
  Serial.println("=== ACTION PACKET ===");
  Serial.println(static_cast<String>("type = ") + packet->type);
  Serial.println(static_cast<String>("dest = ") + packet->dest);
  Serial.println(static_cast<String>("sender = ") + packet->sender);
  Serial.println(static_cast<String>("numPacket = ") + packet->numPacket);
  Serial.println(static_cast<String>("actionId = ") + packet->id);
  Serial.println(static_cast<String>("valueParam = ") + packet->valueParam);
  Serial.println(static_cast<String>("ckSum = ") + packet->ckSum);
}

void fillIncomingActionPacket(ActionServicePacket* incomingPacket, int module_id, COMMANDS_TYPE command, float paramValue){
  incomingPacket->dest = module_id;
  incomingPacket->sender = MODULE_ID::CENTRAL_MODULE_ID;
  incomingPacket->type = TYPE_PACKET::CONTROL;
  incomingPacket->id = command;
  incomingPacket->valueParam = paramValue;
  incomingPacket->ckSum = calcCheckSum(incomingPacket, ACTSERV_PACKET_LENGTH);
}

ActionServicePacket* getServicePacket(MODULE_ID moduleId)
{
  if(moduleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    return &internalServicePacket;
  }
  else if(moduleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    return &externalServicePacket;
  }
  else
  {
    return nullptr;
  }
}

MeteoDataPacket* getMeteoDataPacket(MODULE_ID moduleId)
{
  if(moduleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    return &internalDataPacket;
  }
  else if(moduleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    return &externalDataPacket;
  }
  else
  {
    return nullptr;
  }
}

uint8_t getMaxMeteoParamIndex()
{
  return (currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID) ? COUNT_METEO_PARAM_INTERNAL - 1 : COUNT_METEO_PARAM_EXTERNAL - 1;
}
