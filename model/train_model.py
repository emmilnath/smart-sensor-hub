"""Train a small anomaly classifier for ESP32 deployment.

Generates synthetic sensor data, trains a tiny neural network,
and exports to TFLite format for microcontroller deployment.

Usage:
    python train_model.py
    python train_model.py --epochs 50 --output model.tflite
"""
from __future__ import annotations
import argparse
import pathlib

import numpy as np

try:
    import tensorflow as tf
    HAS_TF = True
except ImportError:
    HAS_TF = False
    print("TensorFlow not installed — generating synthetic model header only")


# ── Synthetic data generation ────────────────────────
def generate_data(n_samples: int = 10000, seed: int = 42):
    """Generate synthetic sensor data with labels.

    Classes:
        0 = normal: comfortable indoor conditions
        1 = warning: borderline conditions
        2 = critical: out-of-range conditions
    """
    rng = np.random.RandomState(seed)

    features = []
    labels = []

    # Normal: 60% of data
    n_normal = int(n_samples * 0.6)
    temp = rng.normal(22, 3, n_normal)
    hum = rng.normal(50, 10, n_normal)
    pres = rng.normal(1013, 10, n_normal)
    light = rng.uniform(0.2, 0.8, n_normal)
    features.append(np.column_stack([temp, hum, pres, light]))
    labels.append(np.zeros(n_normal, dtype=int))

    # Warning: 25% of data
    n_warn = int(n_samples * 0.25)
    temp = rng.normal(35, 5, n_warn)
    hum = rng.normal(80, 8, n_warn)
    pres = rng.normal(990, 15, n_warn)
    light = rng.uniform(0.0, 0.3, n_warn)
    features.append(np.column_stack([temp, hum, pres, light]))
    labels.append(np.ones(n_warn, dtype=int))

    # Critical: 15% of data
    n_crit = n_samples - n_normal - n_warn
    temp = rng.normal(50, 8, n_crit)
    hum = rng.normal(95, 5, n_crit)
    pres = rng.normal(960, 20, n_crit)
    light = rng.uniform(0.8, 1.0, n_crit)
    features.append(np.column_stack([temp, hum, pres, light]))
    labels.append(np.full(n_crit, 2, dtype=int))

    X = np.vstack(features).astype(np.float32)
    y = np.concatenate(labels)

    # Shuffle
    idx = rng.permutation(len(X))
    return X[idx], y[idx]


def build_model():
    """Tiny 3-layer MLP suitable for TFLite Micro."""
    model = tf.keras.Sequential([
        tf.keras.layers.Input(shape=(4,)),
        tf.keras.layers.Dense(16, activation="relu"),
        tf.keras.layers.Dense(8, activation="relu"),
        tf.keras.layers.Dense(3, activation="softmax"),
    ])
    model.compile(
        optimizer=tf.keras.optimizers.Adam(1e-3),
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )
    return model


def export_tflite(model, output_path: str):
    """Convert Keras model to TFLite."""
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite_model = converter.convert()

    with open(output_path, "wb") as f:
        f.write(tflite_model)

    print(f"Saved TFLite model: {output_path} ({len(tflite_model)} bytes)")
    return tflite_model


def export_c_header(tflite_bytes: bytes, header_path: str):
    """Convert TFLite model to C header for embedding in firmware."""
    with open(header_path, "w") as f:
        f.write("// Auto-generated — do not edit\n")
        f.write("#ifndef SENSOR_MODEL_H\n")
        f.write("#define SENSOR_MODEL_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"const unsigned int sensor_model_len = {len(tflite_bytes)};\n")
        f.write("alignas(8) const uint8_t sensor_model_data[] = {\n")

        for i, byte in enumerate(tflite_bytes):
            if i % 16 == 0:
                f.write("    ")
            f.write(f"0x{byte:02x},")
            if i % 16 == 15:
                f.write("\n")
            else:
                f.write(" ")

        f.write("\n};\n\n")
        f.write("#endif /* SENSOR_MODEL_H */\n")

    print(f"Saved C header: {header_path}")


def main():
    parser = argparse.ArgumentParser(description="Train sensor anomaly model")
    parser.add_argument("--epochs", type=int, default=30)
    parser.add_argument("--samples", type=int, default=10000)
    parser.add_argument("--output", type=str, default="sensor_model.tflite")
    args = parser.parse_args()

    out_dir = pathlib.Path(__file__).parent
    X, y = generate_data(args.samples)
    print(f"Dataset: {len(X)} samples, {4} features, {3} classes")

    if not HAS_TF:
        # Generate a dummy C header for compilation testing
        dummy_bytes = bytes([0x00] * 64)
        export_c_header(dummy_bytes, str(out_dir / "sensor_model.h"))
        print("Dummy model header generated (install TensorFlow for real training)")
        return

    # Normalise
    mean = X.mean(axis=0)
    std = X.std(axis=0)
    X_norm = (X - mean) / std
    print(f"Normalisation — mean: {mean}, std: {std}")

    # Split
    split = int(len(X_norm) * 0.8)
    X_train, X_test = X_norm[:split], X_norm[split:]
    y_train, y_test = y[:split], y[split:]

    # Train
    model = build_model()
    model.summary()
    model.fit(X_train, y_train, epochs=args.epochs, batch_size=32,
              validation_split=0.15, verbose=1)

    # Evaluate
    loss, acc = model.evaluate(X_test, y_test, verbose=0)
    print(f"\nTest accuracy: {acc:.4f}")

    # Export
    tflite_path = str(out_dir / args.output)
    tflite_bytes = export_tflite(model, tflite_path)
    export_c_header(tflite_bytes, str(out_dir / "sensor_model.h"))

    # Save normalisation parameters
    np.savez(str(out_dir / "normalisation.npz"), mean=mean, std=std)
    print(f"\nNormalisation params saved. Update ml_engine.cpp _mean/_std arrays with:")
    print(f"  mean = {{{', '.join(f'{v:.2f}f' for v in mean)}}};")
    print(f"  std  = {{{', '.join(f'{v:.2f}f' for v in std)}}};")


if __name__ == "__main__":
    main()
