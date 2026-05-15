#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include <MPU6050_tockn.h>
#include <Wire.h>
#include "uav_model_autoenc.h"
#include <WiFi.h>
#include <PubSubClient.h>

// --- 1. NETWORK CONFIG ---
const char* ssid = "BabaYaga";
const char* password = "123ootyk";
const char* mqtt_server = "10.106.15.33"; // <-- UPDATE THIS IF NEEDED

WiFiClient espClient;
PubSubClient client(espClient);

// --- 2. AI GLOBALS ---
const int kTensorArenaSize = 60 * 1024; 
uint8_t* tensor_arena = nullptr;        

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
MPU6050 mpu6050(Wire);

// --- YOUR CUSTOM ANOMALY THRESHOLD ---
const float ANOMALY_THRESHOLD = 2.5; 

// --- 3. BULLETPROOF WIFI ---
void setup_wifi() {
    delay(10);
    Serial.println("\nConnecting to WiFi...");
    
    WiFi.disconnect(true, true); 
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(ssid, password);

    int attempt_counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempt_counter++;
        if (attempt_counter > 30) {
            Serial.println("\nCannot find hotspot. Rebooting ESP32...");
            delay(1000);
            ESP.restart();
        }
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("UAV_Guardian_AEC")) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    tensor_arena = (uint8_t*)malloc(kTensorArenaSize);
    if (tensor_arena == nullptr) {
        Serial.println("CRITICAL: Could not allocate arena!");
        while(1);
    }

    setup_wifi();
    client.setServer(mqtt_server, 1883);

    Serial.println("--- UAV-GUARDIAN: AUTOENCODER ONLINE ---");
    Wire.begin(); 
    mpu6050.begin();
    mpu6050.calcGyroOffsets(true); 

    static tflite::MicroErrorReporter micro_error_reporter;
    model = tflite::GetModel(uav_model_tflite);
    static tflite::AllOpsResolver resolver;

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("Inference Allocation Failed!");
        while (1);
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    Serial.println("--- SYSTEM READY ---");
}

// --- ADD THIS TO YOUR GLOBALS (Top of the script) ---
float input_backup[300]; // Our private copy of the sensor data

void loop() {
    if (!client.connected()) { reconnect(); }
    client.loop();

    // 1. COLLECT 1 SECOND OF DATA
    for (int i = 0; i < 100; i++) {
        mpu6050.update();
        
        float ax = mpu6050.getAccX();
        float ay = mpu6050.getAccY();
        float az = mpu6050.getAccZ();

        if (input != nullptr) {
            // Fill the AI's input
            input->data.f[i * 3]     = ax;
            input->data.f[i * 3 + 1] = ay;
            input->data.f[i * 3 + 2] = az;

            // ALSO save it to our private backup
            input_backup[i * 3]     = ax;
            input_backup[i * 3 + 1] = ay;
            input_backup[i * 3 + 2] = az;
        }
        delay(10); 
    }

    // 2. RUN AUTOENCODER (This will overwrite the 'input' tensor memory)
    if (interpreter->Invoke() != kTfLiteOk) { return; }

    // 3. CALCULATE MSE (Compare backup vs AI output)
    float total_mse = 0;
    for (int i = 0; i < 300; i++) {
        float val_original = input_backup[i]; // Read from our backup
        float val_ai_draw  = output->data.f[i]; // Read from AI output
        
        float diff = val_original - val_ai_draw;
        total_mse += (diff * diff);
    }
    float mse = total_mse / 300.0;

    // 4. PUBLISH & SERIAL PRINT
    String msg = "MSE: " + String(mse, 4);
    
    // Diagnostic print to see the difference now
    Serial.print("Original Sample: "); Serial.print(input_backup[0], 4);
    Serial.print(" | AI Reconstructed: "); Serial.println(output->data.f[0], 4);
    
    if (mse > ANOMALY_THRESHOLD) {
        msg += " [ANOMALY!]";
        Serial.print("ALERT! MSE: ");
    } else {
        Serial.print("Normal. MSE: ");
    }
    Serial.println(mse, 4);

    client.publish("uav/guardian/health", msg.c_str());
    delay(200); 
}
