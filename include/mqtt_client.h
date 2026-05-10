/**
 * @file mqtt_client.h
 * @brief MQTT publish/subscribe wrapper for sensor telemetry.
 */
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "sensor_manager.h"

/* Callback type for incoming commands */
typedef void (*CommandCallback)(const char* topic, const char* payload);

class MQTTClient {
public:
    MQTTClient();

    /**
     * Configure broker connection parameters.
     */
    void begin(const char* broker, uint16_t port,
               const char* client_id,
               const char* user = nullptr, const char* pass = nullptr);

    /**
     * Connect to broker. Returns true on success.
     */
    bool connect();

    /**
     * Maintain connection and process incoming messages. Call in loop().
     */
    void loop();

    /**
     * Publish sensor data as JSON.
     */
    bool publishSensorData(const SensorData& data);

    /**
     * Publish ML inference result.
     */
    bool publishMLResult(const char* classification, float confidence);

    /**
     * Subscribe to command topic and register callback.
     */
    void onCommand(CommandCallback cb);

    /**
     * Check connection status.
     */
    bool connected() const;

private:
    WiFiClient _wifi;
    PubSubClient _mqtt;
    CommandCallback _cmd_cb;
    const char* _client_id;
    const char* _user;
    const char* _pass;

    static void _static_callback(char* topic, byte* payload, unsigned int length);
    static MQTTClient* _instance;
    void _handleMessage(char* topic, byte* payload, unsigned int length);
};

#endif /* MQTT_CLIENT_H */
