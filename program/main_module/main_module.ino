#include <microDS3231.h>
#include <LiquidCrystal_I2C.h>
#include <general.h>

#define KEY_POSTHANDLE_DELAY_MSEC    1000
#define WAIT_ACK_REC_COMMAND_MSEC    2000
#define PRINT_SERVICE_MSG_MSEC       1000

#define PIPE_WRITE_ADDRESS           0xF0F0F0F0E1LL
#define PIPE_READ_ADDRESS_EXTERNAL   0xF0F0F0F0E2LL
#define PIPE_READ_ADDRESS_INTERNAL   0xF0F0F0F0E3LL

#define COUNT_SEGMENTS_IN_PACKET   3
#define COUNT_METEO_PARAM_EXTERNAL 6
#define COUNT_METEO_PARAM_INTERNAL 4
#define COUNT_ATTEMPT_SEND_ACTION  3

#define RADIO_CE_PIN               8
#define RADIO_CSN_PIN              9
#define LCD2004_ADDRESS            0x3F
#define COUNT_KEYS                 5

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

float currIncomingData[DATA_SEGMENT_LENGTH];
float servicePacketExternal[DATA_SEGMENT_LENGTH];
float servicePacketInternal[DATA_SEGMENT_LENGTH];
float actionPacket[DATA_SEGMENT_LENGTH];
float** dataPacketExternal = nullptr;
float** dataPacketInternal = nullptr;

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

  if(!dataPacketExternal){
    dataPacketExternal = new float*[COUNT_SEGMENTS_IN_PACKET];
    for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
      dataPacketExternal[i] = new float[DATA_SEGMENT_LENGTH];
    }
  }

  if(!dataPacketInternal){
    dataPacketInternal = new float*[COUNT_SEGMENTS_IN_PACKET];
    for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
      dataPacketInternal[i] = new float[DATA_SEGMENT_LENGTH];
    }
  }
  
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
  radio.openReadingPipe(1, PIPE_READ_ADDRESS_EXTERNAL);
  radio.openReadingPipe(2, PIPE_READ_ADDRESS_INTERNAL);
  radio.startListening();
}

void loop()
{
  if(radio.available())
  {
    processIncomingData();

    if(currWorkMode == WORK_MODE::SHOW_METEO_DATA && 
       isCompleteDataPacket(currDisplayedModuleId)){
      updateDisplay();
    }
  }

  //TODO Сервисные пакеты удаляются после логгирования\обработки, пакеты с данными живут до получения нового пакета
  buttonsHandler();
  updateDisplayHeader(); //Всегда обновляем первую строку, т.к. там выводится текущее время
  delay(LOOP_DELAY_MSEC);
}

void processIncomingData()
{
  Serial.println("Data Incoming!");
  radio.read(currIncomingData, DATA_SEGMENT_LENGTH_B);

  for(int i = 0; i < DATA_SEGMENT_LENGTH; i++){
    Serial.println(i + (String)": " + currIncomingData[i]);
  }

  const bool validIncomingData = analyzeIncomingPacket(currIncomingData);

  if(!validIncomingData){
    Serial.println("DATA IS INVALID!");
    return;
  }

  saveIncomingData(currIncomingData);

  if(currPacketType == TYPE_PACKET::DATA){
    //Если это сегмент пакета данных, анализ проводим только после получения сегмента 2, 
    //иначе будет попытка работы с неполным пакетом данных
    if(currIncomingData[6] != (COUNT_SEGMENTS_IN_PACKET - 1)){
      return;
    }
  }
    
  if(checkIncomingDataIntegrity()){
    Serial.println("Packet is Correct!");
    debugSavedIncomingPacket();
  }else{
    Serial.println("!!Packet is Corrupted!!");
    resetCurrIncomingPacket(); //Пакет поврежден - очищаем буфер, в который он был записан
  }
}

bool processReceivedServicePacket(bool (*handlerPacket)(float*))
{
  const unsigned long currTime = millis();

  while(true)
  {
    if(radio.available())
    {
      processIncomingData();
      float* const servicePacket = getServicePacket(currDisplayedModuleId);

      if(servicePacket[6] == -1 && checkIncomingDataIntegrity())
      { //Проверяем на -1, т.к. это значит, что данные пакета записались в массив текущего модуля. 
        //В этом элементе массива по протоколу всегда -1. А если запись не произошла, то там будет начальный 0
        return handlerPacket(servicePacket);
      }
    }

    if((millis() - currTime) > WAIT_ACK_REC_COMMAND_MSEC){
      return false;
    }

    delay(LOOP_DELAY_MSEC);
  }
}

void sendActionPacket(float* actionPacket)
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

bool attemptSendActionPacket(float* actionPacket)
{
  radio.stopListening();
  radio.write(actionPacket, DATA_SEGMENT_LENGTH_B, true);
  radio.startListening();

  const bool wasReceivedResponse = processReceivedServicePacket(handlerCorrectServicePacket);
  if(!wasReceivedResponse){
    return false;
  }

  const COMMANDS_TYPE commandType = static_cast<COMMANDS_TYPE>(actionPacket[3]);
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

bool handlerCorrectServicePacket(float* servicePacket)
{
  debugServicePacket(servicePacket);
  const SERVICE_MSG_TYPE msgType = static_cast<SERVICE_MSG_TYPE>(servicePacket[3]);
  if(msgType == SERVICE_MSG_TYPE::GET_ERROR_COMMAND){    
    printDisplayModuleServiceMsg(msgType);
  }

  return msgType == SERVICE_MSG_TYPE::SUCCESS_GET_COMMAND;
}

bool readModuleServiceParam(float* servicePacket)
{
  const SERVICE_MSG_TYPE msgType = static_cast<SERVICE_MSG_TYPE>(servicePacket[3]);
  if(msgType == SERVICE_MSG_TYPE::REPORT_TIME_INTERVAL ||
     msgType == SERVICE_MSG_TYPE::REPORT_LIFE_TIME){
      printDisplayModuleServiceMsg(msgType, servicePacket[4]);
      return true;
  }

  return false;
}

bool analyzeIncomingPacket(float* packet)
{
  if(packet[0] != MODULE_ID::CENTRAL_MODULE_ID) 
    return false; //Проверка, что данный пакет предназначен главному модулю

  if(packet[1] == MODULE_ID::INTERNAL_MODULE_ID || packet[1] == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    currPacketModuleId = static_cast<MODULE_ID>(packet[1]);
  }
  else
  {
    currPacketModuleId = MODULE_ID::INCORRECT_MODULE_ID;
    return false;
  }

  if(packet[2] == TYPE_PACKET::DATA || packet[2] == TYPE_PACKET::SERVICE)
  {
    currPacketType = static_cast<TYPE_PACKET>(packet[2]);
  }
  else
  {
    currPacketType = TYPE_PACKET::UNKNOWN;
    return false;
  }

  return true;
}

//Сохраняем данные в нужный буфер в зависимости от результатов проведенного ранее анализа
void saveIncomingData(float* packet)
{
  if((currPacketType == TYPE_PACKET::DATA && static_cast<int>(packet[6]) == 0) || 
     currPacketType == TYPE_PACKET::SERVICE){
    //Если это первый сегмент пакета данных и сервисный пакет, очищаем весь буфер данных или сервисный буфер для данного модуля
    resetCurrIncomingPacket();
  }

  switch(currPacketType){
    case TYPE_PACKET::DATA:{
      float** dataPacket = getMeteoDataPacket(currPacketModuleId);
      memmove(dataPacket[static_cast<int>(packet[6])], packet, DATA_SEGMENT_LENGTH_B);
      break;
    }
    case TYPE_PACKET::SERVICE:{
      float* servicePacket = getServicePacket(currPacketModuleId);
      memmove(servicePacket, packet, DATA_SEGMENT_LENGTH_B);
      break;
    }
    default:
      break;
  }
}

//Проверяем целостность сохраненных данных, вычисляя контрольную сумму
bool checkIncomingDataIntegrity()
{
  switch(currPacketType){
    case TYPE_PACKET::DATA:
    {
      float** dataPacket = getMeteoDataPacket(currPacketModuleId);
      const float checkSum = calcFullCheckSum(dataPacket, DATA_SEGMENT_LENGTH);
      return (checkSum == dataPacket[0][7] && checkSum == dataPacket[1][7] && checkSum == dataPacket[2][7]);
    }
    case TYPE_PACKET::SERVICE:
    {
      float* servicePacket = getServicePacket(currPacketModuleId);
      return calcCheckSum(servicePacket, DATA_SEGMENT_LENGTH) == servicePacket[7];
    }
    default:
      return false;
  }
}

bool isCompleteDataPacket(MODULE_ID moduleId)
{
  float** dataPacket = getMeteoDataPacket(moduleId);
  if(!dataPacket) 
    return false;

  return dataPacket[0][6] == 0 && dataPacket[1][6] == 1 && dataPacket[2][6] == 2;
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
  float** dataPacket = getMeteoDataPacket(moduleId);
  if(!dataPacket) 
    return;

  for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
    memset(dataPacket[i], 0, DATA_SEGMENT_LENGTH_B);
  }
}

void resetServiceBuffer(MODULE_ID moduleId)
{
  float* servicePacket = getServicePacket(moduleId);
  if(!servicePacket) 
    return;

  memset(servicePacket, 0, DATA_SEGMENT_LENGTH_B);
}

void resetIncomingDataBuffers()
{
  resetDataBuffer(INTERNAL_MODULE_ID);
  resetDataBuffer(EXTERNAL_MODULE_ID);
  resetServiceBuffer(INTERNAL_MODULE_ID);
  resetServiceBuffer(EXTERNAL_MODULE_ID);
}

void fillActionPacket(COMMANDS_TYPE commandId, float* actionPacket)
{
  actionPacket[0] = currDisplayedModuleId;
  actionPacket[1] = MODULE_ID::CENTRAL_MODULE_ID;
  actionPacket[2] = TYPE_PACKET::CONTROL;
  actionPacket[3] = commandId;
  actionPacket[4] = -1; //К сожалению, придумать адекватный способ задавать параметр команде не удалось.
  actionPacket[5] = currNumOutPacket;
  actionPacket[6] = -1;
  actionPacket[7] = calcCheckSum(actionPacket, DATA_SEGMENT_LENGTH);

  if(currNumOutPacket < 1000000000){
    currNumOutPacket++;
  }else{
    currNumOutPacket = 0;
  }
}
