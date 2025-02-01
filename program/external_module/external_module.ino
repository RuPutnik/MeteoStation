#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <Adafruit_AHT10.h>
#include <Adafruit_BMP280.h>

#define LOOP_DELAY_MSEC            10
#define COUNT_DETECTOR             6
#define START_DELAY_MSEC           3000
#define PIPE_READ_ADDRESS          0xF0F0F0F0E1LL
#define PIPE_WRITE_ADDRESS         0xF0F0F0F0E2LL
#define SERIAL_SPEED               9600
#define RADIO_CHANNEL_NUMBER       8
#define DATA_SEGMENT_LENGTH        8
#define DATA_SEGMENT_LENGTH_B      (DATA_SEGMENT_LENGTH * sizeof(float))  //32
#define COUNT_SEGMENTS_IN_PACKET   3
#define COUNT_SEND_DATA_INTERVALS  4

#define CENTRAL_MODULE_ID          0
#define MODULE_ID                  2

#define RAIN_DETECT_PORT           A0
#define TEMT6000_PORT              A1
#define GUAVA_PORT                 A2
#define LED_PORT                   2
#define RADIO_CE_PORT              9
#define RADIO_CSN_PORT             10

Adafruit_AHT10 ahtDetector;
Adafruit_BMP280 bmpDetector;
RF24 radio(RADIO_CE_PORT, RADIO_CSN_PORT);

int currSendDataIntervalIndex;
unsigned long sendDataIntervalsMsec[COUNT_SEND_DATA_INTERVALS]{1000, 5000, 10000, 60000}; //Возможные интервалы отправки данных
float** dataPacket = nullptr;
float servicePacket[DATA_SEGMENT_LENGTH];
float actionPacket[DATA_SEGMENT_LENGTH];
bool detectorMap[COUNT_DETECTOR];
float numberPacket;
unsigned long prevTime;

enum TYPE_PACKET{
  DATA    = 1,
  CONTROL = 2,
  SERVICE = 3
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



//Главные функции
void setup() {
  Serial.begin(SERIAL_SPEED);
  pinMode(LED_PORT, OUTPUT);
  numberPacket = 0;
  prevTime = 0;
  currSendDataIntervalIndex = 2; //По умолчанию используем интервал 10сек (10 000 мсек)
  resetDetectorMap(detectorMap);

  if(!dataPacket){
    dataPacket = new float*[COUNT_SEGMENTS_IN_PACKET];
    for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
      dataPacket[i] = new float[DATA_SEGMENT_LENGTH];
    }
  }

  //Запускаем, настраиваем все датчики
  turnOnLED();
  startRadio();

  startAHT10();
  startBMP280();
  
  delay(START_DELAY_MSEC);
  turnOffLED();

  //Сообщаем главному модулю, что мы успешно запустились
  fillServicePacketSMS(servicePacket);
  sendPacketService(servicePacket);
  Serial.println("=== SUCCESS SETUP ===");
}

void loop() {
  if((millis() - prevTime) >= sendDataIntervalsMsec[currSendDataIntervalIndex]){
    Serial.println("=== FORM DATA PACKET ===");
    //формируем массив данных
    fillDataPacket((float**)dataPacket);
    //отправляем массив метеоданных

    Serial.println("=== SEND DATA PACKET ===");
    debugDataPacket((float**)dataPacket);
    sendPacketData((float**)dataPacket);

    prevTime=millis();
  }

   Serial.println("=== READ PACKET ===");
 
  if(radio.available()){
    Serial.println("=== PACKET INCOMING ===");
    //пытаемся читать входящие данные                              
    radio.read(&actionPacket, DATA_SEGMENT_LENGTH_B);
    //анализируем входные данные
    analyzeIncomingPacket(actionPacket);  
  }  
  
  delay(LOOP_DELAY_MSEC); //Обязательная задержка
  Serial.println("=== END ITERATION ===");
}

void analyzeIncomingPacket(float* packet){
  Serial.println("=== START ANALYZE INCOMING ===");
  debugActionPacket(packet);

  if(packet[0]!=MODULE_ID) return; //Проверка, что данный пакет предназначен текущему модулю
  if(packet[1]!=CENTRAL_MODULE_ID) return; //Проверка, что данный пакет поступил от главного модуля
  if(packet[2]!=TYPE_PACKET::CONTROL) return; //Проверка, что данный пакет имеет тип 'управляющий'
  if(packet[7]!=calcCheckSum(packet, DATA_SEGMENT_LENGTH)) return; //Проверка на контрольную сумму пакета

  sendReceipt(); //Посылаем главному модулю квитанцию, что получили его команду
  Serial.println((String)"PACKET ACTION TYPE : " + (int)packet[3]);
  switch((int)packet[3]){
    case COMMANDS_TYPE::RESTART_ALL:
      restartAll();
    break;
    case COMMANDS_TYPE::TURNOFF_RADIO:
      stopRadio();
    break;
    case COMMANDS_TYPE::CHANGE_SEND_INTERVAL:
      changeSendInterval(); //Параметр можно менять на одно из фиксированного набора значений по кругу 
    break;
    case COMMANDS_TYPE::STOP_SEND_DATA:
      stopSendData(static_cast<int>(packet[4])); //Возможно, не получится реализовать. Сложно вводить числа на блоке управления
    break;
    case COMMANDS_TYPE::RESUME_SEND_DATA:
      resumeSendData((int)packet[4]); //Возможно, не получится реализовать. Сложно вводить числа на блоке управления
    break;
    case COMMANDS_TYPE::GET_DETECTOR_MAP:
      returnDetectorMap();
    break;
    case COMMANDS_TYPE::GET_TIME_INTERVAL:
      returnCurrentTimeInterval();
    break;
    case COMMANDS_TYPE::GET_LIFE_TIME:
      returnCurrentLifeTime();
    break;
    case COMMANDS_TYPE::HEARTBEAT:
      heartbeatReaction();
    break;
    default:
	  //Если тип команды не распознан, посылаем об этом сообщение главному модулю
      fillServicePacketGEC(servicePacket, actionPacket);
      sendPacketService(servicePacket);
    break;
  }
}

void sendPacketData(float** packet){
  radio.stopListening();
  radio.write(packet[0], DATA_SEGMENT_LENGTH_B);
  radio.write(packet[1], DATA_SEGMENT_LENGTH_B);
  radio.write(packet[2], DATA_SEGMENT_LENGTH_B);
  radio.startListening();
}

void sendPacketService(float* packet){
  debugServicePacket(packet);
  radio.stopListening();
  radio.write(packet, DATA_SEGMENT_LENGTH_B);
  radio.startListening();
}

//Отправка квитанции о получении пакета управления (Отсылается перед выполнением какого-либо требуемого действия)
void sendReceipt(){
  fillServicePacketSGC(servicePacket, actionPacket);
  sendPacketService(servicePacket);
}

//Функции формирования служебных пакетов
void fillHeaderAndTailServicePacket(float* packet){
  packet[0]=CENTRAL_MODULE_ID;
  packet[1]=MODULE_ID;
  packet[2]=TYPE_PACKET::SERVICE;
  packet[5]=numberPacket;
  packet[6]=-1;
  packet[7]=calcCheckSum(packet, DATA_SEGMENT_LENGTH);
}

void fillServicePacketSMS(float* packet){
  packet[3] = SERVICE_MSG_TYPE::START_MODULE_SUCCESS;
  packet[4] = 0;

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketSGC(float* packet, float* actionPacket){
  packet[3] = SERVICE_MSG_TYPE::SUCCESS_GET_COMMAND;
  packet[4] = actionPacket[3];

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketESD(float* packet, short numErrDetector){
  packet[3] = SERVICE_MSG_TYPE::ERROR_START_DETECTOR;
  packet[4] = numErrDetector;

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketRDM(float* packet, bool* detectorMap){
  packet[3] = SERVICE_MSG_TYPE::REPORT_DETECTOR_MAP;
  uint32_t bitMapDetector=0;

  for(int i=0; i<COUNT_DETECTOR; i++){
    //Заполняем побитово 4-байтовое целое беззнаковое, оставляя 3 старших байтах и 2 старших бита младшего байта неиспользованными
    bitMapDetector |= (static_cast<uint32_t>(detectorMap[i]) << i);
  }
  packet[4] = bitMapDetector;

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketRTI(float* packet, int currentTimeIntervalMsec){
  packet[3] = SERVICE_MSG_TYPE::REPORT_TIME_INTERVAL;
  packet[4] = currentTimeIntervalMsec;

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketRLT(float* packet){
  packet[3] = SERVICE_MSG_TYPE::REPORT_LIFE_TIME;
  packet[4] = millis();

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketGEC(float* packet, float* actionPacket){
  packet[3] = SERVICE_MSG_TYPE::GET_ERROR_COMMAND;
  packet[4] = actionPacket[3];

  fillHeaderAndTailServicePacket(packet);
}

//Функции-реакции на управляющие пакеты
void restartAll(){
  for(int i = 0; i < COUNT_SEGMENTS_IN_PACKET; i++){
    delete[] dataPacket[i];
  }
  delete[] dataPacket;
  dataPacket = nullptr;

  memset(servicePacket, 0, DATA_SEGMENT_LENGTH_B);
  memset(actionPacket, 0, DATA_SEGMENT_LENGTH_B);

  stopRadio();
  setup();
}

void changeSendInterval(){
  currSendDataIntervalIndex = (currSendDataIntervalIndex > 0) ? currSendDataIntervalIndex - 1 : COUNT_SEND_DATA_INTERVALS - 1;
}

void stopSendData(int numberDetector){
  detectorMap[numberDetector] = false;
}

void resumeSendData(int numberDetector){
  detectorMap[numberDetector] = true;
}

void returnDetectorMap(){
  fillServicePacketRDM(servicePacket, detectorMap);
  sendPacketService(servicePacket);
}

void returnCurrentTimeInterval(){
  fillServicePacketRTI(servicePacket, sendDataIntervalsMsec[currSendDataIntervalIndex]);
  sendPacketService(servicePacket);
}

void returnCurrentLifeTime(){
  fillServicePacketRLT(servicePacket);
  sendPacketService(servicePacket);
}

void heartbeatReaction(){
  turnOnLED();
  delay(1000);
  turnOffLED();
}

//Функция формирования пакета данных
void fillDataPacket(float** dataArray){
  dataArray[0][0] = CENTRAL_MODULE_ID;
  dataArray[0][1] = MODULE_ID;
  dataArray[0][2] = TYPE_PACKET::DATA;

  memmove(dataArray[1], dataArray[0], 3 * sizeof(float)); //Копируем данные в оставшиеся сегменты пакета
  memmove(dataArray[2], dataArray[0], 3 * sizeof(float));

  dataArray[0][3] = -1;
  dataArray[0][4] = -1;
  dataArray[1][3] = -1;
  dataArray[1][4] = -1;
  dataArray[2][3] = -1;
  dataArray[2][4] = -1;

  if(detectorMap[0]){
    dataArray[0][3] = getTemperatureValue();
  }

  if(detectorMap[1]){
    dataArray[0][4] = getHumidityValue();
  }

  if(detectorMap[2]){
    dataArray[1][3] = getRainValue();
  }

  if(detectorMap[3]){
    dataArray[1][4] = getPressureValue();
  }

  if(detectorMap[4]){
    dataArray[2][3] = getSolarValue();
  }

  if(detectorMap[5]){
    dataArray[2][4] = getUVValue();
  }

  dataArray[0][5] = numberPacket;
  dataArray[1][5] = numberPacket;
  dataArray[2][5] = numberPacket;

  dataArray[0][6] = 0;
  dataArray[1][6] = 1;
  dataArray[2][6] = 2;

  float resCkSum = calcFullCheckSum(dataArray, DATA_SEGMENT_LENGTH);

  dataArray[0][7] = resCkSum;
  dataArray[1][7] = resCkSum;
  dataArray[2][7] = resCkSum;
  
  if(numberPacket < 1000000000){
    numberPacket++;
  }else{
    numberPacket = 0;
  }
}

//Функции инициализации и деиницаилизации
void turnOnLED(){
  digitalWrite(LED_PORT, HIGH);
}

void turnOffLED(){
  digitalWrite(LED_PORT, LOW);
}

void startRadio(){
  //То что ниже - надо еще настроить
  radio.begin();
  radio.setChannel(RADIO_CHANNEL_NUMBER);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1,PIPE_READ_ADDRESS);
  radio.openWritingPipe(PIPE_WRITE_ADDRESS); //Открываем трубу для отправки
  radio.startListening();
}

void stopRadio(){
  radio.stopListening();
  radio.closeReadingPipe(1);
}

void startAHT10(){
  int count=10;
  while(!ahtDetector.begin()) {
    delay(10);
    count--;
    if(count<0){
      fillServicePacketESD(servicePacket, 1);
      sendPacketService(servicePacket); //Послать сообщение об ошибке
      Serial.println("ERROR AHT10");
      return;
    }
  }
  Serial.println("SUCCESS AHT10");
}

void startBMP280(){
  int count=10;
  while(!bmpDetector.begin()) {
    delay(10);
    count--;
    if(count<0){
      fillServicePacketESD(servicePacket, 0);
      sendPacketService(servicePacket); //Послать сообщение об ошибке
      Serial.println("ERROR BMP280");
      return;
    }
  }
  Serial.println("SUCCESS BMP280");
  bmpDetector.setSampling(Adafruit_BMP280::MODE_FORCED,// Режим работы
                  Adafruit_BMP280::SAMPLING_X4,        // Точность изм. температуры
                  Adafruit_BMP280::SAMPLING_X16,       // Точность изм. давления
                  Adafruit_BMP280::FILTER_X16,         // Уровень фильтрации
                  Adafruit_BMP280::STANDBY_MS_1000);   // Период просыпания, мСек
}