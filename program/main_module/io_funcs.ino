//Функции для обработки входа (нажатия кнопок) и выхода (работа с дисплеем, запись в SD карту)

struct KeyBoundle
{
  KEY_PORT port;
  void (*handlerPtr)();
};

KeyBoundle keyBundles[COUNT_KEYS];

void startDisplay()
{
  lcd.init();
  lcd.backlight(); //Включаем подсветку LCD дисплея
  lcd.setCursor(0, 0);
  lcd.print(F("--------------------"));
  lcd.setCursor(4, 1);
  lcd.print(F("MeteoStation"));
  lcd.setCursor(3, 2);
  lcd.print(F("Starting up..."));
  lcd.setCursor(0, 3);
  lcd.print(F("--------------------"));
}

void resetDisplay()
{
  lcd.clear();
}

void updateDisplay()
{
  resetDisplay();
  updateDisplayHeader();
  updateDisplayContent();
}

void updateDisplayHeader()
{
   lcd.setCursor(0, 0);
   if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID){
     lcd.print(F("M2"));
   }
   else if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID){
     lcd.print(F("M1"));
   }
   
   lcd.setCursor(3, 0);
   lcd.print(getCurrDateTime());
}

void updateDisplayContent()
{
  if(currWorkMode == WORK_MODE::SHOW_METEO_DATA)
  {
    updateDisplayContentMeteodata();
  }
  else
  {
    updateDisplayContentCommands();
  }
}

void updateDisplayContentMeteodata()
{
  if(!isCompleteDataPacket(currDisplayedModuleId)){
    lcd.setCursor(6, 2);
    lcd.print(F("No data"));
    return;
  }

  lcd.setCursor(0, 2);
  lcd.print(formMeteoDataMsg(currMeteoParam));

  if(currMeteoParam > 0){
    lcd.setCursor(0, 1);
    lcd.print(formMeteoDataMsg(currMeteoParam - 1));
  }

  if(currMeteoParam < getMaxMeteoParamIndex()){
    lcd.setCursor(0, 3);
    lcd.print(formMeteoDataMsg(currMeteoParam + 1));
  }
}

void updateDisplayContentCommands()
{
  lcd.setCursor(0, 2);
  lcd.print(String{F("> ")} + formCommandMsg(currCommand));

  if(currCommand > COMMANDS_TYPE::RESTART_ALL){
    lcd.setCursor(2, 1);
    lcd.print(formCommandMsg(static_cast<COMMANDS_TYPE>(currCommand - 1)));
  }

  if(currCommand < COMMANDS_TYPE::HEARTBEAT){
    lcd.setCursor(2, 3);
    lcd.print(formCommandMsg(static_cast<COMMANDS_TYPE>(currCommand + 1)));
  }
}

void printDisplayFailDeliveryCommand()
{
  resetDisplay();
  updateDisplayHeader();

  lcd.setCursor(3, 2);
  lcd.print(F("Command failed"));
  delay(PRINT_SERVICE_MSG_MSEC);
}

void printDisplayModuleServiceMsg(SERVICE_MSG_TYPE typeServicePacket, float valueParam)
{
  resetDisplay();
  updateDisplayHeader();

  String printBuffer;

  lcd.setCursor(0, 2);
  switch(typeServicePacket)
  {
    case SERVICE_MSG_TYPE::START_MODULE_SUCCESS:
      printBuffer = F("Module ");
      printBuffer = printBuffer + String{static_cast<int>(valueParam)};
      printBuffer = printBuffer + F(" started");
      break;
    case SERVICE_MSG_TYPE::SUCCESS_GET_COMMAND:
      lcd.setCursor(2, 2);
      printBuffer = F("Command executed");
      break;
    case SERVICE_MSG_TYPE::ERROR_START_DETECTOR:
      printBuffer = F("Error start ");
      printBuffer = printBuffer + String{static_cast<int>(valueParam)};
      printBuffer = printBuffer + F(" sensor");
      break;
    case SERVICE_MSG_TYPE::REPORT_TIME_INTERVAL:
      printBuffer = F("Send interval: ");
      printBuffer = printBuffer + String{static_cast<long int>(valueParam)};
      break;
    case SERVICE_MSG_TYPE::REPORT_LIFE_TIME:
      printBuffer = F("Lifetime: ");
      printBuffer = printBuffer + String{static_cast<unsigned long int>(valueParam)};
      break;
    case SERVICE_MSG_TYPE::GET_ERROR_COMMAND:
      printBuffer = F("Invalid command: ");
      printBuffer = printBuffer + String{static_cast<int>(valueParam)};
      break;
    default:
      printBuffer = F("Unknown msg type");
      break;
  }
  lcd.print(printBuffer);
  delay(PRINT_SERVICE_MSG_MSEC);
}

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
    else
    {
      updateDisplayHeader();
    }
  }
}

void topRightButtonHandler()
{
  //Данные назад / команда назад
  Serial.println(F("TopRight press"));
  if(currWorkMode == WORK_MODE::SHOW_METEO_DATA)
  {
    if(currMeteoParam > 0){
      currMeteoParam--;
    }
  }
  else
  {
    if(currCommand > COMMANDS_TYPE::RESTART_ALL){
      currCommand = static_cast<COMMANDS_TYPE>(static_cast<uint8_t>(currCommand) - 1);
    }
  }
}

void bottomRightButtonHandler()
{
  //Данные вперед / команда вперед
  Serial.println(F("BottomRight press"));
  if(currWorkMode == WORK_MODE::SHOW_METEO_DATA)
  {
    if(currMeteoParam < getMaxMeteoParamIndex()){
      currMeteoParam++;
    }
  }
  else
  {
    if(currCommand < COMMANDS_TYPE::HEARTBEAT){
      currCommand = static_cast<COMMANDS_TYPE>(static_cast<uint8_t>(currCommand) + 1);
    }
  }
}

void centerButtonHandler()
{
  //Смена модуля
  Serial.println(F("Center press"));
  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID){
    currDisplayedModuleId = MODULE_ID::EXTERNAL_MODULE_ID;
  }else{
    currDisplayedModuleId = MODULE_ID::INTERNAL_MODULE_ID;
  }

  currMeteoParam = 1;
  currCommand = COMMANDS_TYPE::STOP_START_SEND;
}

void topLeftButtonHandler()
{
  //Войти-выйти в режим управления
  Serial.println(F("TopLeft press"));
  if(currWorkMode == WORK_MODE::SHOW_METEO_DATA){
    currWorkMode = WORK_MODE::SHOW_COMMANDS;
  }else{
    currWorkMode = WORK_MODE::SHOW_METEO_DATA;
  }

  currMeteoParam = 1;
  currCommand = COMMANDS_TYPE::STOP_START_SEND;
}

void bottomLeftButtonHandler()
{
  //Формат отображения значений / Отправить команду
  Serial.println(F("BottomLeft press"));
  if(currWorkMode == WORK_MODE::SHOW_METEO_DATA)
  {
    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      currShowDataMode = SHOW_DATA_MODE::ALTERNATIVE;
    }
    else
    {
      currShowDataMode = SHOW_DATA_MODE::CLASSIC;
    }
  }
  else
  {
    fillActionPacket(currCommand, &actionPacket);
    sendActionPacket(&actionPacket);
    memset(&actionPacket, 0, sizeof(ActionServicePacket));
    resetServiceBuffer(currDisplayedModuleId);
  }
}

String formTemperatureMsg()
{
  float temperatureValue;
  String scaleName;

  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    temperatureValue = internalDataPacket.val1;
  }
  else if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    temperatureValue = externalDataPacket.val1;
  }
  else
  {
    return F("T = <?>");
  }

  if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
  {
    scaleName = F("C");
  }
  else
  {
    temperatureValue = temperatureValue * 1.8 + 32; //Перевод из цельсия в фаренгейты
    scaleName = F("F");
  }

  const String resultStr = F("T = ");

  return resultStr + static_cast<String>(temperatureValue) + scaleName;
} 

String formPressureMsg()
{
  if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    float pressureValue = externalDataPacket.val4;
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      pressureValue = pressureValue / 133.3;
      scaleName = F(" mmHg");
    }
    else
    {
      scaleName = F(" Pa");
    }

    const String str = F("Press = ");

    return str + String(pressureValue) + scaleName;
  }
  else
  {
    return F("Press = <?>");
  }
}

//Влажность
String formHumidityMsg()
{
  float humidityValue;

  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    humidityValue = internalDataPacket.val2;
  }
  else if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    humidityValue = externalDataPacket.val2;
  }
  else
  {
    return F("Relat wet = <?>");
  }

  String str = F("Relat wet = ");

  return str + String(humidityValue) + "%";
}

String formSolarMsg()
{
  if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    float solarValue = externalDataPacket.val5;
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      solarValue = normalize(solarValue) * 100;
      scaleName = "%";
    }
    else
    {
      scaleName = F(" Abs.");
    }

    const String resultStr = F("Light = ");

    return resultStr + String(solarValue) + scaleName;
  }
  else
  {
    return F("Light = <?>");
  }
}

String formUVMsg()
{
  if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    float uvValue = externalDataPacket.val6;
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      uvValue = uvValue / 20.5;
      scaleName = F(" Unit"); //В результате данных преобразований получаем индекс УФ излучения в диапазоне 0..10
    }
    else
    {
      scaleName = F(" Abs.");
    }

    const String resultStr = F("UV = ");

    return resultStr + String(uvValue) + scaleName;
  }
  else
  {
    return F("UV = <?>");
  }
}

String formRainMsg()
{
  if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
  {
    float rainValue = externalDataPacket.val3;
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      rainValue = 100 - (normalize(rainValue) * 100);
      scaleName = "%";
    }
    else
    {
      scaleName = F(" Abs.");
    }

    const String resultStr = F("Rain = ");

    return resultStr + static_cast<String>(rainValue) + scaleName;
  }
  else
  {
    return F("Rain = <?>");
  }
}

String formMicrophoneMsg()
{
  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    float microphoneValue = internalDataPacket.val4;
    String scaleName;

    if(currShowDataMode == SHOW_DATA_MODE::CLASSIC)
    {
      microphoneValue = normalize(microphoneValue * 2) * 100;
      scaleName = "%";
    }
    else
    {
      scaleName = F(" Abs.");
    }

    const String resultStr = F("Sound = ");

    return resultStr + static_cast<String>(microphoneValue) + scaleName;
  }
  else
  {
    return F("Sound = <?>");
  }
}

String formMQ135Msg()
{
  //Углекислый газ
  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
  {
    float carbonDioxideValue = internalDataPacket.val3;

    const String resultStr = F("CO2 = ");

    return resultStr + static_cast<String>(carbonDioxideValue) + F(" ppm");
  }
  else
  {
    return F("CO2 = <?>");
  }
}

String formCommandMsg(COMMANDS_TYPE commandId)
{
  switch(commandId){
    case RESTART_ALL:
      return F("Restart module");
    case STOP_START_SEND:
      return F("Stop/start send");
    case CHANGE_SEND_INTERVAL:
      return F("Change send interv");
    case GET_TIME_INTERVAL:
      return F("Get send interv ms");
    case GET_LIFE_TIME:
      return F("Get lifetime ms");
    case HEARTBEAT:
      return F("Ping");
    default:
      return F("Command <?>");  
  }
}

String formMeteoDataMsg(uint8_t indexParam)
{
  switch(indexParam){
    case 0:
      return formTemperatureMsg();
    case 1:
      return formHumidityMsg();
    case 2:      
      if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
      {
        return formPressureMsg();
      }
      else if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
      {
        return formMicrophoneMsg();
      }
      else
      {
        return "";
      }
    case 3:
      if(currDisplayedModuleId == MODULE_ID::EXTERNAL_MODULE_ID)
      {
        return formRainMsg();
      }
      else if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID)
      {
        return formMQ135Msg();
      }
      else
      {
        return "";
      }
    case 4:
      return formUVMsg();
    case 5:
      return formSolarMsg();
    default:
      return "";
  }
}

String getCurrDateTime()
{
  char time[9], date[11];
  static char currDateTime[18];

  rtc.getTimeChar(time);
  rtc.getDateChar(date);

  strncpy(currDateTime, date, 6);
  currDateTime[6] = date[8]; //Берем 2 последние цифры года
  currDateTime[7] = date[9];
  currDateTime[8] = ' ';
  strcpy(currDateTime + 9, time);

  return String{currDateTime};
}