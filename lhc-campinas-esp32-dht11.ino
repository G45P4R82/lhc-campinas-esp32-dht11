#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "secrets.h"

// ---------------- Configurações ----------------
#define DHTPIN 5
#define DHTTYPE DHT11

#define DEVICE_ID "iot-sensor-externo-lhc-001"

#define MQTT_TOPIC          "sensores/iot-sensor-externo-lhc-001/ambiente"
#define MQTT_RESPONSE_TOPIC "sensores/iot-sensor-externo-lhc-001/resposta"
#define MQTT_COMMAND_TOPIC  "comandos/iot-sensor-externo-lhc-001/cmd"
#define MQTT_STATUS_TOPIC   "sensores/iot-sensor-externo-lhc-001/status"

#define STATUS_LED 2
#define PUBLISH_INTERVAL_MS 30000UL

// ---------------- Objetos globais ----------------
DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastPublish = 0;
unsigned long lastMqttAttempt = 0;

// ---------------- LED ----------------
// LED ativo em nível baixo (LOW = aceso)
void setLed(bool on) {
  digitalWrite(STATUS_LED, on ? LOW : HIGH);
}

void blinkLed(int count, int onTime, int offTime) {
  for (int index = 0; index < count; index++) {
    setLed(true);
    delay(onTime);
    setLed(false);
    delay(offTime);
  }
}

// ---------------- Protótipos ----------------
void publishReading(bool responseOnly);

// ---------------- Callback MQTT ----------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, MQTT_COMMAND_TOPIC) != 0) {
    return;
  }

  String command;
  for (unsigned int index = 0; index < length; index++) {
    command += static_cast<char>(payload[index]);
  }

  Serial.print(F("[CMD] Comando recebido: "));
  Serial.println(command);

  if (command == "read") {
    publishReading(true);
  }

  if (command == "restart") {
    mqttClient.publish(MQTT_RESPONSE_TOPIC, "{\"status\":\"restarting\"}");
    delay(300);
    ESP.restart();
  }
}

// ---------------- Conexões ----------------
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting WiFi"));

  while (WiFi.status() != WL_CONNECTED) {
    blinkLed(2, 100, 100);
    delay(900);
    Serial.print('.');
  }

  Serial.print(F("\nWiFi IP: "));
  Serial.println(WiFi.localIP());
}

bool connectMqtt() {
  if (mqttClient.connected()) return true;
  if (millis() - lastMqttAttempt < 5000) return false;
  lastMqttAttempt = millis();
  blinkLed(3, 70, 70);
  if (mqttClient.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    mqttClient.subscribe(MQTT_COMMAND_TOPIC, 1);
    mqttClient.publish(MQTT_STATUS_TOPIC, "{\"status\":\"online\"}");
    Serial.println(F("MQTT connected"));
    return true;
  }
  Serial.print(F("MQTT failed state="));
  Serial.println(mqttClient.state());
  return false;
}

// ---------------- Setup ----------------
void setup() {
  pinMode(STATUS_LED, OUTPUT);
  setLed(false);

  Serial.begin(115200);
  delay(100);

  dht.begin();

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setKeepAlive(120);
  mqttClient.setBufferSize(512);
  mqttClient.setSocketTimeout(5);
  mqttClient.setCallback(mqttCallback);

  // Força a primeira publicação logo no início
  lastPublish = millis() - PUBLISH_INTERVAL_MS;

  blinkLed(5, 100, 100);
}

// ---------------- Loop ----------------
void loop() {
  connectWiFi();
  if (!connectMqtt()) {
    delay(100);
    return;
  }

  mqttClient.loop();

  if (!mqttClient.connected()) {
    return;
  }

  if (millis() - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = millis();
    publishReading(false);
  }

  delay(10);
}

// ---------------- Publicação ----------------
void publishReading(bool responseOnly) {
  Serial.println(F("[DHT] iniciando leitura"));
  float humidity     = dht.readHumidity();
  float temperatureC = dht.readTemperature();
  Serial.println(F("[DHT] leitura concluida"));

  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println(F("DHT11 read failed"));
    blinkLed(5, 50, 50);
    return;
  }

  float heatIndexC = dht.computeHeatIndex(temperatureC, humidity, false);

  char payload[384];
  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device_id\":\"%s\","
      "\"temperature_c\":%.2f,"
      "\"humidity\":%.2f,"
      "\"humidity_percent\":%.2f,"
      "\"heat_index_c\":%.2f,"
      "\"latitude\":-22.91790182,"
      "\"longitude\":-47.05266746,"
      "\"sensor_status\":\"ok\","
      "\"wifi_status\":\"connected\","
      "\"mqtt_status\":\"connected\","
      "\"publish_ok\":true"
    "}",
    DEVICE_ID,
    temperatureC,
    humidity,
    humidity,
    heatIndexC
  );

  const char* topic = responseOnly ? MQTT_RESPONSE_TOPIC : MQTT_TOPIC;
  bool ok = mqttClient.publish(topic, payload);

  // Print formatado no Serial Monitor
  Serial.println();
  Serial.println(F("========== LEITURA PUBLICADA =========="));
  Serial.print(F("Tipo            : "));
  Serial.println(responseOnly ? F("Resposta a comando (read)") : F("Publicacao periodica"));
  Serial.print(F("Topico          : "));
  Serial.println(topic);
  Serial.print(F("Dispositivo     : "));
  Serial.println(DEVICE_ID);
  Serial.println(F("---------------------------------------"));
  Serial.print(F("Temperatura     : "));
  Serial.print(temperatureC, 2);
  Serial.println(F(" C"));
  Serial.print(F("Umidade         : "));
  Serial.print(humidity, 2);
  Serial.println(F(" %"));
  Serial.print(F("Sensacao termica: "));
  Serial.print(heatIndexC, 2);
  Serial.println(F(" C"));
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Latitude        : -22.91790182"));
  Serial.println(F("Longitude       : -47.05266746"));
  Serial.println(F("Sensor          : ok"));
  Serial.print(F("WiFi            : "));
  Serial.print(WiFi.status() == WL_CONNECTED ? F("conectado") : F("desconectado"));
  Serial.print(F(" (RSSI "));
  Serial.print(WiFi.RSSI());
  Serial.println(F(" dBm)"));
  Serial.print(F("MQTT            : "));
  Serial.println(mqttClient.connected() ? F("conectado") : F("desconectado"));
  Serial.print(F("Publicacao      : "));
  Serial.println(ok ? F("OK") : F("FALHOU"));
  Serial.println(F("---------------------------------------"));
  Serial.print(F("Payload JSON    : "));
  Serial.println(payload);
  Serial.println(F("======================================="));

  if (ok) {
    blinkLed(1, 100, 100);
  } else {
    mqttClient.disconnect();
  }
}
