#ifndef CALCULATEVALUE_H
#define CALCULATEVALUE_H

#include "Arduino.h"

#define lyingCylinder //defines the shape of the tank
#define L  2.3        // lenth of the tank
#define r  0.75       // radius of the tank

class calcValue {
  private:
    double maxVolume  = 0;
    double volume     = 0;
    uint8_t level     = 0;    //in %
    uint16_t liter    = 0;

    double lyingCyl(double height);

  public:
    calcValue();
    void processValue(double height);
    uint8_t getLevel();
    uint16_t getLiter();
    double getVolume();
};

#endif
