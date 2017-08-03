# MQTT Client library for ESP8266 Arduino

This is MQTT client library for ESP8266, using mqtt_msg package from [MQTT client library for Contiki](https://github.com/esar/contiki-mqtt) and use for ESP8266 NON-OS SDK [esp_mqtt](https://github.com/tuanpmt/esp_mqtt)

Features:

- Support subscribing, publishing, authentication, will messages, keep alive pings and all 3 QoS levels (it should be a fully functional client).

## Requirements
- ESP8266WiFi
- WiFiClientSecure

## Status
- Support 3 type of qos (0, 1, 2) and outbox
- only mqtt over TCP

## MQTT URI Scheme

- `mqtt://[username][:password@]hostname[:port][#clientId]`
    + `mqtt` for MQTT over TCP 
    + `ws` for MQTT over Websocket
- Example:
    + **Full** `mqtt://username:password@test.mosquitto.org:1883#my_client_id`
    + **Websocket** `ws://username:password@test.mosquitto.org:1883/mqtt#my_client_id`
    + **Minimal** `mqtt://test.mosquitto.org`, with `user`, `pass` = NULL, port = 1883, client id = "ESP_" + ESP.getChipId()

## API 
### Setup  
- bool begin(String uri);
- bool begin(String uri, int keepalive, bool clean_session); 
- bool begin(String uri, LwtOptions lwt);
- bool begin(String uri, LwtOptions lwt, int keepalive, bool clean_session)  

### Events
- void onConnect(THandlerFunction fn);
- void onDisconnect(THandlerFunction fn);
- void onSubscribe(THandlerFunction_PubSub fn);
- void onPublish(THandlerFunction_PubSub fn);
- void onData(THandlerFunction_Data fn);

### Pub/Sub 
- int subscribe(String topic);
- int subscribe(String topic, uint8_t qos);
- int publish(String topic, String data);
- int publish(String topic, String data, int qos, int retain);

## License

Copyright (c) 2016 Tuan PM (https://twitter.com/tuanpmt) 
ESP8266 port (c) 2016 Ivan Grokhotkov (ivan@esp8266.com)

License Apache License