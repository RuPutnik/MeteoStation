sensors_event_t humidityEvent, tempEvent;

//Функции получения данных
float getTemperatureValue(){
  float tempAHT10, tempBMP280;
  ahtDetector.getEvent(&humidityEvent, &tempEvent);

  tempAHT10=tempEvent.temperature;
  tempBMP280=bmpDetector.readTemperature();
  return (tempAHT10+tempBMP280)/2;
}

float getHumidityValue(){
  ahtDetector.getEvent(&humidityEvent, &tempEvent);
  return humidityEvent.relative_humidity;
}

float getPressureValue(){
  return bmpDetector.readPressure();
}

float getSolarValue(){
  return analogRead(TEMT6000_PORT);
}

float getUVValue(){
  return analogRead(GUAVA_PORT); 
}

float getRainValue(){
  return analogRead(RAIN_DETECT_PORT);
}
