# Water level Sensor Modbus Data to MQTT Gateway
This sketch runs on an ESP32 and reads data from a water level sensor over RS485 Modbus and publishes the data over MQTT. This code can be modified for any water level sensor, it has been tested on QDY30A. The sensor S_YW_01B also has the same protocol (different address). You find attached to this repo the documentation I used to get data out of the sensor.

## Wiring
 Connect the yellow cable for RS485A1 and the blue cable for RS485B1. RS485A1 connects to the A pin (green screw terminal) on the RS485toTTL board and RS485B1 connects to B. 

![Port](/img/wiring.PNG)

ESP32 Wroom (or other ESP) | TTL to RS485
----|----
GOIO5                      | DE
GPIO4                      | RE
GPIO14                     | RO
GPIO15                     | DI
3,3V                       | VCC
G(GND)                     | GND

## Libraries
The following libraries are used beside the "Standard" ESP32 libraries:
- [ModbusMaster by Doc Walker](https://github.com/4-20ma/ModbusMaster)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- ArduinoOTA
- [SoftwareSerial](https://github.com/plerup/espsoftwareserial)