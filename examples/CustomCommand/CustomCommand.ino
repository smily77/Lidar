/*
 * CustomCommand.ino - RPLidar Custom Command example
 *
 * This example demonstrates how to send custom commands to RPLidar
 * and read custom responses. This is useful for:
 * - Accessing advanced features
 * - Testing new commands
 * - Implementing features not yet in the library
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

// Baud rate
#define RPLIDAR_BAUD 115200

// Custom command examples
#define RPLIDAR_CMD_GET_ACC_BOARD_FLAG  0xFF

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for Serial port to connect
    }

    Serial.println("RPLidar Custom Command Example");
    Serial.println("===============================");
    Serial.println();

    // Initialize RPLidar serial port
#ifdef ESP32
    Serial2.begin(RPLIDAR_BAUD, SERIAL_8N1, RPLIDAR_RX, RPLIDAR_TX);
#else
    RPLIDAR_SERIAL.begin(RPLIDAR_BAUD);
#endif

    // Initialize RPLidar
    Serial.print("Initializing RPLidar...");
    if (!lidar.begin(RPLIDAR_SERIAL, RPLIDAR_BAUD)) {
        Serial.println(" FAILED!");
        while (1);
    }
    Serial.println(" OK!");
    Serial.println();

    // Example 1: Standard command using custom function
    Serial.println("Example 1: Sending standard GET_INFO command via custom function");
    sendGetInfoCustom();
    Serial.println();

    // Example 2: Send command with payload
    Serial.println("Example 2: Sending command with payload (Express Scan)");
    sendExpressScanCustom();
    Serial.println();

    // Example 3: Reading raw response
    Serial.println("Example 3: Reading raw measurement data");
    demonstrateRawReading();
    Serial.println();

    Serial.println("Custom command examples completed!");
}

void loop() {
    // Nothing in loop for this example
    delay(1000);
}

// Example 1: Send GET_INFO command manually
void sendGetInfoCustom() {
    Serial.println("Sending GET_INFO command (0x50)...");

    // Send command
    if (!lidar.sendCommand(RPLIDAR_CMD_GET_INFO)) {
        Serial.println("Failed to send command!");
        return;
    }

    // Read response
    uint8_t buffer[32];
    if (lidar.readResponse(buffer, sizeof(buffer), 1000)) {
        Serial.println("Response received:");

        // Parse device info manually
        uint8_t model = buffer[0];
        uint16_t firmware = buffer[2] | (buffer[1] << 8);
        uint8_t hardware = buffer[3];

        Serial.print("Model: ");
        Serial.println(model);
        Serial.print("Firmware: ");
        Serial.println(firmware);
        Serial.print("Hardware: ");
        Serial.println(hardware);
        Serial.print("Serial: ");
        for (int i = 4; i < 20; i++) {
            if (buffer[i] < 0x10) Serial.print("0");
            Serial.print(buffer[i], HEX);
        }
        Serial.println();
    } else {
        Serial.println("Failed to read response!");
    }
}

// Example 2: Send Express Scan command with payload
void sendExpressScanCustom() {
    Serial.println("Sending EXPRESS_SCAN command (0x82) with payload...");

    // Prepare payload
    uint8_t payload[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    // payload[0] = scan mode (0 = standard express)

    // Send command with payload
    if (lidar.sendCommand(RPLIDAR_CMD_EXPRESS_SCAN, payload, 5)) {
        Serial.println("Express scan command sent successfully!");
        Serial.println("Note: Response will be continuous measurement data");

        // Read a few measurement points
        delay(1000); // Wait for scan to start

        Serial.println("Reading first 10 measurements:");
        for (int i = 0; i < 10; i++) {
            RPLidarMeasurement measurement;
            if (lidar.readMeasurement(measurement)) {
                Serial.print(i + 1);
                Serial.print(": Angle=");
                Serial.print(measurement.angle, 2);
                Serial.print(" Distance=");
                Serial.print(measurement.distance, 2);
                Serial.print(" Quality=");
                Serial.println(measurement.quality);
            }
        }

        // Stop scan
        lidar.stop();
    } else {
        Serial.println("Failed to send command!");
    }
}

// Example 3: Demonstrate raw data reading
void demonstrateRawReading() {
    Serial.println("Starting scan and reading raw data...");

    // Start scan
    if (!lidar.startScan()) {
        Serial.println("Failed to start scan!");
        return;
    }

    delay(500); // Wait for scan to stabilize

    Serial.println("Reading 5 raw measurement packets (5 bytes each):");

    for (int i = 0; i < 5; i++) {
        uint8_t rawBuffer[5];

        if (lidar.readRawMeasurement(rawBuffer, 5, 1000)) {
            Serial.print("Packet ");
            Serial.print(i + 1);
            Serial.print(": ");

            // Print raw bytes in hex
            for (int j = 0; j < 5; j++) {
                if (rawBuffer[j] < 0x10) Serial.print("0");
                Serial.print(rawBuffer[j], HEX);
                Serial.print(" ");
            }

            // Parse and print values
            uint8_t quality = rawBuffer[0] & 0x3F;
            bool startBit = (rawBuffer[0] & 0x01) != 0;
            uint16_t angle_q6 = ((rawBuffer[2] >> 1) | (rawBuffer[1] << 7)) & 0x7FFF;
            uint16_t distance_q2 = rawBuffer[3] | (rawBuffer[4] << 8);

            Serial.print("| Q:");
            Serial.print(quality);
            Serial.print(" A:");
            Serial.print(angle_q6 / 64.0, 2);
            Serial.print(" D:");
            Serial.print(distance_q2 / 4.0, 2);
            if (startBit) Serial.print(" [START]");
            Serial.println();
        } else {
            Serial.println("Failed to read raw data!");
            break;
        }
    }

    // Stop scan
    lidar.stop();
}

/*
 * Additional custom command template:
 *
 * void sendCustomCommand() {
 *     // Define your custom command
 *     const uint8_t CUSTOM_CMD = 0xXX;
 *
 *     // Option 1: Command without payload
 *     if (lidar.sendCommand(CUSTOM_CMD)) {
 *         uint8_t buffer[64];
 *         if (lidar.readResponse(buffer, sizeof(buffer), 1000)) {
 *             // Process response
 *         }
 *     }
 *
 *     // Option 2: Command with payload
 *     uint8_t payload[N] = { ... };
 *     if (lidar.sendCommand(CUSTOM_CMD, payload, N)) {
 *         uint8_t buffer[64];
 *         if (lidar.readResponse(buffer, sizeof(buffer), 1000)) {
 *             // Process response
 *         }
 *     }
 * }
 */
