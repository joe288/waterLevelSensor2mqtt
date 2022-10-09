// Water level Sensor Modbus Data to MQTT Gateway
// Repo: https://github.com/joe288/waterLevelSensor2mqtt
// author: joe288@live.de
// QDY30A Sensor

// Libraries:
// - ModbusMaster by Doc Walker
// - ArduinoOTA
// - SoftwareSerial
// Hardware:
// - ESP32
// - RS485 to TTL converter: https://www.aliexpress.com/item/1005001621798947.html
#include <WiFi.h>
//#include <ESP8266WiFi.h>          // Wifi connection
#include <PubSubClient.h>         // MQTT support
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include "settings.h"
#include "QDY30AInterface.h"
#include "calculateVolume.h"

char newclientid[80];
char buildversion[12]="v1.0.0";
unsigned long lastModbus , lastStatus, lastWifiCheck, lastTick, uptime;

WiFiClient espClient;
PubSubClient mqtt(mqtt_server, 1883, 0, espClient);

QDY30AIF sensorInterface(MAX485_RE_NEG, MAX485_DE, MAX485_RX, MAX485_TX);
calcValue calculateVolume;

// MQTT reconnect logic
void reconnect() {
  //String mytopic;
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    byte mac[6];                     // the MAC address of your Wifi shield
    WiFi.macAddress(mac);
    sprintf(newclientid, "%s-%02x%02x%02x", clientID, mac[2], mac[1], mac[0]);
    Serial.print(F("Client ID: "));
    Serial.println(newclientid);
    // Attempt to connect
    char topic[80];
    sprintf(topic, "%s/%s", topicRoot, "connection");
    if (mqtt.connect(newclientid, mqtt_user, mqtt_password, topic, 1, true, "offline")) { //last will
      Serial.println(F("connected"));
      // ... and resubscribe
      mqtt.publish(topic, "online", true);
      sprintf(topic, "%s/write/#", topicRoot);
      mqtt.subscribe(topic);
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(mqtt.state());
      Serial.println(F(" try again in 5 seconds"));
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(SERIAL_RATE);
  Serial.println(F("\nwaterLevelSensor to MQTT Gateway"));
  
  // Init outputs, RS485 in receive mode
  pinMode(STATUS_LED, OUTPUT);

  // Connect to Wifi
  Serial.print(F("Connecting to Wifi"));
  WiFi.mode(WIFI_STA);
  lastWifiCheck=0;

#ifdef FIXEDIP
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
#endif

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    lastWifiCheck++;
    if (lastWifiCheck > 180) {
      // reboot the ESP if cannot connect to wifi
      ESP.restart();
    }
  }
  lastWifiCheck = 0;
  Serial.println("");
  Serial.println(F("Connected to wifi network"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("Signal [RSSI]: "));
  Serial.println(WiFi.RSSI());

  // Set up the Modbus line
  sensorInterface.initSensor();
  Serial.println("Modbus connection is set up");
  
  // Set up the MQTT server connection
  if (mqtt_server != "") {
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setBufferSize(1024);
    mqtt.setCallback(callback);
  }

  // Hostname defaults to esp32-[ChipID]
  byte mac[6];                     // the MAC address of your Wifi shield
  WiFi.macAddress(mac);
  char value[80];
  sprintf(value, "%s-%02x%02x%02x", clientID, mac[2], mac[1], mac[0]);
  ArduinoOTA.setHostname(value);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
 //   os_timer_disarm(&myTimer);
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
   // os_timer_arm(&myTimer, 1000, true);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Convert the incoming byte array to a string

  int i = 0;
  for (i = 0; i < length; i++) {        // each char to upper
    payload[i] = toupper(payload[i]);
  }
  payload[length] = '\0';               // Null terminator used to terminate the char array
  String message = (char*)payload;

  char expectedTopic[40];
}

void loop() {
  ArduinoOTA.handle();
  // Handle MQTT connection/reconnection
  if (mqtt_server != "") {
    if (!mqtt.connected()) {
      reconnect();
    }
    mqtt.loop();
  }

  if (millis() - lastWifiCheck >= WIFICHECK) {
    // reconnect to the wifi network if connection is lost
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Reconnecting to wifi...");
      WiFi.reconnect();
    }
    lastWifiCheck = millis();
  }
  // Uptime calculation
  if (millis() - lastTick >= 60 * mS_TO_S_FACTOR) {
    lastTick = millis();
    uptime++;
  }
  // Query the modbus device
  if (millis() - lastModbus >= UPDATE_MODBUS * mS_TO_S_FACTOR) {
    lastModbus = millis();
    float hight = sensorInterface.readLevel();
    String unit = sensorInterface.getLevelUnit();

    double meter = 0;
    if (unit == "cm"){
      meter = hight /100;
    }
    calculateVolume.processValue(meter);
    uint8_t level = calculateVolume.getLevel();
    uint16_t liter = calculateVolume.getLiter();

    // Serial.print(hight);
    // Serial.println(unit);
    // Serial.println(meter);
    char topic[80];
    char value[300];
    sprintf(value, "{\"value\": %f, \"unit\": %s, \"level\": %d, \"liter\": %d\"}", hight, unit, level, liter);
    sprintf(topic, "%s/%s", topicRoot, "waterLevel");
    mqtt.publish(topic, value);
  }
  if (millis() - lastStatus >= UPDATE_STATUS * mS_TO_S_FACTOR) {
    lastStatus = millis();
    // Send MQTT update
    if (mqtt_server != "") {
      char topic[80];
      char value[300];
      sprintf(value, "{\"rssi\": %d, \"uptime\": %d, \"ssid\": \"%s\", \"ip\": \"%d.%d.%d.%d\", \"clientid\":\"%s\", \"version\":\"%s\"}", WiFi.RSSI(), uptime, WiFi.SSID().c_str(), WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], newclientid, buildversion);
      sprintf(topic, "%s/%s", topicRoot, "status");
      mqtt.publish(topic, value);
      Serial.println(F("MQTT status sent"));
    }
  }
}
