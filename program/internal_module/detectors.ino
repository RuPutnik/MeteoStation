sensors_event_t humidityEvent, tempEvent;

//Функции получения данных
float getTemperatureValue(){
  ahtDetector.getEvent(&humidityEvent, &tempEvent);

  return tempEvent.temperature;
}

float getHumidityValue(){
  ahtDetector.getEvent(&humidityEvent, &tempEvent);
  return humidityEvent.relative_humidity;
}

int16_t getCurrMicrophoneValue(){
  return analogRead(MICROPHONE_PORT);
}

float getRealMicrophoneValue()
{
  int16_t currSignal;
  int16_t currSignalMax = 0;
  int16_t currSignalMin = 1024;

  for(int16_t i = 0; i < 500; i++){ //примерно 50 мс
    currSignal = getCurrMicrophoneValue();
    
    if(currSignal < 1024){
      if(currSignal > currSignalMax){
        currSignalMax = currSignal;
      }else if(currSignal < currSignalMin){
        currSignalMin = currSignal;
      }
    }
  }

  return currSignalMax - currSignalMin;
}

float getMQ135Value(){
  const float temperature = getTemperatureValue();
  const float humidity = getHumidityValue();
  //В качестве опорных параметров датчика (получены в результате калибровки) были приняты: RZero=315.0, RLoad=1.0 в MQ135.h
  return mq135_sensor.getCorrectedPPM(temperature, humidity);
}
