#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "secrets.h"

#define DHTPIN 5
#define DHTTYPE DHT11
#define DEVICE_ID "iot-sensor-externo-lhc-001"
#define MQTT_TOPIC "sensores/iot-sensor-externo-lhc-001/ambiente"
#define MQTT_RESPONSE_TOPIC "sensores/iot-sensor-externo-lhc-001/resposta"
#define MQTT_COMMAND_TOPIC "comandos/iot-sensor-externo-lhc-001/cmd"
#define MQTT_STATUS_TOPIC "sensores/iot-sensor-externo-lhc-001/status"
#define STATUS_LED 2
#define PUBLISH_INTERVAL_MS 30000UL

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
unsigned long lastPublish = 0;

void setLed(bool on) { digitalWrite(STATUS_LED, on ? LOW : HIGH); }

void blinkLed(int count, int onTime, int offTime) {
  for (int index = 0; index < count; index++) {
    setLed(true); delay(onTime); setLed(false); delay(offTime);
  }
}

void publishReading(bool responseOnly);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, MQTT_COMMAND_TOPIC) != 0) return;
  String command;
  for (unsigned int index = 0; index < length; index++) command += static_cast<char>(payload[index]);
  if (command == "read") publishReading(true);
  if (command == "restart") {
    mqttClient.publish(MQTT_RESPONSE_TOPIC, "{\"status\":\"restarting\"}");
    delay(300); ESP.restart();
  }
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA); WiFi.setSleep(false); WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("Connecting WiFi"));
  while (WiFi.status() != WL_CONNECTED) {
    blinkLed(2, 100, 100); delay(900); Serial.print('.');
  }
  Serial.print(F("\nWiFi IP: ")); Serial.println(WiFi.localIP());
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    blinkLed(3, 70, 70);
    if (mqttClient.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      mqttClient.subscribe(MQTT_COMMAND_TOPIC, 1);
      mqttClient.publish(MQTT_STATUS_TOPIC, "{\"status\":\"online\"}");
      Serial.println(F("MQTT connected"));
    } else {
      Serial.print(F("MQTT failed state=")); Serial.println(mqttClient.state());
      delay(3000);
    }
  }
}

void setup() {
  pinMode(STATUS_LED, OUTPUT); setLed(false); Serial.begin(115200); delay(100);
  dht.begin(); mqttClient.setServer(MQTT_HOST, MQTT_PORT); mqttClient.setKeepAlive(60); mqttClient.setCallback(mqttCallback);
  lastPublish = millis() - PUBLISH_INTERVAL_MS;
  blinkLed(5, 100, 100);
}

void loop() {
  connectWiFi(); connectMqtt();
  mqttClient.loop();
  if (!mqttClient.connected()) return;
  if (millis() - lastPublish >= PUBLISH_INTERVAL_MS) { lastPublish = millis(); publishReading(false); }
  delay(10);
}

void publishReading(bool responseOnly) {
  float humidity = dht.readHumidity(); float temperatureC = dht.readTemperature();
  if (isnan(humidity) || isnan(temperatureC)) { Serial.println(F("DHT11 read failed")); blinkLed(5, 50, 50); return; }
  float heatIndexC = dht.computeHeatIndex(temperatureC, humidity, false);
  char payload[240];
  snprintf(payload, sizeof(payload), "{\"device_id\":\"%s\",\"temperature_c\":%.2f,\"humidity\":%.2f,\"humidity_percent\":%.2f,\"heat_index_c\":%.2f,\"latitude\":-22.91790182,\"longitude\":-47.05266746,\"sensor_status\":\"ok\",\"wifi_status\":\"connected\",\"mqtt_status\":\"connected\",\"publish_ok\":true}", DEVICE_ID, temperatureC, humidity, humidity, heatIndexC);
  const char* topic = responseOnly ? MQTT_RESPONSE_TOPIC : MQTT_TOPIC;
  if (mqttClient.publish(topic, payload)) { blinkLed(1, 100, 100); Serial.println(payload); }
  else mqttClient.disconnect();
}
