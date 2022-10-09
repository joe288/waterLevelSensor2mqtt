#define SERIAL_RATE     115200    // Serial speed for status info
#define MAX485_DE       5         // DE pin on the TTL to RS485 converter
#define MAX485_RE_NEG   4         // RE pin on the TTL to RS485 converter
#define MAX485_RX       14        // RO pin on the TTL to RS485 converter
#define MAX485_TX       15        // DI pin on the TTL to RS485 converter
#define STATUS_LED      2         // Status LED on the Wemos D1 mini (D4)
#define UPDATE_MODBUS   60        // modbus device is read in seconds
#define UPDATE_STATUS   60        // status mqtt message is sent in seconds
#define WIFICHECK       500       // how often check lost wifi connection ms
#define mS_TO_S_FACTOR  1000      // Conversion factor for milliseconds to seconds

#define WDT_TIMEOUT 3         //3 seconds WDT

#define RGBLED_PIN D3         // Neopixel led D3
#define NUM_LEDS 1
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define BRIGHTNESS  64        // Default LED brightness.

// Update the below parameters for your project
// Also check NTP.h for some parameters as well
const char* ssid = "****";           // Wifi SSID
const char* password = "******";    // Wifi password
const char* mqtt_server = "192.168.*.*";     // MQTT server
const char* mqtt_user = "";             // MQTT userid
const char* mqtt_password = "";         // MQTT password
const char* clientID = "waterLevelSensor";                // MQTT client ID
const char* topicRoot = "waterLevelSensor";             // MQTT root topic for the device, keep / at the end


// Comment the entire second below for dynamic IP (including the define)
// #define FIXEDIP   1
IPAddress local_IP(192, 168, 1, 205);         // Set your Static IP address
IPAddress gateway(192, 168, 1, 254);          // Set your Gateway IP address
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 254);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
