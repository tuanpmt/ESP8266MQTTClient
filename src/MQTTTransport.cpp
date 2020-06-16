/*
ESP8266 MQTT Client library for ESP8266 Arduino
Version 0.1
Copyright (c) 2016 Tuan PM (tuanpm@live.com)
ESP8266 port (c) 2015 Ivan Grokhotkov (ivan@esp8266.com)
License (MIT license):
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <StreamString.h>
#include <base64.h>
#include <Hash.h>
#include "ESP8266MQTTClient.h"
#include "MQTTTransport.h"

MQTTTransportTraits::MQTTTransportTraits(): _isSecure(false)
{
}
MQTTTransportTraits::MQTTTransportTraits(bool secure): _isSecure(secure)
{
}
std::unique_ptr<WiFiClient> MQTTTransportTraits::create()
{
	if(_isSecure)
		return std::unique_ptr<WiFiClient>(new WiFiClientSecure());
	return std::unique_ptr<WiFiClient>(new WiFiClient());
}
bool MQTTTransportTraits::connect(WiFiClient* client, const char* host, int port, const char *path)
{
	if(_isSecure) {
		WiFiClientSecure *client = (WiFiClientSecure*) client;
	}
	return client->connect(host, port);
}
int MQTTTransportTraits::write(WiFiClient* client, unsigned char *data, int size)
{
	if(_isSecure) {
		WiFiClientSecure *client = (WiFiClientSecure*) client;
	}
	return client->write(reinterpret_cast<const char*>(data), size);
}
int MQTTTransportTraits::read(WiFiClient* client, unsigned char *data, int size)
{
	if(_isSecure) {
		WiFiClientSecure *client = (WiFiClientSecure*) client;
	}
	return client->read(data, size);
}

/**
 * MQTT Over WS
 */

MQTTWSTraits::MQTTWSTraits(): _isSecure(false)
{
	randomSeed(RANDOM_REG32);
}
MQTTWSTraits::MQTTWSTraits(bool secure): _isSecure(secure)
{
	randomSeed(RANDOM_REG32);
}


bool MQTTWSTraits::connect(WiFiClient* client, const char* host, int port, const char *path)
{
	uint8_t randomKey[16] = { 0 }, timeout = 0;
	int bite;
	bool foundupgrade = false;
	String serverKey, temp, acceptKey;

	for(uint8_t i = 0; i < sizeof(randomKey); i++) {
		randomKey[i] = random(0xFF);
	}
	_key = base64::encode(randomKey, 16);
	LOG("Key: %s\r\n", _key.c_str());
	String handshake = "GET "+ String(path) +" HTTP/1.1\r\n"
	                   "Connection: Upgrade\r\n"
	                   "Upgrade: websocket\r\n"
	                   "Host: " + String(host) + ":" + String(port) + "\r\n"
	                   "Sec-WebSocket-Version: 13\r\n"
	                   "Origin: file://\r\n"
	                   "Sec-WebSocket-Protocol: mqttv3.1\r\n"
	                   "User-Agent: ESP8266MQTTClient\r\n"
	                   "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n"
	                   "Sec-WebSocket-Key: " + _key + "\r\n\r\n";
	if(!client->connect(host, port)) {
		LOG("ERROR: Can't connect \r\n");
		return false;
	}
	client->write(handshake.c_str(), handshake.length());

	while(client->connected() && !client->available()) {
		delay(100);
		if(timeout++ > 10) {
			LOG("ERROR Read timeout\r\n");
			return false;
		}
	}
	while((bite = client->read()) != -1) {

		temp += (char)bite;

		if((char)bite == '\n') {
			if(!foundupgrade && (temp.startsWith("Upgrade: websocket") || temp.startsWith("upgrade: websocket"))) {
				foundupgrade = true;
			} else if(temp.startsWith("Sec-WebSocket-Accept: ") || temp.startsWith("sec-websocket-accept: ")) {
				serverKey = temp.substring(22, temp.length() - 2); // Don't save last CR+LF
			}
			LOG("Data=%s", temp.c_str());
			temp = "";
		}

		if(!client->available()) {
			delay(100);
		}
	}
	_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	uint8_t sha1HashBin[20] = { 0 };
	sha1(_key, &sha1HashBin[0]);
	acceptKey = base64::encode(sha1HashBin, 20);
	acceptKey.trim();
	LOG("AcceptKey: %s\r\n", acceptKey.c_str());
	LOG("ServerKey: %s\r\n", serverKey.c_str());
	timeout = 0;
	return acceptKey == serverKey;
}

int MQTTWSTraits::write(WiFiClient* client, unsigned char *data, int size)
{
	char *mask, *data_buffer;
	int header_len = 0;
	data_buffer = (char *) malloc(MAX_WEBSOCKET_HEADER_SIZE + size);
	if(data_buffer == NULL)
		return -1;
	// Opcode; final fragment
	data_buffer[header_len++] = WS_OPCODE_BINARY | WS_FIN;

	// NOTE: no support for > 16-bit sized messages
	if(size > 125) {
		data_buffer[header_len++] = WS_SIZE16 | WS_MASK;
		data_buffer[header_len++] = (uint8_t)(size >> 8);
		data_buffer[header_len++] = (uint8_t)(size & 0xFF);
	} else {
		data_buffer[header_len++] = (uint8_t)(size | WS_MASK);
	}
	mask = &data_buffer[header_len];
	data_buffer[header_len++] = random(0, 256);
	data_buffer[header_len++] = random(0, 256);
	data_buffer[header_len++] = random(0, 256);
	data_buffer[header_len++] = random(0, 256);

	for(int i = 0; i < size; ++i) {
		data_buffer[header_len++] = (data[i] ^ mask[i % 4]);
	}
	client->write(reinterpret_cast<const char*>(data_buffer), header_len);
	client->flush();
	free(data_buffer);
	return size;
}

// https://tools.ietf.org/html/rfc6455#section-5.2
int MQTTWSTraits::read(WiFiClient *client, unsigned char *data, int size) {
    if (size < 0) {
        return -1;
    } else if (size == 0) {
        return 0;
    }
    uint32_t max_buffer_length = size;

    if (client->available() < 2) {
        return 0;
    }
    uint8_t fixed_size_header[2];
    client->peekBytes(fixed_size_header, 2);
    uint8_t opcode = fixed_size_header[0] & 0x0F;
    bool mask = (fixed_size_header[1] >> 7) & 0x01;
    uint32_t payload_length = fixed_size_header[1] & 0x7F;

    uint8_t header_length = 2;
    if (payload_length == 126) {
        header_length += 2;
    } else if (payload_length == 127) {
        header_length += 8;
    }
    if (mask) {
        header_length += 4;
    }

    if (client->available() < header_length) {
        return 0;
    }

    uint8_t header[header_length];
    client->read(header, header_length);
    uint8_t index = 2;

    if (payload_length == 126) {
        payload_length = header[index] << 8 | header[index + 1];
        index += 2;
    } else if (payload_length == 127) {
        if (header[index] != 0 || header[index + 1] != 0 || header[index + 2] != 0 || header[index + 3] != 0) {
            // really too big!
            payload_length = 0xFFFFFFFF;
        } else {
            payload_length =
                    header[index + 4] << 24 | header[index + 5] << 16 | header[index + 6] << 8 | header[index + 7];
        }
        index += 8;
    }

    uint8_t *mask_key_ptr = NULL;
    if (mask) {
        mask_key_ptr = &header[index];
        index += 4;
    }

    // read payload but only so much that it fits into the buffer size except when more is already available
    uint32_t buffer_length;
    if (max_buffer_length < payload_length) {
        uint32_t payload_avail = client->available();
        if (payload_avail > max_buffer_length && payload_avail < payload_length) {
            buffer_length = payload_avail;
        } else {
            buffer_length = size;
        }
    } else {
        buffer_length = payload_length;
    }

    uint8_t buffer[buffer_length];
    uint32_t read_length = client->read(buffer, buffer_length);
    uint32_t copy_length = read_length;
    if (max_buffer_length < copy_length) {
        copy_length = max_buffer_length;
    }

    uint8_t *data_ptr = buffer;

    if (opcode == 0x00 || opcode == 0x01 || opcode == 0x02) {
        if (mask) {
            for (size_t i = 0; i < copy_length; i++) {
                data[i] = (data_ptr[i] ^ mask_key_ptr[i % 4]);
            }
        } else {
            memcpy(data, data_ptr, copy_length);
        }

        return copy_length;
    } else {
        return 0;
    }
}
