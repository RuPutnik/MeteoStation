//Функции для обработки входа (нажатия кнопок) и выхода (работа с дисплеем, запись в SD карту)

void startDisplay()
{
  lcd.init(); //Инициируем работу с LCD дисплеем
  lcd.backlight(); //Включаем подсветку LCD дисплея
  lcd.setCursor(0, 0);
  lcd.print("--------------------");
  lcd.setCursor(4, 1);
  lcd.print("MeteoStation");
  lcd.setCursor(3, 2);
  lcd.print("Starting up...");
  lcd.setCursor(0, 3);
  lcd.print("--------------------");
}

void updateDisplay()
{
  lcd.setCursor(0, 3);
  lcd.print(getCurrDateTime());
  Serial.println(getCurrDateTime());
  //Анализировать тек.модуль, режим работы, текущую команду/метеопараметр
}

struct KeyBoundle
{
  KEY_PORT port;
  void (*handlerPtr)();
};

KeyBoundle keyBundles[COUNT_KEYS];

void addButtonHandler(KEY_PORT port, void (*handlerPtr)()){
  static int currPortIndex = 0;

  if(currPortIndex < COUNT_KEYS){
    keyBundles[currPortIndex] = KeyBoundle{port, handlerPtr};
    currPortIndex++;
  }
}

void initializeButtons()
{
  pinMode(KEY_1, INPUT);
  pinMode(KEY_2, INPUT);
  pinMode(KEY_3, INPUT);
  pinMode(KEY_4, INPUT);
  pinMode(KEY_5, INPUT);

  addButtonHandler(KEY_1, centerButtonHandler);
  addButtonHandler(KEY_2, topLeftButtonHandler);
  addButtonHandler(KEY_3, bottomRightButtonHandler);
  addButtonHandler(KEY_4, bottomLeftButtonHandler);
  addButtonHandler(KEY_5, topRightButtonHandler);
}

void buttonsHandler()
{
  for(int i = 0; i < COUNT_KEYS; i++){
    const int portValue = digitalRead(keyBundles[i].port);
    if(portValue == HIGH){
      keyBundles[i].handlerPtr();
      updateDisplay();
      //Для предотвращения многократного срабатывания одной и той же кнопки при хотя бы небольшом зажатии
      delay(KEY_POSTHANDLE_DELAY_MSEC);
    }
  }
}

void topRightButtonHandler()
{
  //Данные назад / команда назад
  Serial.println("TopRight press");

}

void bottomRightButtonHandler()
{
  //Данные вперед / команда вперед
  Serial.println("BottomRight press");

}

void centerButtonHandler()
{
  //Смена модуля
  Serial.println("Center press");

}

void topLeftButtonHandler()
{
  //Войти-выйти в режим управления
  Serial.println("TopLeft press");

}

void bottomLeftButtonHandler()
{
  //Формат отображения значений / Отправить команду
  Serial.println("BottomLeft press");

}

String formTemperatureMsg()
{
  float temperatureValue;
  String scaleName;

  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    temperatureValue = dataPacketInternal[0][3];
  }
  else if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    temperatureValue = dataPacketExternal[0][3];
  }
  else
  {
    return "T = <?>";
  }

  if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
  {
    scaleName = "C";
  }
  else
  {
    temperatureValue = temperatureValue * 1.8 + 32; //Перевод из цельсия в фаренгейты
    scaleName = "F";
  }

  return "T = " + static_cast<String>(temperatureValue) + scaleName;
} 

String formPressureMsg()
{
  if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    float pressureValue = dataPacketExternal[1][4];
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      pressureValue = pressureValue / 133.3;
      scaleName = " mmHg";
    }
    else
    {
      scaleName = " Pa";
    }

    return "Pressure = " + static_cast<String>(pressureValue) + scaleName;
  }
  else
  {
    return "Pressure = <?>";
  }
}

//Влажность
String formHumidityMsg()
{
  float humidityValue;

  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    humidityValue = dataPacketInternal[0][4];
  }
  else if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    humidityValue = dataPacketExternal[0][4];
  }
  else
  {
    return "Relative wet = <?>";
  }

  return "Relative wet = " + static_cast<String>(humidityValue) + "%";
}

String formSolarMsg()
{
  if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    float solarValue = dataPacketExternal[2][3];
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      solarValue = normalize(solarValue) * 100;
      scaleName = "%";
    }
    else
    {
      scaleName = " Abs.";
    }

    return "Light = " + static_cast<String>(solarValue) + scaleName;
  }
  else
  {
    return "Light = <?>";
  }
}

String formUVMsg()
{
  if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    float uvValue = dataPacketExternal[2][4];
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      uvValue = uvValue / 20.5;
      scaleName = " Unit"; //В результате данных преобразований получаем индекс УФ излучения в диапазоне 0..10
    }
    else
    {
      scaleName = " Abs.";
    }

    return "UV = " + static_cast<String>(uvValue) + scaleName;
  }
  else
  {
    return "UV = <?>";
  }
}

String formRainMsg()
{
  if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    float rainValue = dataPacketExternal[1][3];
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      rainValue = normalize(rainValue) * 100;
      scaleName = "%";
    }
    else
    {
      scaleName = " Abs.";
    }

    return "Rain = " + static_cast<String>(rainValue) + scaleName;
  }
  else
  {
    return "Rain = <?>";
  }
}

String formMicrophoneMsg()
{
  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    float microphoneValue = dataPacketInternal[1][4];
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      microphoneValue = normalize(microphoneValue * 2) * 100;
      scaleName = "%";
    }
    else
    {
      scaleName = " Abs.";
    }

    return "Sound = " + static_cast<String>(microphoneValue) + scaleName;
  }
  else
  {
    return "Sound = <?>";
  }
}

String formMQ135Msg()
{
  //Углекислый газ
  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    float carbonDioxideValue = dataPacketInternal[1][3];

    return "CO2 = " + static_cast<String>(carbonDioxideValue) + " ppm";
  }
  else
  {
    return "CO2 = <?>";
  }
}

String formCommandMsg(COMMANDS_TYPE commandId)
{
  switch(commandId){
    case RESTART_ALL:
      return "  Restart module";
    case TURNOFF_RADIO:
      return "  Disable radio";
    case CHANGE_SEND_INTERVAL:
      return "  Change send interv";
    case STOP_SEND_DATA:
      return "  Stop sending";
    case RESUME_SEND_DATA:
      return "  Resume sending";
    case GET_DETECTOR_MAP:
      return "  Get detector map";
    case GET_TIME_INTERVAL:
      return "  Get send interv";
    case GET_LIFE_TIME:
      return "  Get lifetime";
    case HEARTBEAT:
      return "  Ping";
    default:
      return "  Команда <?>";  
  }
}

char* getCurrDateTime()
{
  char time[8];
  char date[10];
  static char dateTime[17];

  memset(dateTime, 0, 17);

  rtc.getTimeChar(time);
  rtc.getDateChar(date);

  strncpy(dateTime, date, 6);
  dateTime[6] = date[8]; //Берем 2 последние цифры года
  dateTime[7] = date[9];
  dateTime[8] = ' ';
  strcpy(dateTime + 9, time);

  return dateTime;
}

void sendDataToCard(String msg){
  sendMsgToCard(LOG_MSG_TYPE::METEODATA, msg);
}

void sendInfoToCard(String msg){
  sendMsgToCard(LOG_MSG_TYPE::INFO, msg);
}

void sendErrorToCard(String msg){
  sendMsgToCard(LOG_MSG_TYPE::ERROR, msg);
}

void sendMsgToCard(LOG_MSG_TYPE typeMsg, String msg)
{
  if(!sdCardInitialized){
    return;
  }
  activateSdCard(); //Переключаем SPI на работу с Sd картой

  File logFile = SD.open(LOG_FILENAME);

  if(!logFile){
    logFile = SD.open(LOG_FILENAME, FILE_WRITE);
    if(!logFile){
      Serial.println("Error creation log file!");
      return;
    }
  }

  String prefixMsg;

  switch(typeMsg){
    case LOG_MSG_TYPE::METEODATA:
      prefixMsg = "[Данные] ";
    break;
    case LOG_MSG_TYPE::INFO:
      prefixMsg = "[Инфо] ";
    break;
    case LOG_MSG_TYPE::ERROR:
      prefixMsg = "[Ошибка] ";
    break;
    default:
      prefixMsg = "[?] ";
    break;
  }

  logFile.println(prefixMsg + msg);
  logFile.close();
}