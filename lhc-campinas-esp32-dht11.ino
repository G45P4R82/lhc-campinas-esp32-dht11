#include <DHT.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WiFi.h>

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
Preferences preferences;

String wifiSsid;
String wifiPassword;
String mqttHost;
String mqttUsername;
String mqttPassword;
uint16_t mqttPort = 1883;
bool configurationReady = false;

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
void loadConfiguration();
void handleSerialConfiguration();

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

// ---------------- Configuração serial ----------------
void printConfigurationStatus() {
  Serial.println(F("[CONFIG] status"));
  Serial.print(F("[CONFIG] sensor=DHT11 pin="));
  Serial.println(DHTPIN);
  Serial.print(F("[CONFIG] configured="));
  Serial.println(configurationReady ? F("true") : F("false"));
  Serial.println(F("[CONFIG] end"));
}

void saveConfiguration(
  const String& newWifiSsid,
  const String& newWifiPassword,
  const String& newMqttHost,
  uint16_t newMqttPort,
  const String& newMqttUsername,
  const String& newMqttPassword
) {
  preferences.begin("sensor", false);
  preferences.putString("wifi_ssid", newWifiSsid);
  preferences.putString("wifi_pass", newWifiPassword);
  preferences.putString("mqtt_host", newMqttHost);
  preferences.putUShort("mqtt_port", newMqttPort);
  preferences.putString("mqtt_user", newMqttUsername);
  preferences.putString("mqtt_pass", newMqttPassword);
  preferences.end();

  wifiSsid = newWifiSsid;
  wifiPassword = newWifiPassword;
  mqttHost = newMqttHost;
  mqttPort = newMqttPort;
  mqttUsername = newMqttUsername;
  mqttPassword = newMqttPassword;
  configurationReady = wifiSsid.length() > 0 && mqttHost.length() > 0;
}

void handleSerialConfiguration() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();

  if (line == "CONFIG_STATUS") {
    printConfigurationStatus();
    return;
  }

  if (line == "CONFIG_CLEAR") {
    preferences.begin("sensor", false);
    preferences.clear();
    preferences.end();
    configurationReady = false;
    Serial.println(F("CONFIG_OK cleared"));
    return;
  }

  if (!line.startsWith("CONFIG_SET ")) return;

  String values = line.substring(11);
  String fields[6];
  int start = 0;
  for (int index = 0; index < 5; index++) {
    int separator = values.indexOf('|', start);
    if (separator < 0) {
      Serial.println(F("CONFIG_ERROR invalid_fields"));
      return;
    }
    fields[index] = values.substring(start, separator);
    start = separator + 1;
  }
  fields[5] = values.substring(start);

  uint16_t newPort = static_cast<uint16_t>(fields[3].toInt());
  if (fields[0].length() == 0 || fields[2].length() == 0 || newPort == 0) {
    Serial.println(F("CONFIG_ERROR missing_required_fields"));
    return;
  }

  saveConfiguration(fields[0], fields[1], fields[2], newPort, fields[4], fields[5]);
  mqttClient.setServer(mqttHost.c_str(), mqttPort);
  Serial.println(F("CONFIG_OK saved"));
}

void loadConfiguration() {
  preferences.begin("sensor", true);
  wifiSsid = preferences.getString("wifi_ssid", "");
  wifiPassword = preferences.getString("wifi_pass", "");
  mqttHost = preferences.getString("mqtt_host", "");
  mqttPort = preferences.getUShort("mqtt_port", 1883);
  mqttUsername = preferences.getString("mqtt_user", "");
  mqttPassword = preferences.getString("mqtt_pass", "");
  preferences.end();
  configurationReady = wifiSsid.length() > 0 && mqttHost.length() > 0;
}

// ---------------- Conexões ----------------
void connectWiFi() {
  if (!configurationReady) return;
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

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
  if (!configurationReady) return false;
  if (mqttClient.connected()) return true;
  if (millis() - lastMqttAttempt < 5000) return false;
  lastMqttAttempt = millis();
  blinkLed(3, 70, 70);
  if (mqttClient.connect(DEVICE_ID, mqttUsername.c_str(), mqttPassword.c_str())) {
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

  loadConfiguration();
  mqttClient.setServer(mqttHost.c_str(), mqttPort);
  mqttClient.setKeepAlive(120);
  mqttClient.setBufferSize(512);
  mqttClient.setSocketTimeout(5);
  mqttClient.setCallback(mqttCallback);

  // Força a primeira publicação logo no início
  lastPublish = millis() - PUBLISH_INTERVAL_MS;

  blinkLed(5, 100, 100);
  Serial.println(F("[CONFIG] envie CONFIG_SET ssid|senha_wifi|host_mqtt|porta|usuario|senha_mqtt"));
  printConfigurationStatus();
}

// ---------------- Loop ----------------
void loop() {
  handleSerialConfiguration();
  if (!configurationReady) {
    delay(20);
    return;
  }
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
