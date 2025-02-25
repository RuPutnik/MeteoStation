void activateRadio(){
  digitalWrite(RADIO_CSN_PIN, LOW);
  digitalWrite(SD_CARD_CSN_PIN, HIGH);
}

void activateSdCard(){
  digitalWrite(RADIO_CSN_PIN, HIGH);
  digitalWrite(SD_CARD_CSN_PIN, LOW);
}

void sendDataToCard(MODULE_ID moduleId, const String& msg){
  sendMsgToCard(LOG_MSG_TYPE::METEODATA, moduleId, msg);
}

void sendInfoToCard(MODULE_ID moduleId, const String& msg){
  sendMsgToCard(LOG_MSG_TYPE::INFO, moduleId, msg);
}

void sendErrorToCard(MODULE_ID moduleId, const String& msg){
  sendMsgToCard(LOG_MSG_TYPE::ERROR, moduleId, msg);
}

String createMeteodataStr(MeteoDataPacket* dataPacket)
{
  String meteodataStr;
  const String sepComma{F(", ")};

  meteodataStr += String{dataPacket->val1} + sepComma;
  meteodataStr += String{dataPacket->val2} + sepComma;
  meteodataStr += String{dataPacket->val3} + sepComma;
  meteodataStr += String{dataPacket->val4} + sepComma;
  meteodataStr += String{dataPacket->val5} + sepComma;
  meteodataStr += String{dataPacket->val6};

  return meteodataStr;
}

String createActionServiceStr(ActionServicePacket* actSerPacket)
{
  String actSerStr;
  
  switch(actSerPacket->type){
    case TYPE_PACKET::CONTROL:
      actSerStr = F("[Action] ");
    break;
    case TYPE_PACKET::SERVICE:
      actSerStr = F("[Service] ");
    break;
    default:
      return {};
    break;
  }

  actSerStr += F("dest = ");
  actSerStr += String{actSerPacket->dest};
  actSerStr += F(", sender = ");
  actSerStr += String{actSerPacket->sender};
  actSerStr += F(", id = ");
  actSerStr += String{actSerPacket->id};
  actSerStr += F(", param = ");
  actSerStr += String{actSerPacket->valueParam};

  return actSerStr;
}

void startSdCard()
{
  pinMode(SD_CARD_CSN_PIN, OUTPUT);
  activateSdCard();
  if(!SD.begin(SD_CARD_CSN_PIN)) {
    Serial.println(F("Initialization SD card was failed"));
    return;
  }
  activateRadio();

  sdCardInitialized = true;
}

void sendMsgToCard(LOG_MSG_TYPE typeMsg, MODULE_ID moduleId, const String& msg)
{
  if(!sdCardInitialized){
    Serial.println(F("Sd Card isn't initialized!"));
    return;
  }

  activateSdCard(); //Переключаем SPI на работу с Sd картой

  File logFile = SD.open(LOG_FILENAME, FILE_WRITE);

  if(!logFile){
    Serial.println(F("Error during open/create log file!"));
    return;
  }

  String prefixMsg;

  switch(typeMsg){
    case LOG_MSG_TYPE::METEODATA:
      prefixMsg = F(" [Данные] ");
    break;
    case LOG_MSG_TYPE::INFO:
      prefixMsg = F(" [Инфо] ");
    break;
    case LOG_MSG_TYPE::ERROR:
      prefixMsg = F(" [Ошибка] ");
    break;
    default:
      prefixMsg = F(" [?] ");
    break;
  }

  String module;

  switch(moduleId)
  {
    case MODULE_ID::INTERNAL_MODULE_ID:
      module = F("[Internal]: ");
    break;
    case MODULE_ID::EXTERNAL_MODULE_ID:
      module = F("[External]: ");
    break;
    case MODULE_ID::CENTRAL_MODULE_ID:
      module = F("[Central]: ");
    break;
    default:
      module = F(": ");
    break;
  }
  
  Serial.println(F("Запись в файл: "));
  logFile.println(getCurrDateTime() + prefixMsg + module + msg);
  logFile.close();
  activateRadio();
}