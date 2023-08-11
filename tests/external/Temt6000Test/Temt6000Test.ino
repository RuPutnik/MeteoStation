    void setup() 
{
  Serial.begin(9600);
}
 
void loop() 
{
float sensorVoltage; 
float sensorValue;
 
sensorValue = analogRead(A1);
sensorVoltage = sensorValue/1024*3.3;
Serial.print("Значение датчика = ");
Serial.print(sensorValue);
Serial.println(" ");
Serial.print("Напряжение сигнала = ");
Serial.print(sensorVoltage);
Serial.println(" V");
delay(2000);
}
