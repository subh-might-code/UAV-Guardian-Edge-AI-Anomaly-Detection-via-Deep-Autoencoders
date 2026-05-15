# UAV-Guardian: Edge-AI Anomaly Detection via Deep Autoencoders

UAV-Guardian is a real-time predictive maintenance and safety system for Unmanned Aerial Vehicles (UAVs). It utilizes an **Edge-deployed Deep Autoencoder** running on an ESP32 to detect mechanical anomalies—such as propeller damage, motor imbalances, or loose components—by analyzing high-frequency vibrational data from an MPU6050 IMU.

## The AI Engine: Unsupervised Anomaly Detection

Unlike traditional classification models that require labeled datasets of every possible failure mode (Supervised Learning), this project employs an **Unsupervised Autoencoder**. This is critical for drone safety because failure modes are unpredictable and infinite; a model should know what "normal" looks like and flag anything else as a deviation.

### How the Autoencoder Works
The model is a neural network trained to perform "lossy compression" of **Healthy** flight data.

1.  **Input (300 features):** 1 second of X, Y, and Z accelerometer data sampled at 100Hz.
2.  **Encoder (300 → 32 → 8):** The network compresses the 300 data points into a tiny **Latent Space** (bottleneck) of just 8 numbers. This forces the AI to learn the fundamental "rhythm" and harmonics of a healthy motor.
3.  **Decoder (8 → 32 → 300):** The network attempts to reconstruct the original 300 data points from that compressed 8-number representation.

### Mathematical Framework: Reconstruction Error
Anomaly detection is achieved by measuring the **Mean Squared Error (MSE)** between the original sensor input ($x$) and the AI's reconstruction ($\hat{x}$):

$$MSE = \frac{1}{n} \sum_{i=1}^{n} (x_i - \hat{x}_i)^2$$

* **Training Strategy:** The model was trained in Google Colab using only healthy vibration data.
* **Thresholding:** We calculated the maximum MSE the model produced on healthy data and added a safety buffer to establish the alarm threshold.
    * **Normal Flight:** $MSE < 2.6$ (The AI successfully reconstructs the signal).
    * **Anomaly Detected:** $MSE > 2.5$ (The AI fails to reconstruct the unknown pattern).

---

## Edge Deployment & Memory Engineering

Deploying Deep Learning on a microcontroller like the ESP32 requires solving significant hardware constraints.

### TensorFlow Lite for Microcontrollers (TFLite)
The model was trained in Keras, converted to a `.tflite` flatbuffer, and embedded into the firmware as a C++ byte array (`uav_model_autoenc.h`).

### Resolving the "Identity Shortcut" Bug
During deployment, a critical memory conflict was identified: The TFLite interpreter optimizes RAM by sharing the memory address for the **Input** and **Output** tensors. In an Autoencoder, this causes the AI to overwrite the input with the output, resulting in a false $MSE = 0.0000$.

**The Solution:** I implemented a manual **Input Backup Buffer**. Raw sensor data is cloned into a private RAM buffer before the AI inference begins. The MSE calculation then compares this preserved original data against the AI's output, enabling accurate anomaly detection.

---

## Networking & Communication (MQTT)

The system uses the **MQTT** protocol for low-latency, asynchronous telemetry.

* **Broker:** Eclipse Mosquitto.
* **Pipeline:** 1. ESP32 samples IMU data and runs local inference.
    2. The resulting MSE score and health status are packaged into a string.
    3. Data is published to the topic `uav/guardian/health`.
* **Monitoring:** Any subscriber (laptop, dashboard, or mobile app) can monitor the drone's health in real-time.
* terminal commands:
1) publisher(move to mosquitto directory):
```bash
  mosquitto -v -c mosquitto.conf
```
2) subscriber(move to mosquitto directory):
```bash
  mosquitto_sub -h localhost -t "uav/guardian/health"
```
  

---

## 🛠️ Tech Stack
* **Firmware:** C++ / Arduino / ESP32
* **AI/ML:** TensorFlow, Keras, TFLite Micro, Google Colab
* **Hardware:** MPU6050 (I2C)
* **Communication:** MQTT (PubSubClient), Wi-Fi

---

## Project Structure
```text
├── src/
│   ├── uav_g_autoenc.ino    # Main ESP32 source code
│   └── uav_model_autoenc.h         # Embedded TFLite model
├── training/
│   ├── healthy_autoenc.csv  # Training dataset
│   └── UAV-g-autoenc.ipynb   # Colab notebook
└── README.md               # Project documentation
