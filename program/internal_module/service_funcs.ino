//Служебные функции
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

void fillIncomingActionPacket(ActionServicePacket* incomingPacket, COMMANDS_TYPE command, float paramValue){
  incomingPacket->dest = MODULE_ID::INTERNAL_MODULE_ID;
  incomingPacket->sender = MODULE_ID::CENTRAL_MODULE_ID;
  incomingPacket->type = TYPE_PACKET::CONTROL;
  incomingPacket->id = command;
  incomingPacket->valueParam = paramValue;
  incomingPacket->ckSum = calcCheckSum(incomingPacket, ACTSERV_PACKET_LENGTH);
}
