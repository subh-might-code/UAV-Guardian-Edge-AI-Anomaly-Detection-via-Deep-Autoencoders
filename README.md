# UAV-Guardian-Edge-AI-Anomaly-Detection-via-Deep-Autoencoders
UAV-Guardian is a real-time predictive maintenance system designed for Unmanned Aerial Vehicles (UAVs). It utilizes an Edge-deployed Deep Autoencoder on an ESP32 to detect mechanical anomalies (such as propeller damage, motor failure, or loose components) by analyzing high-frequency vibrational data from an MPU6050 IMU.
## 🧠 The AI Engine: Unsupervised Anomaly Detection
Unlike traditional classifiers that need examples of "broken" states to learn, this project uses an Unsupervised Autoencoder. This is critical for drone safety because there are infinite ways a drone can fail, and we cannot possibly train for all of them.
### How the Autoencoder Works
The model is a neural network trained to perform a "lossy compression" of Healthy flight data.
Input (300 features): 1 second of X, Y, and Z accelerometer data sampled at 100Hz.

Encoder (300 → 32 → 8): The network compresses the 300 data points into a tiny "Latent Space" of just 8 numbers. This forces the AI to learn the fundamental "rhythm" of a healthy motor.

Decoder (8 → 32 → 300): The network attempts to reconstruct the original 300 data points from that compressed representation.
