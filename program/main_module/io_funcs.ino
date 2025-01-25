//Функции для обработки входа (нажатия кнопок) и выхода (работа с дисплеем, запись в SD карту)

void startDisplay()
{
  lcd.init(); //Инициируем работу с LCD дисплеем
  lcd.backlight(); //Включаем подсветку LCD дисплея
  lcd.setCursor(0, 0);
  lcd.print("--------------------");
  lcd.setCursor(0, 1); 
  lcd.print("Метеостанция");                
  lcd.setCursor(0, 2);
  lcd.print("Выполняется запуск.."); 
  lcd.setCursor(0, 3);
  lcd.print("--------------------");
}

void updateDisplay()
{
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

  addButtonHandler(KEY_1, topLeftButtonHandler);
  addButtonHandler(KEY_2, bottomLeftButtonHandler);
  addButtonHandler(KEY_3, centerButtonHandler);
  addButtonHandler(KEY_4, topRightButtonHandler);
  addButtonHandler(KEY_5, bottomRightButtonHandler);
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
      scaleName = "млм рт.ст.";
    }
    else
    {
      scaleName = "Па";
    }

    return "P = " + static_cast<String>(pressureValue) + scaleName;
  }
  else
  {
    return "P = <?>";
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
    return "Отн. влажн. = <?>";
  }

  return "Отн. влажн. = " + static_cast<String>(humidityValue) + "%";
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
      scaleName = " Абс.";
    }

    return "Освещ. = " + static_cast<String>(solarValue) + scaleName;
  }
  else
  {
    return "Освещ. = <?>";
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
      scaleName = " Инд."; //В результате данных преобразований получаем индекс УФ излучения в диапазоне 0..10
    }
    else
    {
      scaleName = " Абс.";
    }

    return "УФ = " + static_cast<String>(uvValue) + scaleName;
  }
  else
  {
    return "УФ = <?>";
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
      scaleName = " Абс.";
    }

    return "Дождь = " + static_cast<String>(rainValue) + scaleName;
  }
  else
  {
    return "Дождь = <?>";
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
      scaleName = " Абс.";
    }

    return "Звук = " + static_cast<String>(microphoneValue) + scaleName;
  }
  else
  {
    return "Звук = <?>";
  }
}

String formMQ135Msg()
{
  //Углекислый газ
  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    float carbonDioxideValue = dataPacketInternal[1][3];

    return "Углек.газ = " + static_cast<String>(carbonDioxideValue) + "PPM";
  }
  else
  {
    return "Углек.газ = <?>";
  }
}

String formCommandMsg(COMMANDS_TYPE commandId)
{
  switch(commandId){
    case RESTART_ALL:
      return "  Перезапустить все";
    case TURNOFF_RADIO:
      return "  Выключить радио";
    case CHANGE_SEND_INTERVAL:
      return "  Изм.интер.отправ.";
    case STOP_SEND_DATA:
      return "  Прекр. отправку";
    case RESUME_SEND_DATA:
      return "  Продолж. отправку";
    case GET_DETECTOR_MAP:
      return "  Получить карту";
    case GET_TIME_INTERVAL:
      return "  Получить интерв.";
    case GET_LIFE_TIME:
      return "  Получ. время жизни";
    case HEARTBEAT:
      return "  Пинг";
    default:
      return "  Команда <?>";  
  }
}

String getCurrDateTime()
{
  char time[8];
  char date[10];
  char dateTime[17];

  memset(dateTime, 0, 17);

  rtc.getTimeChar(time);
  rtc.getDateChar(date);

  strncpy(dateTime, date, 6);
  dateTime[6] = date[8]; //Берем 2 последние цифры года
  dateTime[7] = date[9];
  dateTime[8] = ' ';
  strcpy(dateTime + 9, time);

  return String{dateTime};
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