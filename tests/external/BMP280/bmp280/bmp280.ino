#include <Wire.h> // Библиотека для работы с шиной I2C
#include <SPI.h>  // Библиотека для работы с шиной SPI
#include <Adafruit_BMP280.h> // Библиотека для работы с датчиком BMP280

// Для подключения по шине I2C
Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(9600); // Для вывода отладочной информации в терминал
    if(!bmp.begin()) { // Если датчик BMP280 не найден
    Serial.println("BMP280 SENSOR ERROR"); // Выводим сообщение об ошибке
    while(1); // Переходим в бесконечный цикл
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, // Режим работы
                Adafruit_BMP280::SAMPLING_X2, // Точность изм. температуры
                Adafruit_BMP280::SAMPLING_X16, // Точность изм. давления
                Adafruit_BMP280::FILTER_X16, // Уровень фильтрации
                Adafruit_BMP280::STANDBY_MS_500); // Период просыпания, мСек
}



void loop() {
  // Выводим значение температуры
  Serial.print(F("Temperature = "));
  Serial.print(bmp.readTemperature()); // Функция измерения температуры
  Serial.println(" *C");

  // Выводим значение атмосферного давления
  Serial.print(F("Pressure = "));
  Serial.print(bmp.readPressure());  // Функция измерения атм. давления
  Serial.println(" Pa");

  // Выводим значение высоты
  Serial.print(F("Approx altitude = "));
  Serial.print(bmp.readAltitude(1013.25)); // Функция измерения высоты
  Serial.println(" m");
delay(500);
}