# UAV-Guardian-Edge-AI-Anomaly-Detection-via-Deep-Autoencoders
UAV-Guardian is a real-time predictive maintenance system designed for Unmanned Aerial Vehicles (UAVs). It utilizes an Edge-deployed Deep Autoencoder on an ESP32 to detect mechanical anomalies (such as propeller damage, motor failure, or loose components) by analyzing high-frequency vibrational data from an MPU6050 IMU.
## 🧠 The AI Engine: Unsupervised Anomaly Detection
Unlike traditional classifiers that need examples of "broken" states to learn, this project uses an Unsupervised Autoencoder. This is critical for drone safety because there are infinite ways a drone can fail, and we cannot possibly train for all of them.
### How the Autoencoder Works
The model is a neural network trained to perform a "lossy compression" of Healthy flight data.

1) Input (300 features): 1 second of X, Y, and Z accelerometer data sampled at 100Hz.

2) Encoder (300 → 32 → 8): The network compresses the 300 data points into a tiny "Latent Space" of just 8 numbers. This forces the AI to learn the fundamental "rhythm" of a healthy motor.

3) Decoder (8 → 32 → 300): The network attempts to reconstruct the original 300 data points from that compressed representation.

### The Mathematical Threshold
Because the AI was only trained on healthy data, it becomes an expert at reconstructing normal vibrations. When an anomaly occurs (e.g., a tape-weighted propeller or a loose screw), the new vibration pattern is "unknown" to the AI.

Reconstruction Error (MSE): We calculate the Mean Squared Error (MSE) between the Original Input ($x$) and the AI's Reconstruction:

'''($\hat{x}$):$$MSE = \frac{1}{n} \sum_{i=1}^{n} (x_i - \hat{x}_i)^2$$'''
