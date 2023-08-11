#include <Wire.h>                     //  Подключаем библиотеку для работы с шиной I2C
#include <LiquidCrystal_I2C.h>        //  Подключаем библиотеку для работы с LCD дисплеем по шине I2C

LiquidCrystal_I2C lcd(0x3F,20,4);  //Объявляем  объект библиотеки, указывая параметры дисплея (адрес I2C = 0x27,
//количество столбцов = 20, количество строк = 4)

void setup(){                       
    lcd.init();                      
    lcd.backlight();                 
    lcd.setCursor(0, 0);              
    lcd.print("LCD");                
    lcd.setCursor(0, 1);              
    lcd.print("www.iarduino.ru");   
    lcd.setCursor(0, 2);            
    lcd.print("4x20");             
    lcd.setCursor(0, 3);              
    lcd.print("TEST");                

 Serial.begin(9600);
}
void loop(){  
}
