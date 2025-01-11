#include <SPI.h>
#include <RF24.h>
#include <string.h>
#define CHANNEL_NUMBER 8
#define SIZE_TMD_DATA 8
#define CENTRAL_MODULE_ID      0
#define INTERNAL_MODULE_ID     1
#define EXTERNAL_MODULE_ID     2
#define PIPE_READ_ADDRESS          0xF0F0F0F0E1LL
#define PIPE_WRITE_ADDRESS         0xF0F0F0F0E2LL
#define SERIAL_SPEED               9600
#define RADIO_CHANNEL_NUMBER       8
#define DATA_SEGMENT_LENGTH        8
#define DATA_SEGMENT_LENGTH_B      DATA_SEGMENT_LENGTH*sizeof(float)  //32
#define COUNT_SEGMENTS_IN_PACKET   3
RF24 radio(9, 10);  // указать номера пинов, куда подключаются CE и CSN
//Здесь сырые данные должны переводиться в адекватные единицы
float tmdData[SIZE_TMD_DATA];
char dataParam[10];
void setup(){                        
    radio.begin();
    radio.setChannel(CHANNEL_NUMBER);
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_HIGH);
    radio.openReadingPipe(1,0x1234567890LL);
    radio.startListening();
    Serial.begin(9600);

}
void loop(){
  if (radio.available())
  {
    radio.read(&tmdData, SIZE_TMD_DATA*sizeof(float));   
    
    for(int i=0;i<SIZE_TMD_DATA;++i){
      sprintf(dataParam, "%d: %f", i, tmdData[i]);
      Serial.println(dataParam);
    }
    if((int)tmdData[7]==calcCheckSum(tmdData,SIZE_TMD_DATA)){
      Serial.println("Packet is valid");  
    }else{
      Serial.println("!!Packet is invalid!!"); 
    }      
  }else{
    Serial.println("No data");
  }
  delay(20);
}

int calcCheckSum(float* data, int sizeData){
  int sum=0;
  for(int i=0;i<sizeData-1;i++){
    sum+=(int)data[i];
  }
  return sum;   
}
