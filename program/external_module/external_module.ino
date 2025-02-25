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

uint8_t currSendDataIntervalIndex;
unsigned long sendDataIntervalsMsec[COUNT_SEND_DATA_INTERVALS]{1000, 5000, 10000, 60000}; //Возможные интервалы отправки данных
MeteoDataPacket dataPacket;
ActionServicePacket servicePacket;
ActionServicePacket actionPacket;
uint32_t numberPacket;
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

  //Запускаем, настраиваем все датчики
  turnOnLED();
  startRadio();

  startAHT10();
  startBMP280();
  
  delay(SETUP_DELAY);
  turnOffLED();

  //Сообщаем главному модулю, что мы успешно запустились
  fillServicePacketSMS(&servicePacket);
  sendPacketService(&servicePacket);
  Serial.println("=== SUCCESS SETUP ===");
}

void loop() {
  if(sendTmData && (millis() - prevTime) >= sendDataIntervalsMsec[currSendDataIntervalIndex]){
    Serial.println("=== FORM DATA PACKET ===");
    //формируем массив данных
    fillDataPacket(&dataPacket);
    //отправляем массив метеоданных

    Serial.println("=== SEND DATA PACKET ===");
    debugDataPacket(&dataPacket);
    sendPacketData(&dataPacket);

    prevTime = millis();
  }

   Serial.println("=== READ PACKET ===");
 
  if(radio.available()){
    Serial.println("=== PACKET INCOMING ===");
    //пытаемся читать входящие данные                              
    radio.read(&actionPacket, ACTSERV_PACKET_LENGTH);
    //анализируем входные данные
    analyzeIncomingPacket(&actionPacket);  
  }  
  
  delay(LOOP_DELAY_MSEC); //Обязательная задержка
  Serial.println("=== END ITERATION ===");
}

void analyzeIncomingPacket(ActionServicePacket* packet){
  Serial.println("=== START ANALYZE INCOMING ===");
  debugActionPacket(packet);

  if(packet->dest != MODULE_ID::EXTERNAL_MODULE_ID) return; //Проверка, что данный пакет предназначен текущему модулю
  if(packet->sender != MODULE_ID::CENTRAL_MODULE_ID) return; //Проверка, что данный пакет поступил от главного модуля
  if(packet->type != TYPE_PACKET::CONTROL) return; //Проверка, что данный пакет имеет тип 'управляющий'
  if(packet->ckSum != calcCheckSum(packet, ACTSERV_PACKET_LENGTH)) return; //Проверка на контрольную сумму пакета

  const COMMANDS_TYPE commandId = static_cast<COMMANDS_TYPE>(packet->id);
  Serial.println((String)"PACKET ACTION ID : " + commandId);
  if(commandId >= COMMANDS_TYPE::RESTART_ALL && commandId <= COMMANDS_TYPE::HEARTBEAT){
    sendReceipt();
  }

  switch(commandId){
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
      fillServicePacketGEC(&servicePacket, &actionPacket);
      sendPacketService(&servicePacket);
    break;
  }
}

void sendPacketData(MeteoDataPacket* packet){
  radio.stopListening();
  radio.write(packet, DATA_PACKET_LENGTH);
  radio.startListening();
}

void sendPacketService(ActionServicePacket* packet){
  debugServicePacket(packet);
  radio.stopListening();
  radio.write(packet, ACTSERV_PACKET_LENGTH);
  radio.startListening();
}

//Отправка квитанции о получении пакета управления (Отсылается перед выполнением какого-либо требуемого действия)
void sendReceipt(){
  fillServicePacketSGC(&servicePacket, &actionPacket);
  sendPacketService(&servicePacket);
}

//Функции формирования служебных пакетов
void fillHeaderAndTailServicePacket(ActionServicePacket* packet){
  packet->dest = MODULE_ID::CENTRAL_MODULE_ID;
  packet->sender = MODULE_ID::EXTERNAL_MODULE_ID;
  packet->type = TYPE_PACKET::SERVICE;
  packet->numPacket = numberPacket;
  packet->ckSum = calcCheckSum(packet, ACTSERV_PACKET_LENGTH);
}

void fillServicePacketSMS(ActionServicePacket* packet){
  packet->id = SERVICE_MSG_TYPE::START_MODULE_SUCCESS;
  packet->valueParam = MODULE_ID::EXTERNAL_MODULE_ID;

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketSGC(ActionServicePacket* packet, ActionServicePacket* actionPacket){
  packet->id = SERVICE_MSG_TYPE::SUCCESS_GET_COMMAND;
  packet->valueParam = actionPacket->id;

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketESD(ActionServicePacket* packet, short numErrDetector){
  packet->id = SERVICE_MSG_TYPE::ERROR_START_DETECTOR;
  packet->valueParam = numErrDetector;

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketRTI(ActionServicePacket* packet, int currentTimeIntervalMsec){
  packet->id = SERVICE_MSG_TYPE::REPORT_TIME_INTERVAL;
  packet->valueParam = (sendTmData ? currentTimeIntervalMsec : -1);

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketRLT(ActionServicePacket* packet){
  packet->id = SERVICE_MSG_TYPE::REPORT_LIFE_TIME;
  packet->valueParam = millis();

  fillHeaderAndTailServicePacket(packet);
}

void fillServicePacketGEC(ActionServicePacket* packet, ActionServicePacket* actionPacket){
  packet->id = SERVICE_MSG_TYPE::GET_ERROR_COMMAND;
  packet->valueParam = actionPacket->id;

  fillHeaderAndTailServicePacket(packet);
}

//Функции-реакции на управляющие пакеты
void restartAll(){
  memset(&dataPacket, 0, DATA_PACKET_LENGTH);
  memset(&servicePacket, 0, ACTSERV_PACKET_LENGTH);
  memset(&actionPacket, 0, ACTSERV_PACKET_LENGTH);

  stopRadio();
  setup();
}

void changeSendInterval(){
  currSendDataIntervalIndex = (currSendDataIntervalIndex > 0) ? currSendDataIntervalIndex - 1 : COUNT_SEND_DATA_INTERVALS - 1;
}

void returnCurrentTimeInterval(){
  fillServicePacketRTI(&servicePacket, sendDataIntervalsMsec[currSendDataIntervalIndex]);
  sendPacketService(&servicePacket);
}

void returnCurrentLifeTime(){
  fillServicePacketRLT(&servicePacket);
  sendPacketService(&servicePacket);
}

void heartbeatReaction(){
  turnOnLED();
  delay(1000);
  turnOffLED();
}

//Функция формирования пакета данных
void fillDataPacket(MeteoDataPacket* dataPacket){
  dataPacket->dest = MODULE_ID::CENTRAL_MODULE_ID;
  dataPacket->sender = MODULE_ID::EXTERNAL_MODULE_ID;
  dataPacket->type = TYPE_PACKET::DATA;

  dataPacket->val1 = getTemperatureValue();
  dataPacket->val2 = getHumidityValue();
  dataPacket->val3 = getRainValue();
  dataPacket->val4 = getPressureValue();
  dataPacket->val5 = getSolarValue();
  dataPacket->val6 = getUVValue();

  dataPacket->numPacket = numberPacket;

  const uint32_t resCkSum = calcCheckSum(dataPacket, DATA_PACKET_LENGTH);

  dataPacket->ckSum = resCkSum;
  
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
  radio.begin();
  radio.setChannel(RADIO_CHANNEL_NUMBER);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.enableDynamicPayloads();
  radio.openReadingPipe(1,PIPE_READ_ADDRESS);
  radio.openWritingPipe(PIPE_WRITE_ADDRESS); //Открываем трубу для отправки метеоданных
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
      fillServicePacketESD(&servicePacket, 1);
      sendPacketService(&servicePacket); //Послать сообщение об ошибке
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
      fillServicePacketESD(&servicePacket, 0);
      sendPacketService(&servicePacket); //Послать сообщение об ошибке
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
