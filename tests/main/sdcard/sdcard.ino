#include <SD.h>
#include <SPI.h>

#define CS 10

Sd2Card card;
SdVolume volume;
SdFile root;

boolean initCard()
{
  
  Serial.print("Connecting to SD card... ");

  // Initialize the SD card
  if(!card.init(SPI_HALF_SPEED, CS))
  {
    // An error occurred
    Serial.println("\nError: Could not connect to SD card!");
    Serial.println("- Is a card inserted?");
    Serial.println("- Is the card formatted as FAT16/FAT32?");
    Serial.println("- Is the CS pin correctly set?");
    Serial.println("- Is the wiring correct?");

    return false;
  }
  else
    Serial.println("Done!");

  return true;
}
void setup(){                       
 Serial.begin(9600);
 pinMode(9,OUTPUT);
 digitalWrite(9,HIGH); //ОБЯЗАТЕЛЬНО ВЫСТАВИТЬ, ЕСЛИ К ЭТОМУ ПИНУ ПОДКЛЮЧЕНО ДРУГОЕ SPI УСТРОЙСТВО
 
    if(!initCard()){
      Serial.println("Problem");
      return;
    }else{
      Serial.println("Everything ok");
    }


  Serial.println("\n----------------");
  Serial.println("---Basic Info---");
  Serial.println("----------------");

  Serial.print("An ");
  
  switch (card.type())
  {
    case SD_CARD_TYPE_SD1:
      Serial.print("SD1");
      break;
      
    case SD_CARD_TYPE_SD2:
      Serial.print("SD2");
      break;
      
    case SD_CARD_TYPE_SDHC:
      Serial.print("SDHC");
      break;
      
    default:
      Serial.print("Unknown");
  }

  Serial.println(" type card got connected");


  /*if(!volume.init(card))
  {
    Serial.println("Could not open / on the card!\nIs the card formatted as FAT16/32?");
    while(1);
  }
  else
  {*/
  delay(100);
  while(!volume.init(card)){
    delay(10);
   // initCard();
  }
  
    Serial.print("The inserted card is formatted as: FAT");
    Serial.println(volume.fatType(), DEC);
    Serial.print("And the card has a size of: ");

    // Calculate the storage capacity of the volume
    uint32_t cardSize = volume.blocksPerCluster() * volume.clusterCount();

    Serial.print(cardSize / 2048);
    Serial.println("MB!");
  

  Serial.println("\n----------------");
  Serial.println("--Stored files--");
  Serial.println("----------------");

  // Print a list of files that are present on the card
  root.openRoot(volume);
  root.ls(LS_R);

}
void loop(){
  
}
