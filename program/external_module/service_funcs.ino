#ifndef SERVICE_FUNCS
#define SERVICE_FUNCS

//Служебные функции
void resetDetectorMap(bool* detectorMap){
  for(int i=0;i<COUNT_DETECTOR;i++){
    detectorMap[i]=true;
  }
}

float normalize(int value){
  return value/1024.0;  
}

int calcCheckSum(float* data, int sizeData){
  int sum=0;
  for(int i=0;i<sizeData-1;i++){
    sum+=(int)data[i];
  }
  return sum;   
}

#endif