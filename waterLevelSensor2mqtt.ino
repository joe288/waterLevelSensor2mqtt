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
#include <esp_task_wdt.h>         // Task-Watchdog
#include "settings.h"
#include "QDY30AInterface.h"
#include "calculateVolume.h"

#define FIXEDIP

// Watchdog-Timeout in Sekunden
#define WDT_TIMEOUT_S 15

int wifiRetries = 0;
int mqttRetries = 0;
const int MAX_RETRIES = 10;
bool apStarted = false;

char newclientid[80];
char buildversion[12] = "v1.0.1";
unsigned long lastModbus, lastStatus, lastWifiCheck, lastTick, uptime;

WiFiClient espClient;
PubSubClient mqtt(mqtt_server, 1883, 0, espClient);

QDY30AIF sensorInterface(MAX485_RE_NEG, MAX485_DE, MAX485_RX, MAX485_TX);
calcValue calculateVolume;

unsigned long previousMillis = 0;
long LEDinterval = 500; 

// MQTT reconnect logic
void reconnect() {
  while (!mqtt.connected()) {
    // Watchdog füttern in langer Retry-Schleife
    esp_task_wdt_reset();

    Serial.print("Attempting MQTT connection...");
    byte mac[6];
    WiFi.macAddress(mac);
    sprintf(newclientid, "%s-%02x%02x%02x", clientID, mac[2], mac[1], mac[0]);
    Serial.print(F("Client ID: "));
    Serial.println(newclientid);

    char topic[80];
    sprintf(topic, "%s/%s", topicRoot, "connection");
    if (mqtt.connect(newclientid, mqtt_user, mqtt_password, topic, 1, true, "offline")) {
      Serial.println(F("connected"));
      mqtt.publish(topic, "online", true);
      sprintf(topic, "%s/write/#", topicRoot);
      mqtt.subscribe(topic);
    } else {
      if (mqttRetries >= MAX_RETRIES) {
        Serial.println("Maximale Versuche erreicht MQTT. Neustart");
        ESP.restart();
      }
      mqttRetries ++;
      Serial.print(F("failed, rc="));
      Serial.print(mqtt.state());
      Serial.println(F(" try again in 5 seconds"));
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(SERIAL_RATE);
  Serial.println(F("\nwaterLevelSensor to MQTT Gateway"));

  // ─── WATCHDOG INIT ─────────────────────────────────────────
  // init Watchdog mit Panic nach Timeout, Task NULL = current task (loop)
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);
  // ───────────────────────────────────────────────────────────

  // Init outputs, RS485 in receive mode
  pinMode(STATUS_LED, OUTPUT);

  // Connect to Wifi
  Serial.print(F("Connecting to Wifi"));
  WiFi.mode(WIFI_STA);

  lastWifiCheck = 0;

#ifdef FIXEDIP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
#endif

  WiFi.begin(ssid, password);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    esp_task_wdt_reset();   // Watchdog füttern
    Serial.print(F("."));
    lastWifiCheck++;
    if (millis() - wifiStart > 18000) {
      //ESP.restart();
      break;
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
  if ((mqtt_server != "") and (WiFi.status() == WL_CONNECTED)) {
    Serial.println("MQTT setup");
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setBufferSize(1024);
    mqtt.setCallback(callback);
  }

  // Hostname defaults to esp32-[ChipID]
  byte mac[6];
  WiFi.macAddress(mac);
  char value[80];
  sprintf(value, "%s-%02x%02x%02x", clientID, mac[2], mac[1], mac[0]);
  
  ArduinoOTA.setHostname(value);

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    esp_task_wdt_reset();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)     Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("end Setup");
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (unsigned int i = 0; i < length; i++) {
    payload[i] = toupper(payload[i]);
  }
  payload[length] = '\0';
  String message = (char*)payload;
  // Dein Callback-Code hier…
}

void loop() {
  // ─── WDT RESET ──────────────────────────────────────────────
  esp_task_wdt_reset();
  // ───────────────────────────────────────────────────────────
  unsigned long currentMillis = millis();

  if (WiFi.status() != WL_CONNECTED)
    LEDinterval = 200;
  else
    LEDinterval = 600;

  if (currentMillis - previousMillis >= LEDinterval) {
    previousMillis = currentMillis;
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
  }

  ArduinoOTA.handle();

  if ((mqtt_server != "")){//and (WiFi.status() == WL_CONNECTED)) {
    if (!mqtt.connected()) {
      reconnect();
    }
    mqtt.loop();
  }

  if (millis() - lastWifiCheck >= WIFICHECK) {
    if (WiFi.status() != WL_CONNECTED && !apStarted) {
      Serial.println("Reconnecting to wifi...");
      WiFi.reconnect();
      wifiRetries++;
      if (wifiRetries >= MAX_RETRIES) {
        Serial.println("Maximale Versuche erreicht. Starte Access Point...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("ESP32_Fallback","12345678");
        Serial.print("AP aktiv! IP-Adresse: ");
        Serial.println(WiFi.softAPIP());
        apStarted = true;
      }
    } else if (WiFi.status() == WL_CONNECTED) {
      wifiRetries = 0;
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
    double hight = sensorInterface.readLevel();
    String unit = sensorInterface.getLevelUnit();
    double meter = (unit == "cm") ? (hight / 100) : hight;
    calculateVolume.processValue(meter);
    uint8_t level = calculateVolume.getLevel();
    uint16_t liter = calculateVolume.getLiter();

    char topic[80], value[300];
    sprintf(value, "{\"value\": %.2f, \"unit\":\"%s\", \"level\": %d, \"liter\": %d}",
            hight, unit.c_str(), level, liter);
    sprintf(topic, "%s/%s", topicRoot, "waterLevel");
    mqtt.publish(topic, value);
  }

  if (millis() - lastStatus >= UPDATE_STATUS * mS_TO_S_FACTOR) {
    lastStatus = millis();
    if (mqtt_server != "") {
      char topic[80], value[300];
      sprintf(value,
              "{\"rssi\": %d, \"uptime\": %d, \"ssid\": \"%s\", \"ip\": \"%d.%d.%d.%d\", \"clientid\":\"%s\", \"version\":\"%s\"}",
              WiFi.RSSI(), uptime, WiFi.SSID().c_str(),
              WiFi.localIP()[0], WiFi.localIP()[1],
              WiFi.localIP()[2], WiFi.localIP()[3],
              newclientid, buildversion);
      sprintf(topic, "%s/%s", topicRoot, "status");
      mqtt.publish(topic, value);
      Serial.println(F("MQTT status sent"));
    }
  }
}

