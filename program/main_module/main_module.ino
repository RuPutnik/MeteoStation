#include <SPI.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>

#include <string.h>

#define LOOP_DELAY_MSEC            10
#define PIPE_READ_ADDRESS          0xF0F0F0F0E2LL
#define PIPE_WRITE_ADDRESS         0xF0F0F0F0E1LL
#define SERIAL_SPEED               9600
#define RADIO_CHANNEL_NUMBER       8
#define DATA_SEGMENT_LENGTH        8
#define DATA_SEGMENT_LENGTH_B      DATA_SEGMENT_LENGTH * sizeof(float)  //32
#define COUNT_SEGMENTS_IN_PACKET   3

#define CENTRAL_MODULE_ID      0
#define INTERNAL_MODULE_ID     1
#define EXTERNAL_MODULE_ID     2
#define INCORRECT_MODULE_ID    3

#define RADIO_CE_PORT          8
#define RADIO_CSN_PORT         9
#define LCD2004_ADDRESS        0x3F

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

LiquidCrystal_I2C lcd(LCD2004_ADDRESS, 20, 4);
RF24 radio(RADIO_CE_PORT, RADIO_CSN_PORT);

float currIncomingData[DATA_SEGMENT_LENGTH];
float servicePacketExternal[DATA_SEGMENT_LENGTH];
float servicePacketInternal[DATA_SEGMENT_LENGTH];
float actionPacket[DATA_SEGMENT_LENGTH];
float** dataPacketExternal = nullptr;
float** dataPacketInternal = nullptr;
TYPE_PACKET currPacketType = TYPE_PACKET::UNKNOWN;
int currPacketModuleId = INCORRECT_MODULE_ID;

//Здесь сырые данные должны переводиться в адекватные единицы
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
  if (!radio.available())
  {
    //Serial.println("No data");
    goto end_loop;
  }
    
  Serial.println("Data Incoming!");
  radio.read(currIncomingData, DATA_SEGMENT_LENGTH_B);

  for(int i = 0; i < DATA_SEGMENT_LENGTH; i++){
    Serial.println(i + (String)": " + currIncomingData[i]);
  }

  const bool validIncomingData = analyzeIncomingPacket(currIncomingData);

  if(!validIncomingData){
    Serial.println("DATA IS INVALID!");
    goto end_loop;
  }

  saveIncomingData(currIncomingData);

  if(currPacketType == TYPE_PACKET::DATA){
    //Если это сегмент пакета данных, анализ проводим только после получения сегмента 2, 
    //иначе будет попытка работы с неполным пакетом данных
    if(currIncomingData[6] != (COUNT_SEGMENTS_IN_PACKET - 1)){
      goto end_loop;
    }
  }
    
  if(checkIncomingDataIntegrity()){
    Serial.println("Packet is Correct!");
    debugSavedIncomingPacket();
  }else{
    Serial.println("!!Packet is Corrupted!!");
    goto end_loop_with_reset;
  }

  //TODO Использование полученных и проверенных данных

end_loop_with_reset:
  resetIncomingDataBuffers();
end_loop:
  delay(LOOP_DELAY_MSEC);
}

bool analyzeIncomingPacket(float* packet)
{
  if(packet[0] != CENTRAL_MODULE_ID) 
    return false; //Проверка, что данный пакет предназначен главному модулю

  if(packet[1] == INTERNAL_MODULE_ID || packet[1] == EXTERNAL_MODULE_ID)
  {
    currPacketModuleId = static_cast<int>(packet[1]);
  }
  else
  {
    currPacketModuleId = INCORRECT_MODULE_ID;
    return false;
  }

  if(packet[2] == TYPE_PACKET::DATA || packet[2] == TYPE_PACKET::SERVICE)
  {
    currPacketType = static_cast<int>(packet[2]);
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
  if(currPacketModuleId == INTERNAL_MODULE_ID){
    if(currPacketType == TYPE_PACKET::DATA){
      memmove(dataPacketInternal[static_cast<int>(packet[6])], packet, DATA_SEGMENT_LENGTH_B);
    }else if(currPacketType == TYPE_PACKET::SERVICE){
      memmove(servicePacketInternal, packet, DATA_SEGMENT_LENGTH_B);
    }
  }else if(currPacketModuleId == EXTERNAL_MODULE_ID){
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
  if(currPacketModuleId == INTERNAL_MODULE_ID){
    if(currPacketType == TYPE_PACKET::DATA){
      const float checkSum = calcFullCheckSum(dataPacketInternal, DATA_SEGMENT_LENGTH);
      return (checkSum == dataPacketInternal[0][7] && checkSum == dataPacketInternal[1][7] && checkSum == dataPacketInternal[2][7]);
    }else if(currPacketType == TYPE_PACKET::SERVICE){
      return calcCheckSum(servicePacketInternal, DATA_SEGMENT_LENGTH) == servicePacketInternal[7];
    }
  }else if(currPacketModuleId == EXTERNAL_MODULE_ID){
    if(currPacketType == TYPE_PACKET::DATA){
      const float checkSum = calcFullCheckSum(dataPacketExternal, DATA_SEGMENT_LENGTH);
      return (checkSum == dataPacketExternal[0][7] && checkSum == dataPacketExternal[1][7] && checkSum == dataPacketExternal[2][7]);
    }else if(currPacketType == TYPE_PACKET::SERVICE){
      return calcCheckSum(servicePacketExternal, DATA_SEGMENT_LENGTH) == servicePacketExternal[7];
    }
  }

  return false;
}

void resetIncomingDataBuffers()
{
  for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
    memset(dataPacketInternal[i], 0, DATA_SEGMENT_LENGTH_B);
  }
  for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
    memset(dataPacketExternal[i], 0, DATA_SEGMENT_LENGTH_B);
  }

  memset(servicePacketInternal, 0, DATA_SEGMENT_LENGTH_B);
  memset(servicePacketExternal, 0, DATA_SEGMENT_LENGTH_B);
}
