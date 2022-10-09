#ifndef QDY30AINTERFACE_H
#define QDY30AINTERFACE_H

#include "Arduino.h"
#include <ModbusMaster.h>         // Modbus master library
#include <SoftwareSerial.h>       // Leave the main serial line (USB) for debugging and flashing
#include <esp_task_wdt.h>


class QDY30AIF {
  #define SLAVE_ID        1         // Default slave ID of Growatt
  #define MODBUS_RATE     9600      // Modbus speed, do not change
  
  private:
    ModbusMaster sensorInterface;
    SoftwareSerial *serial;
    void preTransmission();
    void postTransmission();
    int PinMAX485_RE_NEG;
    int PinMAX485_DE;
    int PinMAX485_RX;
    int PinMAX485_TX;
    int setcounter = 0;   
    uint16_t getUnitPressure();
    uint16_t getDecimalPlaces(); 
    struct measureData
    {
      double value;
      String unit;
    };
    struct measureData waterLevel;
    struct modbus_input_registers
    {
      int slaveAddress, Baudrate, unitPressure, decimalPlaces;
      int outputValue, transmitterZero, transmitterPoint;
    };
    struct modbus_input_registers modbusdata;
  public:
    QDY30AIF(int _PinMAX485_RE_NEG, int _PinMAX485_DE, int _PinMAX485_RX, int _PinMAX485_TX);
    void initSensor();
    void writeRegister(uint16_t reg, uint16_t message);
    uint16_t readRegister(uint16_t reg);
    uint8_t ReadInputRegisters(char* json);
    String sendModbusError(uint8_t result);
    double readLevel();
    String getLevelUnit();

    // Error codes
    static const uint8_t Success    = 0x00;
    static const uint8_t Continue   = 0xFF;
    
};

#endif
