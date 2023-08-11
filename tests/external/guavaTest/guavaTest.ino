void setup() 
{
  Serial.begin(9600);
}
 
void loop() 
{
float sensorVoltage; 
float sensorValue;
 
sensorValue = analogRead(A2);
sensorVoltage = sensorValue/1024*5;
Serial.print("3начение датчика = ");
Serial.print(sensorValue);
Serial.println(" ");
Serial.print("Напряжение сигнала = ");
Serial.print(sensorVoltage);
Serial.println(" V");
double UF=sensorValue/20.0 - 1;
if(UF<0) UF=0; 
Serial.print("UF: ");
Serial.println(UF);
delay(2000);
}
