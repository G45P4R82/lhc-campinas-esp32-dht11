# Sensor Externo LHC Campinas

Firmware/tutorial para ESP32-WROVER ou ESP32 DevKit V1 com DHT11 no pino
`D4/GPIO4`. O sensor publica temperatura, umidade e sensação térmica no
Arandu como `iot-sensor-externo-lhc-001`.

## Hardware

- ESP32-WROVER/DevKit V1, 4 MB flash.
- Conversor USB CP2102.
- DHT11 DATA -> `D4/GPIO4`.
- DHT11 VCC -> `3V3`.
- DHT11 GND -> `GND`.
- Resistor de pull-up de `10 kOhm` entre DATA e `3V3` quando o módulo não o possuir.

## Arduino IDE

1. Instale `DHT sensor library`, `Adafruit Unified Sensor` e `PubSubClient`.
2. Selecione `ESP32 Dev Module`.
3. Selecione a porta CP2102, normalmente `/dev/ttyUSB0`.
4. Copie `secrets.example.h` para `secrets.h` e preencha Wi-Fi/MQTT.
5. Abra `lhc-campinas-esp32-dht11.ino` e carregue o firmware.
6. Use o monitor serial em `115200 baud`.

Se a gravação não iniciar, segure `BOOT`, pressione `EN/RESET`, solte `EN` e
depois solte `BOOT`.

## Arduino CLI

```bash
arduino-cli lib install "DHT sensor library" "Adafruit Unified Sensor" PubSubClient
arduino-cli compile --fqbn esp32:esp32:esp32 lhc-campinas-esp32-dht11
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 lhc-campinas-esp32-dht11
arduino-cli monitor -p /dev/ttyUSB0 --config baudrate=115200
```

## MQTT

```text
Identidade: iot-sensor-externo-lhc-001
Dados: sensores/iot-sensor-externo-lhc-001/ambiente
Resposta: sensores/iot-sensor-externo-lhc-001/resposta
Comandos: comandos/iot-sensor-externo-lhc-001/cmd
Regiao: Sudeste
Localizacao: Campinas
```

O firmware publica a cada 30 segundos, reconecta Wi-Fi/MQTT,
envia heartbeat online e aceita os comandos `read` e `restart`.

Credenciais reais nunca devem ser commitadas neste repositorio publico.

## Tutorial em formato de slides

O tutorial visual está em `docs/index.html` e apresenta montagem, configuração
do `secrets.h`, quatro conceitos básicos de MQTT, funcionamento do firmware,
canais MQTT e testes com Mosquitto. Use `←` para voltar, `→` para avançar,
`Home` para o início e `End` para o último slide.

O arquivo `secrets.h` está protegido pelo `.gitignore`. Nunca remova essa
proteção nem publique senhas Wi-Fi ou MQTT. Se uma credencial real tiver sido
exposta, altere-a no broker.
