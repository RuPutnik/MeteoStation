#include <MQ135.h>

/*  MQ135 gas sensor
    Datasheet can be found here: https://www.olimex.com/Products/Components/Sensors/SNS-MQ135/resources/SNS-MQ135.pdf

    Application
    They are used in air quality control equipments for buildings/offices, are suitable for detecting of NH3, NOx, alcohol, Benzene, smoke, CO2, etc

    Original creator of this library: https://github.com/GeorgK/MQ135
*/

#define PIN_MQ135 A0

MQ135 mq135_sensor(PIN_MQ135);

float temperature = 22.5; // Assume current temperature. Recommended to measure with DHT22
float humidity = 27.0; // Assume current humidity. Recommended to measure with DHT22

void setup() {
  Serial.begin(9600);
}

void loop() {
  float rzero = mq135_sensor.getPPM();
  float resistance = mq135_sensor.getResistance();

  Serial.print("MQ135 RZero: ");
  Serial.print(rzero);
  Serial.print("\t Resistance: ");
  Serial.println(resistance);

  delay(300);
}
