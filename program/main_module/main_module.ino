#include <microDS3231.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>

#include <general.h>

#define KEY_POSTHANDLE_DELAY_MSEC    1000
#define WAIT_ACK_REC_COMMAND_MSEC    2000
#define PRINT_SERVICE_MSG_MSEC       1000

#define PIPE_WRITE_ADDRESS           0xF0F0F0F0E1LL
#define PIPE_READ_ADDRESS_EXTERNAL   0xF0F0F0F0E2LL
#define PIPE_READ_ADDRESS_INTERNAL   0xF0F0F0F0E3LL
#define LCD2004_ADDRESS              0x3F

#define COUNT_SEGMENTS_IN_PACKET     3
#define COUNT_METEO_PARAM_EXTERNAL   6
#define COUNT_METEO_PARAM_INTERNAL   4
#define COUNT_ATTEMPT_SEND_ACTION    3
#define COUNT_KEYS                   5

#define RADIO_CE_PIN                 8
#define RADIO_CSN_PIN                9

enum WORK_MODE: uint8_t
{
  SHOW_METEO_DATA = 0,
  SHOW_COMMANDS   = 1
};

enum SHOW_DATA_MODE: uint8_t
{
  CLASSIC       = 0,
  ALTERNATIVE   = 1
};

enum KEY_PORT: uint8_t
{
  KEY_1   = 4, //center
  KEY_2   = 3, //topleft
  KEY_3   = 5, //bottomRight
  KEY_4   = 2, //bottomLeft
  KEY_5   = 6  //topRight
};

enum LOG_MSG_TYPE: uint8_t
{
  METEODATA = 0,
  INFO      = 1,
  ERROR     = 2
};

void printDisplayModuleServiceMsg(SERVICE_MSG_TYPE typeServicePacket, float valueParam = 0);

LiquidCrystal_I2C lcd(LCD2004_ADDRESS, 20, 4);
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
MicroDS3231 rtc;

void* currIncomingPacket = nullptr;
ActionServicePacket externalServicePacket, internalServicePacket;
ActionServicePacket actionPacket;
MeteoDataPacket externalDataPacket, internalDataPacket;

TYPE_PACKET currPacketType = TYPE_PACKET::UNKNOWN;
MODULE_ID currPacketModuleId = MODULE_ID::INCORRECT_MODULE_ID;
MODULE_ID currDisplayedModuleId = MODULE_ID::INTERNAL_MODULE_ID;
WORK_MODE currWorkMode  = WORK_MODE::SHOW_METEO_DATA;
SHOW_DATA_MODE currShowDataMode = SHOW_DATA_MODE::CLASSIC;
COMMANDS_TYPE currCommand = COMMANDS_TYPE::STOP_START_SEND;
uint8_t currMeteoParam;
float currNumOutPacket;

void setup()
{
  startDisplay();
  Serial.begin(SERIAL_SPEED);
  //rtc.setTime(31, 32, 21, 25, 1, 2025);
  currNumOutPacket = 0;
  currMeteoParam = 1;

  currIncomingPacket = malloc(DATA_PACKET_LENGTH);
  
  startRadio();
  initializeButtons();

  delay(SETUP_DELAY);
  updateDisplay();
}

void startRadio()
{
  radio.begin();
  radio.setChannel(RADIO_CHANNEL_NUMBER);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.enableDynamicAck(); //Разрешаем выборочное отключение подтверждения передачи данных
  radio.enableDynamicPayloads();
  radio.openReadingPipe(1, PIPE_READ_ADDRESS_EXTERNAL);
  radio.openReadingPipe(2, PIPE_READ_ADDRESS_INTERNAL);
  radio.startListening();
}

void loop()
{
  if(radio.available())
  {
    const bool isCorrectData = processIncomingData();

    if(isCorrectData)
      displayIncomingData();
  }

  buttonsHandler();
  updateDisplayHeader(); //Всегда обновляем первую строку, т.к. там выводится текущее время
  delay(LOOP_DELAY_MSEC);
}

bool processIncomingData()
{
  Serial.println("Data Incoming!");
  radio.read(currIncomingPacket, DATA_PACKET_LENGTH);

  const bool validIncomingData = analyzeIncomingPacket(currIncomingPacket);

  if(!validIncomingData){
    //Serial.println("DATA IS INVALID!");
    return false;
  }

  saveIncomingData(currIncomingPacket);
    
  if(checkIncomingPacketIntegrity()){
    Serial.println("Packet is Correct!");
    //debugSavedIncomingPacket();
    return true;
  }else{
    //Serial.println("!!Packet is Corrupted!!");
    resetCurrIncomingPacket(); //Пакет поврежден - очищаем буфер, в который он был записан
    return false;
  }
}

void displayIncomingData()
{
  switch(currPacketType){
    case TYPE_PACKET::DATA:
      if(currWorkMode == WORK_MODE::SHOW_METEO_DATA && 
         isCompleteDataPacket(currDisplayedModuleId)){
        updateDisplay();  
      }
    break;
    case TYPE_PACKET::SERVICE:
    {
      ActionServicePacket* const servicePacket = getServicePacket(currPacketModuleId);
      printDisplayModuleServiceMsg(static_cast<SERVICE_MSG_TYPE>(servicePacket->id), servicePacket->valueParam);
      updateDisplay();
      break;
    }
    default:
    break;
  }
}

bool processReceivedServicePacket(bool (*handlerPacket)(ActionServicePacket*))
{
  const unsigned long currTime = millis();

  while(true)
  {
    if(radio.available())
    {
      const bool isCorrectData = processIncomingData();

      if(isCorrectData && isCompleteServicePacket(currDisplayedModuleId))
      { 
        return handlerPacket(getServicePacket(currDisplayedModuleId));
      }
    }

    if((millis() - currTime) > WAIT_ACK_REC_COMMAND_MSEC){
      return false;
    }

    delay(LOOP_DELAY_MSEC);
  }
}

void sendActionPacket(ActionServicePacket* actionPacket)
{
  //Делаем несколько попыток отправки. Если так и не получилось получить подтверждение, сообщаем об этом
  for(int i = 0; i < COUNT_ATTEMPT_SEND_ACTION; i++){
    const bool resultAttempt = attemptSendActionPacket(actionPacket);
    if(resultAttempt){
      return;
    }
  }

  printDisplayFailDeliveryCommand();
}

bool attemptSendActionPacket(ActionServicePacket* actionPacket)
{
  radio.stopListening();
  radio.write(actionPacket, ACTSERV_PACKET_LENGTH, true);
  radio.startListening();

  const bool wasReceivedResponse = processReceivedServicePacket(handlerCorrectServicePacket);
  if(!wasReceivedResponse){
    return false;
  }

  const COMMANDS_TYPE commandType = static_cast<COMMANDS_TYPE>(actionPacket->id);
  if(commandType == COMMANDS_TYPE::GET_TIME_INTERVAL ||
     commandType == COMMANDS_TYPE::GET_LIFE_TIME)
  {
    return processReceivedServicePacket(readModuleServiceParam);
  } 
  else
  {
    printDisplayModuleServiceMsg(SERVICE_MSG_TYPE::SUCCESS_GET_COMMAND);
    return true; 
  }
}

bool handlerCorrectServicePacket(ActionServicePacket* servicePacket)
{
  const SERVICE_MSG_TYPE msgType = static_cast<SERVICE_MSG_TYPE>(servicePacket->id);
  if(msgType == SERVICE_MSG_TYPE::GET_ERROR_COMMAND){    
    printDisplayModuleServiceMsg(msgType, servicePacket->valueParam);
  }

  return msgType == SERVICE_MSG_TYPE::SUCCESS_GET_COMMAND;
}

bool readModuleServiceParam(ActionServicePacket* servicePacket)
{
  const SERVICE_MSG_TYPE msgType = static_cast<SERVICE_MSG_TYPE>(servicePacket->id);
  if(msgType == SERVICE_MSG_TYPE::REPORT_TIME_INTERVAL ||
     msgType == SERVICE_MSG_TYPE::REPORT_LIFE_TIME){
      printDisplayModuleServiceMsg(msgType, servicePacket->valueParam);
      return true;
  }

  return false;
}

bool analyzeIncomingPacket(void* packet)
{  
  HeaderPacket headIncomPack;
  memmove(&headIncomPack, packet, sizeof(HeaderPacket));
  if(headIncomPack.dest != MODULE_ID::CENTRAL_MODULE_ID) 
    return false; //Проверка, что данный пакет предназначен главному модулю

  if(headIncomPack.sender == MODULE_ID::INTERNAL_MODULE_ID || headIncomPack.sender == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    currPacketModuleId = static_cast<MODULE_ID>(headIncomPack.sender);
  }
  else
  {
    currPacketModuleId = MODULE_ID::INCORRECT_MODULE_ID;
    return false;
  }

  if(headIncomPack.type == TYPE_PACKET::DATA || headIncomPack.type == TYPE_PACKET::SERVICE)
  {
    currPacketType = static_cast<TYPE_PACKET>(headIncomPack.type);
  }
  else
  {
    currPacketType = TYPE_PACKET::UNKNOWN;
    return false;
  }

  return true;
}

//Сохраняем данные в нужный буфер в зависимости от результатов проведенного ранее анализа
void saveIncomingData(void* packet)
{
  resetCurrIncomingPacket(); //очищаем старые данные

  switch(currPacketType){
    case TYPE_PACKET::DATA:{
      MeteoDataPacket* dataPacket = getMeteoDataPacket(currPacketModuleId);
      memmove(dataPacket, packet, DATA_PACKET_LENGTH);
      break;
    }
    case TYPE_PACKET::SERVICE:{
      ActionServicePacket* servicePacket = getServicePacket(currPacketModuleId);
      memmove(servicePacket, packet, ACTSERV_PACKET_LENGTH);
      break;
    }
    default:
      break;
  }
}

//Проверяем целостность сохраненных данных, вычисляя контрольную сумму
bool checkIncomingPacketIntegrity()
{
  switch(currPacketType){
    case TYPE_PACKET::DATA:
    {
      return isCompleteDataPacket(currPacketModuleId);
    }
    case TYPE_PACKET::SERVICE:
    {
      return isCompleteServicePacket(currPacketModuleId);
    }
    default:
      return false;
  }
}

bool isCompleteDataPacket(MODULE_ID moduleId)
{
  MeteoDataPacket* dataPacket = getMeteoDataPacket(moduleId);
  if(!dataPacket) 
    return false;

  const uint32_t checkSum = calcCheckSum(dataPacket, DATA_PACKET_LENGTH);
  return dataPacket->ckSum == checkSum && dataPacket->type == TYPE_PACKET::DATA;
}

bool isCompleteServicePacket(MODULE_ID moduleId)
{
  ActionServicePacket* const servicePacket = getServicePacket(moduleId);
  if(!servicePacket) 
    return false;

  const uint32_t checkSum = calcCheckSum(servicePacket, ACTSERV_PACKET_LENGTH);
  return servicePacket->ckSum == checkSum && servicePacket->type == TYPE_PACKET::SERVICE;
}

void resetCurrIncomingPacket()
{
  if(currPacketType == TYPE_PACKET::DATA){
    resetCurrDataBuffer();
  }else if(currPacketType == TYPE_PACKET::SERVICE){
    resetCurrServiceBuffer();
  }
}

void resetCurrDataBuffer()
{
  resetDataBuffer(currPacketModuleId);
}

void resetCurrServiceBuffer()
{
  resetServiceBuffer(currPacketModuleId);
}

void resetDataBuffer(MODULE_ID moduleId)
{
  MeteoDataPacket* dataPacket = getMeteoDataPacket(moduleId);
  if(!dataPacket) 
    return;

  memset(dataPacket, 0, DATA_PACKET_LENGTH);
}

void resetServiceBuffer(MODULE_ID moduleId)
{
  ActionServicePacket* servicePacket = getServicePacket(moduleId);
  if(!servicePacket) 
    return;

  memset(servicePacket, 0, ACTSERV_PACKET_LENGTH);
}

void resetIncomingDataBuffers()
{
  resetDataBuffer(INTERNAL_MODULE_ID);
  resetDataBuffer(EXTERNAL_MODULE_ID);
  resetServiceBuffer(INTERNAL_MODULE_ID);
  resetServiceBuffer(EXTERNAL_MODULE_ID);
}

void fillActionPacket(COMMANDS_TYPE commandId, ActionServicePacket* actionPacket)
{
  actionPacket->dest = currDisplayedModuleId;
  actionPacket->sender = MODULE_ID::CENTRAL_MODULE_ID;
  actionPacket->type = TYPE_PACKET::CONTROL;
  actionPacket->id = commandId;
  actionPacket->valueParam = -1; //К сожалению, придумать адекватный способ задавать параметр команде не удалось.
  actionPacket->numPacket = currNumOutPacket;
  actionPacket->ckSum = calcCheckSum(actionPacket, ACTSERV_PACKET_LENGTH);

  currNumOutPacket = (currNumOutPacket < MAX_PACKET_NUMBER ? currNumOutPacket + 1 : 0);
}
