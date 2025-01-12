#ifndef DETECTORS
#define DETECTORS

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
  return normalize(analogRead(TEMT6000_PORT));
}

float getUVValue(){
float UV=analogRead(GUAVA_PORT)/20.0 - 1;
if(UV<1) UV=0;
return UV; 
}

float getRainValue(){
  return normalize(analogRead(RAIN_DETECT_PORT));
}

#endif