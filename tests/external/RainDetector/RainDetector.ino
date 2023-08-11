int analogRainPin = A0;  // Аналоговый вход

void setup() {
  Serial.begin(9600);
}

void loop() {
  int sensorValue = analogRead(analogRainPin); // Считываем данные с аналогового порта
  Serial.print("Analog value: ");
  Serial.println(sensorValue); // Выводим аналоговое значение
  delay(1000);
}