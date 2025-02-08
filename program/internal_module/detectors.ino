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

float getMicrophoneValue(){
  return analogRead(MICROPHONE_PORT);
}

float getMQ135Value(){
  const float temperature = getTemperatureValue();
  const float humidity = getHumidityValue();

  return mq135_sensor.getCorrectedPPM(temperature, humidity);
}
