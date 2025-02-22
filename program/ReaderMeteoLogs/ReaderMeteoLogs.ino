#include <SPI.h>
#include <SD.h>

#define RADIO_CSN_PIN     9
#define SD_CARD_CSN_PIN   10

#define LOG_FILENAME	"TMD.log"

bool sdCardInitialized = false;

void setup()
{
  startSdCard();
  
}

void loop()
{
	
}

void activateSdCard(){
  digitalWrite(RADIO_CSN_PIN, HIGH);
  digitalWrite(SD_CARD_CSN_PIN, LOW);
}

void startSdCard()
{
  pinMode(SD_CARD_CSN_PIN, OUTPUT);
  activateSdCard();
  if(!SD.begin(SD_CARD_CSN_PIN)) {
    Serial.println(F("Initialization SD card was failed"));
    return;
  }

  sdCardInitialized = true;
}