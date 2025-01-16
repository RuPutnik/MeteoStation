#include <SPI.h>
#include <RF24.h>
#include <SD.h>
#include <microDS3231.h>
#include <LiquidCrystal_I2C.h>

#include <string.h>

#define LOOP_DELAY_MSEC            10
#define KEY_POSTHANDLE_DELAY_MSEC  500       

#define PIPE_READ_ADDRESS          0xF0F0F0F0E2LL
#define PIPE_WRITE_ADDRESS         0xF0F0F0F0E1LL
#define SERIAL_SPEED               9600
#define RADIO_CHANNEL_NUMBER       8
#define DATA_SEGMENT_LENGTH        8
#define DATA_SEGMENT_LENGTH_B      DATA_SEGMENT_LENGTH * sizeof(float)  //32
#define COUNT_SEGMENTS_IN_PACKET   3

#define RADIO_CE_PORT          8 //SPI CE??
#define RADIO_CSN_PORT         9 //SPI CS(N)?
#define LCD2004_ADDRESS        0x3F
#define COUNT_KEYS             5

enum MODULE_ID{
  CENTRAL_MODULE_ID     = 0,
  INTERNAL_MODULE_ID    = 1,
  EXTERNAL_MODULE_ID    = 2,
  INCORRECT_MODULE_ID   = 3
};

enum TYPE_PACKET{
  DATA    = 1,
  CONTROL = 2,
  SERVICE = 3,
  UNKNOWN = 4
};

enum COMMANDS_TYPE{
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

enum SERVICE_MSG_TYPE{
  START_MODULE_SUCCESS = 1,
  SUCCESS_GET_COMMAND  = 2,
  ERROR_START_DETECTOR = 3,
  REPORT_DETECTOR_MAP  = 4,
  REPORT_TIME_INTERVAL = 5,
  REPORT_LIFE_TIME     = 6,
  GET_ERROR_COMMAND    = 7
};

enum WORK_MODE{
  SHOW_METEO_DATA = 0,
  SHOW_COMMANDS   = 1
};

enum SHOW_DATA_MODE{
  HUMAN    = 0,
  SYSTEM   = 1
};

enum KEY_PORT{
  KEY_1   = 4, //top left
  KEY_2   = 3, //bottom left
  KEY_3   = 5, //center
  KEY_4   = 7, //top right
  KEY_5   = 6  //bottom right
};

LiquidCrystal_I2C lcd(LCD2004_ADDRESS, 20, 4);
RF24 radio(RADIO_CE_PORT, RADIO_CSN_PORT);
MicroDS3231 rtc;
Sd2Card card;
SdVolume volume;
SdFile root;

float currIncomingData[DATA_SEGMENT_LENGTH];
float servicePacketExternal[DATA_SEGMENT_LENGTH];
float servicePacketInternal[DATA_SEGMENT_LENGTH];
float actionPacket[DATA_SEGMENT_LENGTH];
float** dataPacketExternal = nullptr;
float** dataPacketInternal = nullptr;

TYPE_PACKET currPacketType = TYPE_PACKET::UNKNOWN;
MODULE_ID currPacketModuleId = INCORRECT_MODULE_ID;
MODULE_ID currDisplayedModuleId = INTERNAL_MODULE_ID;
WORK_MODE currWorkMode  = SHOW_METEO_DATA;
SHOW_DATA_MODE currShowDataMode = SHOW_DATA_MODE::HUMAN;

void setup()
{
  Serial.begin(SERIAL_SPEED);

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
}

void startDisplay()
{
  lcd.init();                       // Инициируем работу с LCD дисплеем
  lcd.backlight();                  // Включаем подсветку LCD дисплея
  lcd.setCursor(0, 0);              // Устанавливаем курсор в позицию (0 столбец, 0 строка)
  lcd.print("LCD");                 // Выводим текст "LCD", начиная с установленной позиции курсора
  lcd.setCursor(0, 2);              // Устанавливаем курсор в позицию (0 столбец, 1 строка)
  lcd.print("www.iarduino.ru");
}

void startRadio()
{
  radio.begin();
  radio.setChannel(RADIO_CHANNEL_NUMBER);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.openReadingPipe(1, PIPE_READ_ADDRESS);
  radio.openWritingPipe(PIPE_WRITE_ADDRESS); //Открываем трубу для отправки
  radio.startListening();
}

void loop(){
  if(radio.available())
  {
    processIncomingData();
  }

  //TODO Использование полученных и проверенных данных
  //TODO Здесь сырые данные должны переводиться в адекватные единицы
  //TODO Не забыть о работе с модулем RTC и SD картой
  //TODO Сервисные пакеты удаляются после логгирования\обработки, пакеты с данными живут до получения нового пакета
  //TODO Тут анализ нажатий клавиш на блоке, отправка управляющих пакетов и отображение полученных данных на дисплее
  //TODO Реализовать: функцию логгирования на сд карту и в сом порт. Реализовать функции получения значений параметров в виде строки. Реализовать функции переключения режима кнопок и текущего модуля
  //TODO Реализовать функции обработки нажатий на кнопки

  delay(LOOP_DELAY_MSEC);
}

void startCardSD()
{

}

void startRTC()
{

}

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

  if(currPacketModuleId == MODULE_ID::INTERNAL_MODULE_ID){
    if(currPacketType == TYPE_PACKET::DATA){
      memmove(dataPacketInternal[static_cast<int>(packet[6])], packet, DATA_SEGMENT_LENGTH_B);
    }else if(currPacketType == TYPE_PACKET::SERVICE){
      memmove(servicePacketInternal, packet, DATA_SEGMENT_LENGTH_B);
    }
  }else if(currPacketModuleId == MODULE_ID::EXTERNAL_MODULE_ID){
    if(currPacketType == TYPE_PACKET::DATA){
      memmove(dataPacketExternal[static_cast<int>(packet[6])], packet, DATA_SEGMENT_LENGTH_B);
    }else if(currPacketType == TYPE_PACKET::SERVICE){
      memmove(servicePacketExternal, packet, DATA_SEGMENT_LENGTH_B);
    }
  }
}

//Проверяем целостность сохраненных данных, вычисляя контрольную сумму
bool checkIncomingDataIntegrity()
{
  if(currPacketModuleId == MODULE_ID::INTERNAL_MODULE_ID){
    if(currPacketType == TYPE_PACKET::DATA){
      const float checkSum = calcFullCheckSum(dataPacketInternal, DATA_SEGMENT_LENGTH);
      return (checkSum == dataPacketInternal[0][7] && checkSum == dataPacketInternal[1][7] && checkSum == dataPacketInternal[2][7]);
    }else if(currPacketType == TYPE_PACKET::SERVICE){
      return calcCheckSum(servicePacketInternal, DATA_SEGMENT_LENGTH) == servicePacketInternal[7];
    }
  }else if(currPacketModuleId == MODULE_ID::EXTERNAL_MODULE_ID){
    if(currPacketType == TYPE_PACKET::DATA){
      const float checkSum = calcFullCheckSum(dataPacketExternal, DATA_SEGMENT_LENGTH);
      return (checkSum == dataPacketExternal[0][7] && checkSum == dataPacketExternal[1][7] && checkSum == dataPacketExternal[2][7]);
    }else if(currPacketType == TYPE_PACKET::SERVICE){
      return calcCheckSum(servicePacketExternal, DATA_SEGMENT_LENGTH) == servicePacketExternal[7];
    }
  }

  return false;
}

bool isCompleteDataPacket(MODULE_ID moduleId)
{
  if(moduleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    return (dataPacketInternal[0][6] == 0 && dataPacketInternal[0][6] == 1 && dataPacketInternal[0][6] == 2);
  }
  else if(moduleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    return (dataPacketExternal[0][6] == 0 && dataPacketExternal[0][6] == 1 && dataPacketExternal[0][6] == 2);
  }

  return false;
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
  if(moduleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
      memset(dataPacketInternal[i], 0, DATA_SEGMENT_LENGTH_B);
    }
  }
  else if(moduleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
      memset(dataPacketExternal[i], 0, DATA_SEGMENT_LENGTH_B);
    }
  }
}

void resetServiceBuffer(MODULE_ID moduleId)
{
  if(moduleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    memset(servicePacketInternal, 0, DATA_SEGMENT_LENGTH_B);
  }
  else if(moduleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    memset(servicePacketExternal, 0, DATA_SEGMENT_LENGTH_B);
  }
}

void resetIncomingDataBuffers()
{
  resetDataBuffer(INTERNAL_MODULE_ID);
  resetDataBuffer(EXTERNAL_MODULE_ID);
  resetServiceBuffer(INTERNAL_MODULE_ID);
  resetServiceBuffer(EXTERNAL_MODULE_ID);
}
