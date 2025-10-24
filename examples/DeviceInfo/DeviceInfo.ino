/*
 * DeviceInfo.ino - RPLidar device information example
 *
 * This example demonstrates how to retrieve and display device information
 * from RPLidar, including model, firmware version, health status, and sample rate.
 *
 * Hardware connections (ESP32 example):
 * - RPLidar TX -> ESP32 RX (e.g., GPIO 16)
 * - RPLidar RX -> ESP32 TX (e.g., GPIO 17)
 * - RPLidar 5V -> ESP32 5V
 * - RPLidar GND -> ESP32 GND
 *
 * Author: RPLidar Arduino Library
 */

#include <RPLidar.h>

// Create RPLidar object
RPLidar lidar;

// Serial port for RPLidar
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

    Serial.println("RPLidar Device Information Example");
    Serial.println("===================================");
    Serial.println();

    // Initialize RPLidar serial port
#ifdef ESP32
    // For ESP32, specify RX/TX pins
    Serial2.begin(RPLIDAR_BAUD, SERIAL_8N1, RPLIDAR_RX, RPLIDAR_TX);
#else
    // For other boards, use default pins
    RPLIDAR_SERIAL.begin(RPLIDAR_BAUD);
#endif

    // Initialize RPLidar
    Serial.print("Initializing RPLidar...");
    if (!lidar.begin(RPLIDAR_SERIAL, RPLIDAR_BAUD)) {
        Serial.println(" FAILED!");
        while (1);
    }
    Serial.println(" OK!");

    // Get and display device information
    displayDeviceInfo();

    // Get and display health status
    displayHealthStatus();

    // Get and display sample rate
    displaySampleRate();

    // Check connection
    Serial.println();
    Serial.print("Connection test: ");
    if (lidar.isConnected()) {
        Serial.println("CONNECTED");
    } else {
        Serial.println("NOT CONNECTED");
    }

    Serial.println();
    Serial.println("Device information complete!");
}

void loop() {
    // Check health status every 5 seconds
    static unsigned long lastCheck = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastCheck >= 5000) {
        lastCheck = currentMillis;

        Serial.println("\n--- Health Check ---");
        displayHealthStatus();
    }

    delay(100);
}

void displayDeviceInfo() {
    Serial.println("\n--- Device Information ---");

    RPLidarDeviceInfo info;
    if (lidar.getDeviceInfo(info)) {
        Serial.print("Model: ");
        Serial.println(info.model);

        Serial.print("Firmware Version: ");
        Serial.print(info.firmware_version >> 8);
        Serial.print(".");
        Serial.println(info.firmware_version & 0xFF);

        Serial.print("Hardware Version: ");
        Serial.println(info.hardware_version);

        Serial.print("Serial Number: ");
        for (int i = 0; i < 16; i++) {
            if (info.serialNumber[i] < 0x10) Serial.print("0");
            Serial.print(info.serialNumber[i], HEX);
        }
        Serial.println();

        // Suggest appropriate baud rate based on model
        Serial.print("Suggested Baud Rate: ");
        if (info.model <= 10) {
            Serial.println("115200 (A series)");
        } else if (info.model <= 30) {
            Serial.println("460800 (C series)");
        } else {
            Serial.println("1000000 (S series)");
        }
    } else {
        Serial.println("Failed to get device information!");
    }
}

void displayHealthStatus() {
    Serial.println("--- Health Status ---");

    RPLidarHealth health;
    if (lidar.getHealth(health)) {
        Serial.print("Status: ");
        switch (health.status) {
            case RPLIDAR_STATUS_OK:
                Serial.println("OK (Healthy)");
                break;
            case RPLIDAR_STATUS_WARNING:
                Serial.println("WARNING");
                Serial.print("Warning Code: ");
                Serial.println(health.error_code);
                break;
            case RPLIDAR_STATUS_ERROR:
                Serial.println("ERROR");
                Serial.print("Error Code: ");
                Serial.println(health.error_code);
                Serial.println("Note: Device may need to be reset");
                break;
            default:
                Serial.print("Unknown (");
                Serial.print(health.status);
                Serial.println(")");
        }

        if (health.error_code != 0) {
            Serial.print("Error/Warning Code: 0x");
            Serial.println(health.error_code, HEX);
        }
    } else {
        Serial.println("Failed to get health status!");
    }
}

void displaySampleRate() {
    Serial.println("\n--- Sample Rate Information ---");

    uint16_t standard_rate, express_rate;
    if (lidar.getSampleRate(standard_rate, express_rate)) {
        Serial.print("Standard Scan Rate: ");
        Serial.print(standard_rate);
        Serial.println(" samples/second");

        Serial.print("Express Scan Rate: ");
        Serial.print(express_rate);
        Serial.println(" samples/second");

        // Calculate rotation speed
        float standard_rpm = (standard_rate / 360.0) * 60.0;
        float express_rpm = (express_rate / 360.0) * 60.0;

        Serial.print("Standard Mode Speed: ~");
        Serial.print(standard_rpm, 1);
        Serial.println(" RPM");

        Serial.print("Express Mode Speed: ~");
        Serial.print(express_rpm, 1);
        Serial.println(" RPM");
    } else {
        Serial.println("Failed to get sample rate!");
    }
}
