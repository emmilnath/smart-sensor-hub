/**
 * @file mqtt_client.cpp
 * @brief MQTT client for publishing sensor telemetry and receiving commands.
 */
#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

MQTTClient* MQTTClient::_instance = nullptr;

MQTTClient::MQTTClient()
    : _mqtt(_wifi), _cmd_cb(nullptr), _client_id(nullptr),
      _user(nullptr), _pass(nullptr)
{
    _instance = this;
}

void MQTTClient::begin(const char* broker, uint16_t port,
                        const char* client_id,
                        const char* user, const char* pass) {
    _client_id = client_id;
    _user = user;
    _pass = pass;
    _mqtt.setServer(broker, port);
    _mqtt.setCallback(_static_callback);
    _mqtt.setBufferSize(512);
}

bool MQTTClient::connect() {
    if (_mqtt.connected()) return true;

    Serial.print("[MQTT] Connecting to broker... ");

    bool ok;
    if (_user && strlen(_user) > 0) {
        ok = _mqtt.connect(_client_id, _user, _pass);
    } else {
        ok = _mqtt.connect(_client_id);
    }

    if (ok) {
        Serial.println("connected");
        _mqtt.subscribe(MQTT_TOPIC_CMD);
        return true;
    }

    Serial.printf("failed (rc=%d)\n", _mqtt.state());
    return false;
}

void MQTTClient::loop() {
    if (!_mqtt.connected()) {
        connect();
    }
    _mqtt.loop();
}

bool MQTTClient::publishSensorData(const SensorData& data) {
    if (!_mqtt.connected()) return false;

    JsonDocument doc;
    doc["temp"] = round(data.temperature * 100) / 100.0;
    doc["hum"]  = round(data.humidity * 100) / 100.0;
    doc["pres"] = round(data.pressure * 10) / 10.0;
    doc["light"] = round(data.light * 1000) / 1000.0;
    doc["ts"]   = data.timestamp;

    char buf[256];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    return _mqtt.publish(MQTT_TOPIC_DATA, buf, len);
}

bool MQTTClient::publishMLResult(const char* classification, float confidence) {
    if (!_mqtt.connected()) return false;

    JsonDocument doc;
    doc["class"] = classification;
    doc["conf"]  = round(confidence * 1000) / 1000.0;
    doc["ts"]    = millis();

    char buf[128];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    return _mqtt.publish(MQTT_TOPIC_ML, buf, len);
}

void MQTTClient::onCommand(CommandCallback cb) {
    _cmd_cb = cb;
}

bool MQTTClient::connected() const {
    return _mqtt.connected();
}

void MQTTClient::_static_callback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->_handleMessage(topic, payload, length);
    }
}

void MQTTClient::_handleMessage(char* topic, byte* payload, unsigned int length) {
    // Null-terminate payload
    char msg[256];
    size_t copy_len = (length < sizeof(msg) - 1) ? length : sizeof(msg) - 1;
    memcpy(msg, payload, copy_len);
    msg[copy_len] = '\0';

    Serial.printf("[MQTT] Received on %s: %s\n", topic, msg);

    if (_cmd_cb) {
        _cmd_cb(topic, msg);
    }
}
