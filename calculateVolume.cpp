#include "calculateVolume.h"

calcValue::calcValue() {
  //calculate the maximal Value
  #ifdef lyingCylinder
    maxVolume = lyingCyl(r*2);
  #endif
}

double calcValue::lyingCyl(double height, double max){
  double a = 0 ;
  double b = 0 ;
  double c = 0 ;
  double d = 0 ;
  double e = 0 ;

  if(height > r*2){
    height = height-r*2;
    volume = max;
  }else
    volume = 0;

  a = (sq(r) * L);
  b = (acos((r - height) / r));
  c = (r - height);
  d = (sqrt (2 * r * height - sq(height)));
  e = (sq(r));
  volume += (a * (b - c * d / e)); //m³
  return volume;
}

void calcValue::processValue(double height){
    #ifdef lyingCylinder
      volume = lyingCyl(height,maxVolume);
    #endif
    level = (volume / maxVolume)*100;
    liter = volume *1000;
}

uint8_t calcValue::getLevel(){
  return level;
}

uint16_t calcValue::getLiter(){
  return liter;
}

double calcValue::getVolume(){
  return volume;
}