# Sensor Externo LHC Campinas

Firmware para ESP32-WROVER ou ESP32 DevKit V1 com DHT11. O sensor publica
temperatura, umidade e sensação térmica via MQTT como
`iot-sensor-externo-lhc-001`.

Tutorial visual completo:

```text
https://g45p4r82.github.io/lhc-campinas-esp32-dht11/
```

## Fluxo do projeto

1. O DHT11 mede temperatura e umidade.
2. O ESP32 conecta-se ao Wi-Fi.
3. O ESP32 conecta-se ao broker MQTT.
4. O firmware publica uma leitura a cada 30 segundos.
5. Um sistema pode consultar os dados ou enviar comandos MQTT.

O código está em `lhc-campinas-esp32-dht11.ino`. A função `setup()` inicializa
os componentes; a função `loop()` mantém conexões, processa comandos e publica
as leituras.

## Hardware e circuito

- ESP32-WROVER/DevKit V1, 4 MB flash.
- Conversor USB CP2102.
- DHT11 DATA -> `GPIO5`.
- DHT11 VCC -> `VIN`.
- DHT11 GND -> `GND`.
- Resistor de pull-up de `10 kOhm` entre DATA e `3V3` quando o módulo não o possuir.

> Na imagem de referência, troque somente o fio de alimentação de `3V3` para
> `VIN`. DATA continua no GPIO5 e GND continua no GND. O código usa
> `#define DHTPIN 5`; a referência antiga ao GPIO4 não corresponde ao firmware.

## MQTT

MQTT é um protocolo leve de mensagens. O ESP32 e os computadores são clientes;
o broker recebe e distribui mensagens; os tópicos organizam os canais.

| Canal | Função |
| --- | --- |
| `sensores/iot-sensor-externo-lhc-001/ambiente` | Leituras periódicas de temperatura, umidade e sensação térmica. |
| `sensores/iot-sensor-externo-lhc-001/resposta` | Resposta a uma leitura solicitada com `read`. |
| `comandos/iot-sensor-externo-lhc-001/cmd` | Comandos enviados ao ESP32. |
| `sensores/iot-sensor-externo-lhc-001/status` | Estado da conexão, como `{"status":"online"}`. |

Os canais são separados para não misturar dados, comandos, respostas e status.

## Configuração do `secrets.h`

O firmware pode receber as credenciais pelo web flasher e salvá-las na memória
NVS do ESP32. Assim, não é necessário colocar senhas reais no código.

Para configuração manual durante o desenvolvimento, copie o arquivo de exemplo:

```bash
cp secrets.example.h secrets.h
```

Preencha os valores da sua rede e do seu broker:

```cpp
#define WIFI_SSID "NOME_DA_SUA_REDE"
#define WIFI_PASSWORD "SENHA_DO_WIFI"
#define MQTT_HOST "ENDERECO_DO_BROKER"
#define MQTT_PORT 1883
#define MQTT_USERNAME "SEU_USUARIO_MQTT"
#define MQTT_PASSWORD "SUA_SENHA_MQTT"
```

O arquivo `secrets.h` está no `.gitignore` e não deve ser publicado. Se uma
senha real tiver sido exposta, altere-a no broker MQTT.

O comando serial usado pelo web flasher é:

```text
CONFIG_SET wifi_ssid|wifi_password|mqtt_host|mqtt_port|mqtt_username|mqtt_password
```

Depois de receber o comando, o ESP32 salva os valores na NVS e pode reiniciar
para conectar ao Wi-Fi e ao MQTT.

## Arduino IDE

1. Instale `DHT sensor library`, `Adafruit Unified Sensor` e `PubSubClient`.
2. Selecione `ESP32 Dev Module`.
3. Selecione a porta CP2102, normalmente `/dev/ttyUSB0`.
4. Configure o `secrets.h`.
5. Abra `lhc-campinas-esp32-dht11.ino` e carregue o firmware.
6. Use o monitor serial em `115200 baud`.

## Build e gravação com Arduino CLI

```bash
arduino-cli lib install "DHT sensor library" "Adafruit Unified Sensor" PubSubClient
arduino-cli compile --fqbn esp32:esp32:esp32 lhc-campinas-esp32-dht11
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 lhc-campinas-esp32-dht11
```

Se o upload não iniciar, segure `BOOT`, pressione e solte `EN/RESET` e depois
solte `BOOT`.

## Monitor serial e debug

```bash
arduino-cli monitor -p /dev/ttyUSB0 --config baudrate=115200
```

Mensagens importantes:

- `WiFi IP`: o ESP32 recebeu endereço na rede.
- `MQTT connected`: conexão com o broker estabelecida.
- `DHT11 read failed`: verifique alimentação, DATA e GPIO5.
- `Publicacao : OK`: publicação MQTT realizada.
- `Publicacao : FALHOU`: verifique broker, porta e credenciais.

## Teste com Mosquitto

Substitua os placeholders pelas credenciais do seu `secrets.h`.

### Acompanhar dados, respostas e status

```bash
mosquitto_sub \
  -h MQTT_HOST -p MQTT_PORT \
  -u MQTT_USERNAME -P MQTT_PASSWORD \
  -t "sensores/iot-sensor-externo-lhc-001/#" -v
```

### Solicitar uma leitura imediata

```bash
mosquitto_pub \
  -h MQTT_HOST -p MQTT_PORT \
  -u MQTT_USERNAME -P MQTT_PASSWORD \
  -t "comandos/iot-sensor-externo-lhc-001/cmd" \
  -m "read"
```

Com o `mosquitto_sub` aberto, a resposta deve aparecer em
`sensores/iot-sensor-externo-lhc-001/resposta`.

### Reiniciar o ESP32

```bash
mosquitto_pub \
  -h MQTT_HOST -p MQTT_PORT \
  -u MQTT_USERNAME -P MQTT_PASSWORD \
  -t "comandos/iot-sensor-externo-lhc-001/cmd" \
  -m "restart"
```

Esse comando reinicia o dispositivo; use-o somente quando necessário.

## Navegação do tutorial

Use `←` para voltar, `→` para avançar, `Home` para o início e `End` para o
último slide.
