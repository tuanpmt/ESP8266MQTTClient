#ifndef _MQTT_TRANSPORT_H_
#define _MQTT_TRANSPORT_H_
// WebSocket protocol constants
// First byte
#define WS_FIN            0x80
#define WS_OPCODE_TEXT    0x01
#define WS_OPCODE_BINARY  0x02
#define WS_OPCODE_CLOSE   0x08
#define WS_OPCODE_PING    0x09
#define WS_OPCODE_PONG    0x0a
// Second byte
#define WS_MASK           0x80
#define WS_SIZE16         126
#define WS_SIZE64         127
#define MAX_WEBSOCKET_HEADER_SIZE 10

class MQTTTransportTraits
{
public:
    MQTTTransportTraits();
    MQTTTransportTraits(bool secure);
    virtual std::unique_ptr<WiFiClient> create();
    virtual bool connect(WiFiClient* client, const char* host, int port, const char *path);
    virtual int write(WiFiClient* client, unsigned char *data, int size);
    virtual int read(WiFiClient* client, unsigned char *data, int size);
protected:
    std::unique_ptr<WiFiClient> _tcp;
    bool _isSecure;
};

class MQTTWSTraits : public MQTTTransportTraits
{
public:
    MQTTWSTraits();
    MQTTWSTraits(bool secure);
    bool connect(WiFiClient* client, const char* host, int port, const char *path) override;
    int write(WiFiClient* client, unsigned char *data, int size) override;
    int read(WiFiClient* client, unsigned char *data, int size) override;
protected:
    String _key;
    bool _isSecure;
};
#endif