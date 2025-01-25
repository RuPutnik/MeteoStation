#include <SPI.h>
#include <RF24.h>
#include <SD.h>
#include <microDS3231.h>
#include <LiquidCrystal_I2C.h>

#include <string.h>

#define LOOP_DELAY_MSEC            10
#define KEY_POSTHANDLE_DELAY_MSEC  1000
#define SETUP_DELAY                2000 

#define PIPE_READ_ADDRESS          0xF0F0F0F0E2LL
#define PIPE_WRITE_ADDRESS         0xF0F0F0F0E1LL
#define SERIAL_SPEED               9600
#define RADIO_CHANNEL_NUMBER       8
#define DATA_SEGMENT_LENGTH        8
#define DATA_SEGMENT_LENGTH_B      DATA_SEGMENT_LENGTH * sizeof(float)  //32
#define COUNT_SEGMENTS_IN_PACKET   3

#define RADIO_CE_PIN               8
#define RADIO_CSN_PIN              9
#define LCD2004_ADDRESS            0x3F
#define COUNT_KEYS                 5
#define SD_CARD_CSN_PIN            10

#define LOG_FILENAME               "MeteoData.log"

enum MODULE_ID: char
{
  CENTRAL_MODULE_ID     = 0,
  INTERNAL_MODULE_ID    = 1,
  EXTERNAL_MODULE_ID    = 2,
  INCORRECT_MODULE_ID   = 3
};

enum TYPE_PACKET: char
{
  DATA    = 1,
  CONTROL = 2,
  SERVICE = 3,
  UNKNOWN = 4
};

enum COMMANDS_TYPE: char
{
  RESTART_ALL          = 1,
  TURNOFF_RADIO        = 2,
  CHANGE_SEND_INTERVAL = 3,
  STOP_SEND_DATA       = 4,
  RESUME_SEND_DATA     = 5,
  GET_DETECTOR_MAP     = 6,
  GET_TIME_INTERVAL    = 7,
  GET_LIFE_TIME        = 8, //в миллисек.
  HEARTBEAT            = 9
};

enum SERVICE_MSG_TYPE: char
{
  START_MODULE_SUCCESS = 1,
  SUCCESS_GET_COMMAND  = 2,
  ERROR_START_DETECTOR = 3,
  REPORT_DETECTOR_MAP  = 4,
  REPORT_TIME_INTERVAL = 5,
  REPORT_LIFE_TIME     = 6,
  GET_ERROR_COMMAND    = 7
};

enum WORK_MODE: char
{
  SHOW_METEO_DATA = 0,
  SHOW_COMMANDS   = 1
};

enum SHOW_DATA_MODE: char
{
  CLASSIC       = 0,
  ALTERNATIVE   = 1
};

enum KEY_PORT: char
{
  KEY_1   = 4, //center
  KEY_2   = 3, //topleft
  KEY_3   = 5, //bottomRight
  KEY_4   = 2, //bottomLeft
  KEY_5   = 6  //topRight
};

enum LOG_MSG_TYPE: char
{
  METEODATA = 0,
  INFO      = 1,
  ERROR     = 2
};

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
COMMANDS_TYPE currCommand = COMMANDS_TYPE::TURNOFF_RADIO;
short currMeteoParamExternal;
short currMeteoParamInternal;
float currNumOutPacket;
bool sdCardInitialized;

void setup()
{
  Serial.begin(SERIAL_SPEED);
  //rtc.setTime(31, 32, 21, 25, 1, 2025);
  currNumOutPacket = 0;
  currMeteoParamExternal = 1;
  currMeteoParamInternal = 1;
  sdCardInitialized = false;
  pinMode(RADIO_CSN_PIN, OUTPUT);
  pinMode(SD_CARD_CSN_PIN, OUTPUT);

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
  
  startDisplay();
  startRadio();
  startSdCard();
  initializeButtons();

  fillActionPacket(COMMANDS_TYPE::RESTART_ALL, actionPacket);
  debugActionPacket(actionPacket);

  delay(SETUP_DELAY);
}

void startRadio()
{
  activateRadio(); //Переключаем SPI на работу с радиомодулем

  radio.begin();
  radio.setChannel(RADIO_CHANNEL_NUMBER);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.openReadingPipe(1, PIPE_READ_ADDRESS);
  radio.openWritingPipe(PIPE_WRITE_ADDRESS); //Открываем трубу для отправки
  radio.startListening();
}

void loop()
{
  activateRadio(); //Переключаем SPI на работу с радиомодулем
  if(radio.available())
  {
    processIncomingData();

    if(isCompleteDataPacket(currDisplayedModuleId)){
      updateDisplay();
    }
  }

  //TODO Использование полученных и проверенных данных
  //TODO Здесь сырые данные должны переводиться в адекватные единицы
  //TODO Не забыть о работе с модулем RTC и SD картой
  //TODO Сервисные пакеты удаляются после логгирования\обработки, пакеты с данными живут до получения нового пакета
  //TODO Тут анализ нажатий клавиш на блоке, отправка управляющих пакетов и отображение полученных данных на дисплее
  //TODO Реализовать: функцию логгирования на сд карту и в сом порт. Реализовать функции получения значений параметров в виде строки. Реализовать функции переключения режима кнопок и текущего модуля
  //TODO Реализовать функции обработки нажатий на кнопки
  buttonsHandler();
  delay(LOOP_DELAY_MSEC);
}

void startSdCard()
{
  activateSdCard();
  if(!SD.begin(SD_CARD_CSN_PIN)) {
    Serial.println("Initialization SD card failed");
    return;
  }

  sdCardInitialized = true;
}
/*
void startRTC()
{
  //Запросить время из потока ввода, если возможно. Иначе продолжить работу как есть 
  if(!Serial){
    return;
  }

  Serial.println("----- MeteoStation -----");
  Serial.println("Press <E> and actual date and time in format dd.MM.yyyy.hh.mm.ss for update");
  Serial.println("Press <R> for start work station");
  Serial.println("Press <P> for print station date and time");
  Serial.println("Wait user input...");

  //Если поток ввода доступен, также можно с помощью ввода команды ввести новое время, продолжить запуск как есть или попросить вывести текущее время
  while(true){
    const int amountAvailable = Serial.available();
    if(amountAvailable > 0){
      char currChar = Serial.read();
      int currIndex = 0;
      char userInput[amountAvailable];
      while(currChar != '\n'){
        userInput[currIndex] = currChar;
        currIndex++;
        currChar = Serial.read();
      }
      userInput[currIndex] = '\0';

      switch(userInput[0]){
        case 'E':
        {
          if(!checkCorrectDateTimeFormat(userInput)){
            Serial.println("Format dateTime was incorrect! Input command and dateTime again!");
            continue;
          }
        
          const int8_t seconds = String{userInput}.substring(19, 21).toInt();
          const int8_t minutes = String{userInput}.substring(16, 18).toInt();
          const int8_t hours = String{userInput}.substring(13, 15).toInt();
          const int8_t days = String{userInput}.substring(2, 4).toInt();
          const int8_t month = String{userInput}.substring(5, 7).toInt();
          const int16_t year = String{userInput}.substring(8, 12).toInt();

          if((seconds > 59 || minutes > 59 || hours > 23)
          || (days > 31 || month > 12 || year > 2099)){
            Serial.println("Part's value of dateTime was incorrect! Input command and dateTime again!");
            continue;
          }

          rtc.setTime(seconds, minutes, hours, days, month, year);
          break;
        }
        case 'R':
          if(currIndex == 1){
            Serial.println("Station is starting...");
            return;
          }else{
            printErrorCommandMessage(userInput);
          }
          break;
        case 'P':
          if(currIndex == 1){
            Serial.println("Current Date and Time: ");
            Serial.println(getCurrDateTime());
          }else{
            printErrorCommandMessage(userInput);
          }
          break;
        default:
          printErrorCommandMessage(userInput);
          break;
      }
    }

    delay(LOOP_DELAY_MSEC);
  }
}

void printErrorCommandMessage(char* command){
  Serial.println("Command is invalid: ");
  Serial.println(command);
  Serial.println("Press <E> and actual date and time in format dd.MM.yyyy.hh.mm.ss for update");
  Serial.println("Press <R> for start work station");
  Serial.println("Press <P> for print station date and time");
}

bool checkCorrectDateTimeFormat(char* dateTime){
  //dd.MM.yyyy.hh.mm.ss
  if(dateTime[1] != ' ') return false;
  if(dateTime[4] != '.' || dateTime[7] != '.' || dateTime[12] != '.' || dateTime[15] != '.' || dateTime[18] != '.') return false;
  if(!isDigit(dateTime[2]) || !isDigit(dateTime[3])) return false;
  if(!isDigit(dateTime[5]) || !isDigit(dateTime[6])) return false;
  if(!isDigit(dateTime[8]) || !isDigit(dateTime[9]) || !isDigit(dateTime[10]) || !isDigit(dateTime[11])) return false;
  if(!isDigit(dateTime[13]) || !isDigit(dateTime[14])) return false;
  if(!isDigit(dateTime[16]) || !isDigit(dateTime[17])) return false;
  if(!isDigit(dateTime[19]) || !isDigit(dateTime[20])) return false;

  return true;
}
*/
void changeWorkMode()
{
  if(currWorkMode == WORK_MODE::SHOW_METEO_DATA){
    currWorkMode = WORK_MODE::SHOW_COMMANDS;
  }else{
    currWorkMode = WORK_MODE::SHOW_METEO_DATA;
  }
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

void sendActionPacket(float* actionPacket)
{
  activateRadio();
  //TODO Реализазовать механизм подтверждения получения управляющего пакета (и, при необходимости, ответного пакета с данными)
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
  if((currPacketType == TYPE_PACKET::DATA && static_cast<int>(packet[6]) == 0) || currPacketType == TYPE_PACKET::SERVICE){
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

  return dataPacket[0][6] == 0 && dataPacket[0][6] == 1 && dataPacket[0][6] == 2;
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
