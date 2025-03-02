#include <SPI.h>
#include <SD.h>

#define RADIO_CSN_PIN     9
#define SD_CARD_CSN_PIN   10

#define LOG_FILENAME	"TMD.log"

Sd2Card card;
SdVolume volume;
SdFile root;

void setup()
{
  Serial.begin(9600);
  pinMode(SD_CARD_CSN_PIN, OUTPUT);
  activateSdCard();

  startSdCard();
  printListFiles();
  //printMeteodata();
}

void loop()
{}

void activateSdCard(){
  digitalWrite(RADIO_CSN_PIN, HIGH);
  digitalWrite(SD_CARD_CSN_PIN, LOW);
}

bool initCard()
{ 
  Serial.println("Connecting to SD card... ");

  if(!card.init(SPI_HALF_SPEED, SD_CARD_CSN_PIN)){
    Serial.println("Error: Could not connect to SD card!");
    return false;
  }else{
    Serial.println("Done!");
  }

  return true;
}

void printListFiles()
{
  while(!volume.init(card)){
    delay(10);
    initCard();
  }

  Serial.println("----------------");
  Serial.println("--Stored files--");
  Serial.println("----------------");

  root.openRoot(volume);
  
  root.ls(LS_R | LS_DATE | LS_SIZE);
}

bool startSdCard()
{
  if(!SD.begin(SD_CARD_CSN_PIN)) {
    Serial.println(F("Initialization SD card was failed"));
    return false;
  }

  return true;
}

void printMeteodata()
{
  File myFile = SD.open(LOG_FILENAME);
  if (myFile) {
    Serial.println("Read from card:");
    // считываем все байты из файла и выводим их в COM-порт
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // закрываем файл
    myFile.close();
  } else {
    // выводим ошибку если не удалось открыть файл
    Serial.println(String("Error opening: ") + String(LOG_FILENAME));
  }
}