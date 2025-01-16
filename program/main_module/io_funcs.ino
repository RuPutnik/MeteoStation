//Функции для обработки входа (нажатия кнопок) и выхода (работа с дисплеем)

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
      //Для предотвращения многократного срабатывания одной и той же кнопки при хотя бы небольшом зажатии
      delay(KEY_POSTHANDLE_DELAY_MSEC);
    }
  }
}

void topRightButtonHandler()
{
  //Данные назад / команда назад


}

void bottomRightButtonHandler()
{
  //Данные вперед / команда вперед


}

void centerButtonHandler()
{
  //Смена модуля


}

void topLeftButtonHandler()
{
  //Войти-выйти в режим управления


}

void bottomLeftButtonHandler()
{
  //Формат отображения значений / Отправить команду


}

String formTemperatureMsg()
{

} 

String formPressureMsg()
{

}

//Влажность
String formHumidityMsg()
{
  //Анализировать currShowDataMode
}

String formSolarMsg()
{

}

String formUVMsg()
{

}

String formRainMsg()
{

}