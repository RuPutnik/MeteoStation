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
  lcd.print("--------------------");
  lcd.setCursor(4, 1);
  lcd.print("MeteoStation");
  lcd.setCursor(3, 2);
  lcd.print("Starting up...");
  lcd.setCursor(0, 3);
  lcd.print("--------------------");
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
     lcd.print("M2");
   }
   else if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID){
     lcd.print("M1");
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
    lcd.print("No data");
    return;
  }

  //TODO
}

void updateDisplayContentCommands()
{
  lcd.setCursor(0, 2);
  lcd.print(String{"> "} + formCommandMsg(currCommand));

  if(currCommand > COMMANDS_TYPE::RESTART_ALL){
    lcd.setCursor(2, 1);
    lcd.print(formCommandMsg(static_cast<COMMANDS_TYPE>(currCommand - 1)));
  }

  if(currCommand < COMMANDS_TYPE::HEARTBEAT){
    lcd.setCursor(2, 3);
    lcd.print(formCommandMsg(static_cast<COMMANDS_TYPE>(currCommand + 1)));
  }
}

void printDisplayExecuteCommandStatus(bool executed)
{
  lcd.clear();
  updateDisplayHeader();

  lcd.setCursor(2, 6);
  lcd.print("Command");

  if(executed)
  {
    lcd.setCursor(3, 6);
    lcd.print("executed");
  }
  else
  {
    lcd.setCursor(3, 7);
    lcd.print("failed");
  }
}

void printDisplayModuleParam(SERVICE_MSG_TYPE typeServicePacket, float valueParam)
{
  // switch(typeServicePacket)
  // {
  //   case SERVICE_MSG_TYPE::REPORT_DETECTOR_MAP:

  //   case SERVICE_MSG_TYPE::REPORT_TIME_INTERVAL:

  //   case SERVICE_MSG_TYPE::REPORT_LIFE_TIME:

  //   default:

      
  // }
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
  Serial.println("TopRight press");
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
  Serial.println("BottomRight press");
  if(currWorkMode == WORK_MODE::SHOW_METEO_DATA)
  {
    const uint8_t maxMeteoParamNumber = ((currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID) ? COUNT_METEO_PARAM_INTERNAL : COUNT_METEO_PARAM_EXTERNAL);
    if(currMeteoParam < maxMeteoParamNumber){
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
  Serial.println("Center press");
  if(currDisplayedModuleId == MODULE_ID::INTERNAL_MODULE_ID){
    currDisplayedModuleId = MODULE_ID::EXTERNAL_MODULE_ID;
  }else{
    currDisplayedModuleId = MODULE_ID::INTERNAL_MODULE_ID;
  }

  currMeteoParam = 1;
  currCommand = COMMANDS_TYPE::TURNOFF_RADIO;
}

void topLeftButtonHandler()
{
  //Войти-выйти в режим управления
  Serial.println("TopLeft press");
  if(currWorkMode == WORK_MODE::SHOW_METEO_DATA){
    currWorkMode = WORK_MODE::SHOW_COMMANDS;
  }else{
    currWorkMode = WORK_MODE::SHOW_METEO_DATA;
  }

  currMeteoParam = 1;
  currCommand = COMMANDS_TYPE::TURNOFF_RADIO;
}

void bottomLeftButtonHandler()
{
  //Формат отображения значений / Отправить команду
  Serial.println("BottomLeft press");
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
    fillActionPacket(currCommand, actionPacket);
    //sendActionPacket(actionPacket);
    memset(actionPacket, 0, DATA_SEGMENT_LENGTH_B);
  }
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
      return "Restart module";
    case TURNOFF_RADIO:
      return "Disable radio";
    case CHANGE_SEND_INTERVAL:
      return "Change send interv";
    case STOP_SEND_DATA:
      return "Stop sending";
    case RESUME_SEND_DATA:
      return "Resume sending";
    case GET_DETECTOR_MAP:
      return "Get detector map";
    case GET_TIME_INTERVAL:
      return "Get send interv";
    case GET_LIFE_TIME:
      return "Get lifetime";
    case HEARTBEAT:
      return "Ping";
    default:
      return "Command <?>";  
  }
}

String formMeteoDataMsg(int indexData)
{
  switch(indexData){
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