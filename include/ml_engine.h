/**
 * @file ml_engine.h
 * @brief TensorFlow Lite Micro inference engine for sensor anomaly detection.
 */
#ifndef ML_ENGINE_H
#define ML_ENGINE_H

#include <Arduino.h>
#include "sensor_manager.h"

/* Classification labels */
enum class SensorState : uint8_t {
    NORMAL   = 0,
    WARNING  = 1,
    CRITICAL = 2,
};

struct MLResult {
    SensorState state;
    float confidence;      // 0.0 – 1.0
    float probabilities[3]; // [normal, warning, critical]
    uint32_t inference_us; // microseconds
    bool valid;
};

class MLEngine {
public:
    MLEngine();
    ~MLEngine();

    /**
     * Load TFLite model and allocate tensors.
     * @param model_data  Pointer to model flatbuffer (from .h include)
     * @param model_size  Size in bytes
     * @return true on success
     */
    bool begin(const uint8_t* model_data, size_t model_size);

    /**
     * Run inference on sensor data.
     */
    MLResult classify(const SensorData& data);

    /**
     * Get human-readable label for a state.
     */
    static const char* stateToString(SensorState state);

private:
    /* Opaque TFLite internals (to avoid exposing TFLite headers) */
    void* _interpreter;
    void* _model;
    uint8_t* _arena;
    bool _ready;

    /* Feature scaling parameters (from training) */
    float _mean[4];
    float _std[4];

    void _normalise(float* features, int n);
};

#endif /* ML_ENGINE_H */
