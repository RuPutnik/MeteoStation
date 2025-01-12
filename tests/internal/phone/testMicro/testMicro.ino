const int sampleWindow=50;
unsigned int sample;
#include <math.h>
void setup(){
  Serial.begin(9600);
}

void loop(){
  unsigned long startMillis=millis();
  unsigned int peakToPeak=0;

  unsigned int signalMax=0;
  unsigned int signalMin=1024;

  while(millis()-startMillis<sampleWindow){
    sample=analogRead(1);
    
    if(sample<1024){
      if(sample>signalMax){
        signalMax=sample;
      }
      else if(sample<signalMin){
        signalMin=sample;
      }
    }
  }

  peakToPeak=signalMax-signalMin;
  double volts=(peakToPeak*5)/1024;
  //Serial.println(volts);
  double dBells=20*log10(volts/0.775);
  Serial.print("ДБ: ");
  Serial.println(dBells);
}
