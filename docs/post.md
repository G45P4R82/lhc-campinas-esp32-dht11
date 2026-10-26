# Sensor Externo LHC Campinas: ESP32, DHT11 e MQTT

## Objetivo do projeto

Este projeto tem como objetivo criar um sensor IoT simples, acessível e fácil
de acompanhar remotamente. Um ESP32 conectado a um sensor DHT11 mede a
temperatura e a umidade do ambiente e publica essas informações por MQTT.

A proposta é transformar uma leitura física do ambiente em dados que podem ser
consultados por sistemas de monitoramento, dashboards ou pelo terminal usando
as ferramentas do Mosquitto.

## O que o dispositivo faz

Depois de ligado e configurado, o ESP32:

1. Inicializa o sensor DHT11.
2. Conecta-se à rede Wi-Fi configurada.
3. Conecta-se ao broker MQTT.
4. Publica temperatura, umidade e sensação térmica a cada 30 segundos.
5. Aceita comandos remotos para fazer uma leitura imediata ou reiniciar.

O dispositivo usa a identidade:

```text
iot-sensor-externo-lhc-001
```

## Por que usar MQTT

MQTT é um protocolo leve, adequado para dispositivos IoT com poucos recursos.
O ESP32 não precisa conhecer diretamente todos os sistemas que vão consumir os
dados. Ele publica uma mensagem no broker, e o broker distribui essa mensagem
para os clientes inscritos no tópico correspondente.

Essa separação facilita a integração com aplicações diferentes, como um
dashboard, um sistema de alertas ou uma ferramenta de linha de comando.

## Organização dos canais

O projeto separa as mensagens em quatro tópicos:

| Tópico | Objetivo |
| --- | --- |
| `sensores/iot-sensor-externo-lhc-001/ambiente` | Leituras periódicas do DHT11. |
| `sensores/iot-sensor-externo-lhc-001/resposta` | Resposta a uma leitura imediata. |
| `comandos/iot-sensor-externo-lhc-001/cmd` | Comandos enviados ao ESP32. |
| `sensores/iot-sensor-externo-lhc-001/status` | Estado da conexão do dispositivo. |

Separar os canais evita misturar dados de medição, comandos e status. Também
permite que cada aplicação assine apenas o que precisa.

## Circuito

O DHT11 é conectado ao ESP32 da seguinte forma:

- DATA no `GPIO5`.
- VCC no `VIN`.
- GND no `GND`.

O firmware usa `#define DHTPIN 5`, portanto a ligação do fio DATA precisa
corresponder a esse pino. A imagem de referência do projeto mostra a montagem;
nela, deve-se trocar somente a alimentação de `3V3` para `VIN`.

## Resultado esperado

Uma publicação MQTT contém dados semelhantes a:

```json
{
  "device_id": "iot-sensor-externo-lhc-001",
  "temperature_c": 25.40,
  "humidity": 60.00,
  "humidity_percent": 60.00,
  "heat_index_c": 26.10,
  "sensor_status": "ok",
  "wifi_status": "connected",
  "mqtt_status": "connected",
  "publish_ok": true
}
```

Com isso, o projeto fornece uma base para observar as condições ambientais de
um local e evoluir posteriormente para armazenamento histórico, gráficos,
alertas e automações.

## Teste rápido

Para acompanhar os dados recebidos pelo broker:

```bash
mosquitto_sub \
  -h MQTT_HOST -p MQTT_PORT \
  -u MQTT_USERNAME -P MQTT_PASSWORD \
  -t "sensores/iot-sensor-externo-lhc-001/#" -v
```

Para solicitar uma leitura imediata:

```bash
mosquitto_pub \
  -h MQTT_HOST -p MQTT_PORT \
  -u MQTT_USERNAME -P MQTT_PASSWORD \
  -t "comandos/iot-sensor-externo-lhc-001/cmd" \
  -m "read"
```

As credenciais devem ser preenchidas no arquivo local `secrets.h`, que não deve
ser publicado no repositório.
