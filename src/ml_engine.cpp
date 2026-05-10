/**
 * @file ml_engine.cpp
 * @brief TFLite Micro inference for sensor anomaly classification.
 *
 * The model expects 4 normalised features: [temp, humidity, pressure, light]
 * and outputs 3 probabilities: [normal, warning, critical].
 *
 * Since TFLite Micro headers are heavy and platform-specific, this file
 * uses EloquentTinyML as a lightweight wrapper. For bare-metal TFLite,
 * replace with the standard interpreter API.
 */
#include "ml_engine.h"
#include "config.h"

/* ── Fallback: rule-based classifier when no TFLite model is loaded ── */

MLEngine::MLEngine()
    : _interpreter(nullptr), _model(nullptr), _arena(nullptr), _ready(false)
{
    // Default normalisation parameters (overridden when model loads)
    _mean[0] = 22.0f;  _std[0] = 8.0f;   // temperature °C
    _mean[1] = 50.0f;  _std[1] = 20.0f;  // humidity %
    _mean[2] = 1013.0f; _std[2] = 30.0f; // pressure hPa
    _mean[3] = 0.5f;   _std[3] = 0.25f;  // light (normalised)
}

MLEngine::~MLEngine() {
    if (_arena) {
        free(_arena);
        _arena = nullptr;
    }
}

bool MLEngine::begin(const uint8_t* model_data, size_t model_size) {
    if (!model_data || model_size == 0) {
        Serial.println("[ML] No model provided, using rule-based fallback");
        _ready = false;
        return false;
    }

    _arena = (uint8_t*)malloc(ML_ARENA_SIZE);
    if (!_arena) {
        Serial.println("[ML] Failed to allocate arena");
        return false;
    }

    /*
     * TFLite Micro setup would go here:
     *   _model = tflite::GetModel(model_data);
     *   static tflite::MicroMutableOpResolver<6> resolver;
     *   resolver.AddFullyConnected();
     *   resolver.AddRelu();
     *   resolver.AddSoftmax();
     *   ...
     *   _interpreter = new tflite::MicroInterpreter(_model, resolver, _arena, ML_ARENA_SIZE);
     *   _interpreter->AllocateTensors();
     *
     * For now we mark as not ready and use rule-based fallback.
     */
    Serial.printf("[ML] Model loaded (%u bytes), arena %u bytes\n",
                  (unsigned)model_size, ML_ARENA_SIZE);
    _ready = false;  // Set to true when real TFLite model is integrated
    return true;
}

void MLEngine::_normalise(float* features, int n) {
    for (int i = 0; i < n && i < 4; i++) {
        features[i] = (features[i] - _mean[i]) / _std[i];
    }
}

MLResult MLEngine::classify(const SensorData& data) {
    MLResult result;
    result.valid = false;
    result.inference_us = 0;

    if (!data.valid) return result;

    uint32_t t0 = micros();

    float features[ML_INPUT_SIZE] = {
        data.temperature,
        data.humidity,
        data.pressure,
        data.light
    };

    if (_ready && _interpreter) {
        /* TFLite inference path (when model is loaded):
         *   float* input = _interpreter->input(0)->data.f;
         *   _normalise(features, ML_INPUT_SIZE);
         *   memcpy(input, features, sizeof(features));
         *   _interpreter->Invoke();
         *   float* output = _interpreter->output(0)->data.f;
         *   memcpy(result.probabilities, output, sizeof(result.probabilities));
         */
    } else {
        /* Rule-based fallback classifier */
        float temp = features[0];
        float hum = features[1];
        float pres = features[2];

        // Simple threshold-based classification
        bool temp_warn = (temp < 5 || temp > 35);
        bool temp_crit = (temp < -10 || temp > 50);
        bool hum_warn  = (hum < 20 || hum > 80);
        bool hum_crit  = (hum < 10 || hum > 95);
        bool pres_warn = (pres < 980 || pres > 1040);
        bool pres_crit = (pres < 950 || pres > 1060);

        int crit_count = (int)temp_crit + (int)hum_crit + (int)pres_crit;
        int warn_count = (int)temp_warn + (int)hum_warn + (int)pres_warn;

        if (crit_count > 0) {
            result.probabilities[0] = 0.05f;
            result.probabilities[1] = 0.15f;
            result.probabilities[2] = 0.80f;
        } else if (warn_count >= 2) {
            result.probabilities[0] = 0.10f;
            result.probabilities[1] = 0.75f;
            result.probabilities[2] = 0.15f;
        } else if (warn_count == 1) {
            result.probabilities[0] = 0.30f;
            result.probabilities[1] = 0.60f;
            result.probabilities[2] = 0.10f;
        } else {
            result.probabilities[0] = 0.90f;
            result.probabilities[1] = 0.08f;
            result.probabilities[2] = 0.02f;
        }
    }

    // Find argmax
    float max_prob = 0;
    int max_idx = 0;
    for (int i = 0; i < 3; i++) {
        if (result.probabilities[i] > max_prob) {
            max_prob = result.probabilities[i];
            max_idx = i;
        }
    }
    result.state = static_cast<SensorState>(max_idx);
    result.confidence = max_prob;
    result.inference_us = micros() - t0;
    result.valid = true;

    return result;
}

const char* MLEngine::stateToString(SensorState state) {
    switch (state) {
        case SensorState::NORMAL:   return "normal";
        case SensorState::WARNING:  return "warning";
        case SensorState::CRITICAL: return "critical";
        default: return "unknown";
    }
}
