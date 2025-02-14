#include <Adafruit_AHT10.h>
#include <Adafruit_BMP280.h>
#include <general.h>

#define COUNT_DETECTOR             6
#define COUNT_SEND_DATA_INTERVALS  4

#define PIPE_READ_ADDRESS          0xF0F0F0F0E1LL
#define PIPE_WRITE_ADDRESS         0xF0F0F0F0E2LL

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
float numberPacket;
unsigned long prevTime;
bool sendTmData;

//Главные функции
void setup() {
  Serial.begin(SERIAL_SPEED);
  pinMode(LED_PORT, OUTPUT);
  numberPacket = 0;
  prevTime = 0;
  currSendDataIntervalIndex = 2; //По умолчанию используем интервал 10сек (10 000 мсек)
  sendTmData = true;

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
  
  delay(SETUP_DELAY);
  turnOffLED();

  //Сообщаем главному модулю, что мы успешно запустились
  fillServicePacketSMS(servicePacket);
  sendPacketService(servicePacket);
  Serial.println("=== SUCCESS SETUP ===");
}

void loop() {
  if(sendTmData && (millis() - prevTime) >= sendDataIntervalsMsec[currSendDataIntervalIndex]){
    Serial.println("=== FORM DATA PACKET ===");
    //формируем массив данных
    fillDataPacket((float**)dataPacket);
    //отправляем массив метеоданных

    Serial.println("=== SEND DATA PACKET ===");
    debugDataPacket((float**)dataPacket);
    sendPacketData((float**)dataPacket);

    prevTime = millis();
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

  if(packet[0] != MODULE_ID::EXTERNAL_MODULE_ID) return; //Проверка, что данный пакет предназначен текущему модулю
  if(packet[1] != MODULE_ID::CENTRAL_MODULE_ID) return; //Проверка, что данный пакет поступил от главного модуля
  if(packet[2] != TYPE_PACKET::CONTROL) return; //Проверка, что данный пакет имеет тип 'управляющий'
  if(packet[7] != calcCheckSum(packet, DATA_SEGMENT_LENGTH)) return; //Проверка на контрольную сумму пакета

  const COMMANDS_TYPE type = static_cast<COMMANDS_TYPE>(packet[3]);
  Serial.println((String)"PACKET ACTION TYPE : " + type);
  if(type >= COMMANDS_TYPE::RESTART_ALL && type <= COMMANDS_TYPE::HEARTBEAT){
    sendReceipt();
  }

  switch(type){
    case COMMANDS_TYPE::RESTART_ALL:
      restartAll();
    break;
    case COMMANDS_TYPE::STOP_START_SEND:
      sendTmData = !sendTmData;
    break;
    case COMMANDS_TYPE::CHANGE_SEND_INTERVAL:
      changeSendInterval(); //Параметр можно менять на одно из фиксированного набора значений по кругу 
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
  packet[0]=MODULE_ID::CENTRAL_MODULE_ID;
  packet[1]=MODULE_ID::EXTERNAL_MODULE_ID;
  packet[2]=TYPE_PACKET::SERVICE;
  packet[5]=numberPacket;
  packet[6]=-1;
  packet[7]=calcCheckSum(packet, DATA_SEGMENT_LENGTH);
}

void fillServicePacketSMS(float* packet){
  packet[3] = SERVICE_MSG_TYPE::START_MODULE_SUCCESS;
  packet[4] = MODULE_ID::EXTERNAL_MODULE_ID;

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

void fillServicePacketRTI(float* packet, int currentTimeIntervalMsec){
  packet[3] = SERVICE_MSG_TYPE::REPORT_TIME_INTERVAL;
  packet[4] = (sendTmData ? currentTimeIntervalMsec : -1);

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
  dataArray[0][0] = MODULE_ID::CENTRAL_MODULE_ID;
  dataArray[0][1] = MODULE_ID::EXTERNAL_MODULE_ID;
  dataArray[0][2] = TYPE_PACKET::DATA;

  memmove(dataArray[1], dataArray[0], 3 * sizeof(float)); //Копируем данные в оставшиеся сегменты пакета
  memmove(dataArray[2], dataArray[0], 3 * sizeof(float));

  dataArray[0][3] = getTemperatureValue();
  dataArray[0][4] = getHumidityValue();
  dataArray[1][3] = getRainValue();
  dataArray[1][4] = getPressureValue();
  dataArray[2][3] = getSolarValue();
  dataArray[2][4] = getUVValue();

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
