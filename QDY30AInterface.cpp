#include "QDY30AInterface.h"

QDY30AIF::QDY30AIF(int _PinMAX485_RE_NEG, int _PinMAX485_DE, int _PinMAX485_RX, int _PinMAX485_TX) {
  PinMAX485_RE_NEG = _PinMAX485_RE_NEG;
  PinMAX485_DE = _PinMAX485_DE;
  PinMAX485_RX = _PinMAX485_RX;
  PinMAX485_TX = _PinMAX485_TX;
  
  // Init outputs, RS485 in receive mode
   pinMode(PinMAX485_RE_NEG, OUTPUT);
   pinMode(PinMAX485_DE, OUTPUT);
   digitalWrite(PinMAX485_RE_NEG, 0);
   digitalWrite(PinMAX485_DE, 0);
}

void QDY30AIF::initSensor() {
  serial = new SoftwareSerial (PinMAX485_RX, PinMAX485_TX, false); //RX, TX
  serial->begin(MODBUS_RATE);
  sensorInterface.begin(SLAVE_ID , *serial);

  static QDY30AIF* obj = this;                               //pointer to the object
  // Callbacks allow us to configure the RS485 transceiver correctly
  sensorInterface.preTransmission ([]() {                   //Set function pointer via anonymous Lambda function
    obj->preTransmission();
  });

  sensorInterface.postTransmission([]() {                   //Set function pointer via anonymous Lambda function
    obj->postTransmission();
  });

  modbusdata.decimalPlaces = getDecimalPlaces();
  modbusdata.unitPressure  = getUnitPressure();
}

void QDY30AIF::writeRegister(uint16_t reg, uint16_t message) {
  sensorInterface.writeSingleRegister(reg, message);
}

uint16_t QDY30AIF::readRegister(uint16_t reg){
  sensorInterface.readHoldingRegisters(reg, 1);
  return sensorInterface.getResponseBuffer(0);
}

void QDY30AIF::preTransmission() {
  digitalWrite(PinMAX485_RE_NEG, 1);
  digitalWrite(PinMAX485_DE, 1);
}

void QDY30AIF::postTransmission() {
  digitalWrite(PinMAX485_RE_NEG, 0);
  digitalWrite(PinMAX485_DE, 0);
}

uint16_t QDY30AIF::getUnitPressure(void){
  uint8_t unit = readRegister(2);
  switch (unit)
  {
  case 1:
    waterLevel.unit ="cm";
    break;
  case 2:
    waterLevel.unit ="mm";
    break;
  case 3:
    waterLevel.unit ="Mpa";
    break;
  case 4:
    waterLevel.unit ="Pa";
    break;
  case 5:
    waterLevel.unit ="kPa";
    break;
  case 6:
    waterLevel.unit ="MA";
    break;
  default:
    break;
  }
  return unit;
}

uint16_t QDY30AIF::getDecimalPlaces(void){
  return readRegister(3);
}

double QDY30AIF::readLevel(void){
  uint16_t data = readRegister(4);
  switch (modbusdata.decimalPlaces)
  {
  case 1:
    waterLevel.value = data*0.1;
    break;
  case 2:
    waterLevel.value = data*0.01;
    break;
  case 3:
    waterLevel.value = data*0.001;
    break;
  default:
    waterLevel.value = data;
    break;
  }
return waterLevel.value;
// return data;
}

String QDY30AIF::getLevelUnit(void){
return waterLevel.unit;
}

uint8_t QDY30AIF::ReadInputRegisters(char* json) {
  uint8_t result;

  esp_task_wdt_reset();
  result = sensorInterface.readHoldingRegisters(0, 7);

  if (result == sensorInterface.ku8MBSuccess)   {
      // Status 
      modbusdata.slaveAddress     = sensorInterface.getResponseBuffer(0);
      modbusdata.Baudrate         = sensorInterface.getResponseBuffer(1);
      modbusdata.unitPressure     = sensorInterface.getResponseBuffer(2);
      modbusdata.decimalPlaces    = sensorInterface.getResponseBuffer(3);
      modbusdata.outputValue      = sensorInterface.getResponseBuffer(4);
      modbusdata.transmitterZero  = sensorInterface.getResponseBuffer(5);
      modbusdata.transmitterPoint = sensorInterface.getResponseBuffer(6);
     
      // Generate the json message
      sprintf(json, "{");
      sprintf(json, "%s \"address\":%d,", json, modbusdata.slaveAddress);
      sprintf(json, "%s \"baudrate\":%d,", json, modbusdata.Baudrate);
      sprintf(json, "%s \"unitPressure\":%d,", json, modbusdata.unitPressure);
      sprintf(json, "%s \"decimalPlaces\":%d,", json, modbusdata.decimalPlaces);
      sprintf(json, "%s \"outputValue\":%d,", json, modbusdata.outputValue);
      sprintf(json, "%s \"transmitterZero\":%d,", json, modbusdata.transmitterZero);
      sprintf(json, "%s \"transmitterPoint\":%d}", json, modbusdata.transmitterPoint);
      return result;
  } else {
    return result;
  }
}
String QDY30AIF::sendModbusError(uint8_t result) {
  String message = "";
  if (result == sensorInterface.ku8MBIllegalFunction) {
    message = "Illegal function";
  }
  if (result == sensorInterface.ku8MBIllegalDataAddress) {
    message = "Illegal data address";
  }
  if (result == sensorInterface.ku8MBIllegalDataValue) {
    message = "Illegal data value";
  }
  if (result == sensorInterface.ku8MBSlaveDeviceFailure) {
    message = "Slave device failure";
  }
  if (result == sensorInterface.ku8MBInvalidSlaveID) {
    message = "Invalid slave ID";
  }
  if (result == sensorInterface.ku8MBInvalidFunction) {
    message = "Invalid function";
  }
  if (result == sensorInterface.ku8MBResponseTimedOut) {
    message = "Response timed out";
  }
  if (result == sensorInterface.ku8MBInvalidCRC) {
    message = "Invalid CRC";
  }
  if (message == "") {
    message = result;
  }
  return message;
}
