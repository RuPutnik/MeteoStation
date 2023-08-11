#include <microDS3231.h>
#include <SPI.h>

MicroDS3231 rtc;                                   

void setup(){                   
 Serial.begin(9600);
 //rtc.setHMSDMY(21,47,30,6,9,2022);
}
void loop(){
  Serial.println(rtc.getDateString());
  // print the number of seconds since reset:
  Serial.println(rtc.getTimeString());
  delay(200);
}
