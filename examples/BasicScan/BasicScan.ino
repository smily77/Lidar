/*
 * BasicScan.ino - Basic RPLidar scanning example
 *
 * This example demonstrates basic scanning functionality with RPLidar.
 * It continuously reads measurements and prints them to the Serial Monitor.
 *
 * Hardware connections (ESP32 example):
 * - RPLidar TX -> ESP32 RX (e.g., GPIO 16)
 * - RPLidar RX -> ESP32 TX (e.g., GPIO 17)
 * - RPLidar 5V -> ESP32 5V
 * - RPLidar GND -> ESP32 GND
 * - RPLidar MOTOR_PWM -> ESP32 PWM pin (optional, for motor control)
 *
 * Author: RPLidar Arduino Library
 */

#include <RPLidar.h>

// Create RPLidar object
RPLidar lidar;

// Serial port for RPLidar (ESP32 example)
// For other boards, use the appropriate Serial port
#define RPLIDAR_SERIAL Serial2

// RX/TX pins for ESP32
#define RPLIDAR_RX 16
#define RPLIDAR_TX 17

// Baud rate - change according to your model:
// A1M: 115200
// C1/C3: 460800
// S2/S2L/S3: 1000000
#define RPLIDAR_BAUD 115200

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for Serial port to connect
    }

    Serial.println("RPLidar Basic Scan Example");
    Serial.println("============================");

    // Initialize RPLidar serial port
#ifdef ESP32
    // For ESP32, specify RX/TX pins
    Serial2.begin(RPLIDAR_BAUD, SERIAL_8N1, RPLIDAR_RX, RPLIDAR_TX);
#else
    // For other boards, use default pins
    RPLIDAR_SERIAL.begin(RPLIDAR_BAUD);
#endif

    // Initialize RPLidar
    if (!lidar.begin(RPLIDAR_SERIAL, RPLIDAR_BAUD)) {
        Serial.println("Failed to initialize RPLidar!");
        while (1);
    }

    Serial.println("RPLidar initialized successfully!");

    // Get device information
    RPLidarDeviceInfo info;
    if (lidar.getDeviceInfo(info)) {
        Serial.println("\nDevice Information:");
        Serial.print("Model: ");
        Serial.println(info.model);
        Serial.print("Firmware Version: ");
        Serial.println(info.firmware_version);
        Serial.print("Hardware Version: ");
        Serial.println(info.hardware_version);
        Serial.print("Serial Number: ");
        for (int i = 0; i < 16; i++) {
            Serial.print(info.serialNumber[i], HEX);
        }
        Serial.println();
    }

    // Check device health
    RPLidarHealth health;
    if (lidar.getHealth(health)) {
        Serial.println("\nDevice Health:");
        Serial.print("Status: ");
        switch (health.status) {
            case RPLIDAR_STATUS_OK:
                Serial.println("OK");
                break;
            case RPLIDAR_STATUS_WARNING:
                Serial.println("WARNING");
                break;
            case RPLIDAR_STATUS_ERROR:
                Serial.println("ERROR");
                break;
            default:
                Serial.println("Unknown");
        }
        if (health.error_code != 0) {
            Serial.print("Error Code: ");
            Serial.println(health.error_code);
        }
    }

    // Start scanning
    Serial.println("\nStarting scan...");
    if (lidar.startScan()) {
        Serial.println("Scan started successfully!");
        Serial.println("\nFormat: Angle | Distance | Quality | NewScan");
        Serial.println("-----------------------------------------------");
    } else {
        Serial.println("Failed to start scan!");
        while (1);
    }

    delay(1000);
}

void loop() {
    // Read measurement
    RPLidarMeasurement measurement;

    if (lidar.readMeasurement(measurement)) {
        // Print measurement
        if (measurement.startBit) {
            Serial.println(); // New scan marker
        }

        Serial.print(measurement.angle, 2);
        Serial.print(" | ");
        Serial.print(measurement.distance, 2);
        Serial.print(" | ");
        Serial.print(measurement.quality);
        Serial.print(" | ");
        Serial.println(measurement.startBit ? "START" : "");

        // Optional: Filter out low quality measurements
        // if (measurement.quality < 10) {
        //     return; // Skip low quality points
        // }

        // Optional: Filter by angle range
        // if (measurement.angle < 45 || measurement.angle > 135) {
        //     return; // Only show front quadrant
        // }
    }
}
