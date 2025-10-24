/*
 * ExpressScan.ino - RPLidar Express Scan Mode example
 *
 * This example demonstrates the Express Scan mode which provides:
 * - Higher sampling rate than standard scan
 * - More stable data output
 * - Better performance at higher rotation speeds
 *
 * Express scan is available on A2, A3, S series models.
 * Not all models support express scan - the example will fall back to standard scan.
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

// Baud rate - adjust for your model
// A2/A3: 115200 or 256000
// S2/S2L/S3: 1000000
#define RPLIDAR_BAUD 115200

// Scan statistics
unsigned long scanCount = 0;
unsigned long pointCount = 0;
unsigned long lastScanTime = 0;
float lastScanDuration = 0;

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for Serial port to connect
    }

    Serial.println("RPLidar Express Scan Example");
    Serial.println("=============================");
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

    // Get device info
    RPLidarDeviceInfo info;
    if (lidar.getDeviceInfo(info)) {
        Serial.println("\nDevice Information:");
        Serial.print("Model: ");
        Serial.println(info.model);
        Serial.print("Firmware: ");
        Serial.println(info.firmware_version);

        // Check if device supports express scan
        if (info.model == 1) {
            Serial.println("\nNOTE: A1 series does not support express scan.");
            Serial.println("Will use standard scan mode instead.");
        }
    }

    // Get sample rates
    uint16_t standard_rate, express_rate;
    if (lidar.getSampleRate(standard_rate, express_rate)) {
        Serial.println("\nSample Rates:");
        Serial.print("Standard: ");
        Serial.print(standard_rate);
        Serial.println(" Hz");
        Serial.print("Express: ");
        Serial.print(express_rate);
        Serial.println(" Hz");

        float improvement = ((float)express_rate / standard_rate - 1.0) * 100.0;
        Serial.print("Express mode improvement: +");
        Serial.print(improvement, 1);
        Serial.println("%");
    }

    // Start express scan
    Serial.println("\nStarting express scan...");
    if (lidar.startExpressScan(0)) {
        Serial.println("Express scan started successfully!");
    } else {
        Serial.println("Express scan failed, trying standard scan...");
        if (lidar.startScan()) {
            Serial.println("Standard scan started as fallback.");
        } else {
            Serial.println("Failed to start any scan mode!");
            while (1);
        }
    }

    Serial.println();
    Serial.println("Collecting data...");
    Serial.println();

    lastScanTime = millis();
}

void loop() {
    RPLidarMeasurement measurement;

    if (lidar.readMeasurement(measurement)) {
        pointCount++;

        // Detect start of new scan
        if (measurement.startBit) {
            unsigned long currentTime = millis();
            lastScanDuration = (currentTime - lastScanTime) / 1000.0;
            lastScanTime = currentTime;
            scanCount++;

            // Print scan statistics
            printScanStatistics();
        }

        // Optional: Print individual measurements
        // Uncomment to see all data points
        /*
        Serial.print("Angle: ");
        Serial.print(measurement.angle, 2);
        Serial.print(" | Distance: ");
        Serial.print(measurement.distance, 2);
        Serial.print(" | Quality: ");
        Serial.println(measurement.quality);
        */

        // Example: Detect objects in specific sectors
        detectObjects(measurement);
    }

    // Print statistics every 5 seconds
    static unsigned long lastStatsTime = 0;
    if (millis() - lastStatsTime >= 5000) {
        lastStatsTime = millis();
        printOverallStatistics();
    }
}

void printScanStatistics() {
    if (scanCount % 10 == 0) { // Print every 10 scans
        Serial.println("--- Scan Statistics ---");
        Serial.print("Scan #: ");
        Serial.println(scanCount);

        Serial.print("Scan duration: ");
        Serial.print(lastScanDuration, 3);
        Serial.println(" seconds");

        float rpm = 60.0 / lastScanDuration;
        Serial.print("Rotation speed: ");
        Serial.print(rpm, 1);
        Serial.println(" RPM");

        Serial.print("Points per scan: ");
        Serial.println(pointCount / scanCount);

        Serial.println();
    }
}

void printOverallStatistics() {
    Serial.println("\n=== Overall Statistics ===");
    Serial.print("Total scans: ");
    Serial.println(scanCount);

    Serial.print("Total points: ");
    Serial.println(pointCount);

    if (scanCount > 0) {
        Serial.print("Average points/scan: ");
        Serial.println(pointCount / scanCount);

        unsigned long elapsedSeconds = millis() / 1000;
        if (elapsedSeconds > 0) {
            Serial.print("Average sample rate: ");
            Serial.print(pointCount / elapsedSeconds);
            Serial.println(" Hz");
        }
    }

    Serial.println("========================\n");
}

void detectObjects(const RPLidarMeasurement& measurement) {
    // Example: Detect objects in different sectors

    // Ignore low quality and invalid measurements
    if (measurement.quality < 10 || measurement.distance < 100) {
        return;
    }

    // Define sectors (in degrees)
    const float FRONT_START = 345.0;
    const float FRONT_END = 15.0;
    const float LEFT_START = 75.0;
    const float LEFT_END = 105.0;
    const float RIGHT_START = 255.0;
    const float RIGHT_END = 285.0;

    // Detection threshold (in mm)
    const float THRESHOLD = 1000.0; // 1 meter

    // Check front sector
    if (measurement.distance < THRESHOLD) {
        if (measurement.angle >= FRONT_START || measurement.angle <= FRONT_END) {
            // Object detected in front
            // Add your action here
        }
        if (measurement.angle >= LEFT_START && measurement.angle <= LEFT_END) {
            // Object detected on left
            // Add your action here
        }
        if (measurement.angle >= RIGHT_START && measurement.angle <= RIGHT_END) {
            // Object detected on right
            // Add your action here
        }
    }
}
