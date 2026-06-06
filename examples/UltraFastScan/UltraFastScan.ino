/*
 * UltraFastScan.ino - Optimized high-speed scanning for RPLidar S2L
 *
 * This example demonstrates the optimized readFast() function for minimal overhead.
 * Perfect for high data rate models like S2L that can produce up to 32000 samples/sec.
 *
 * The example shows how to:
 * 1. Read data with minimal overhead (only distance and angle)
 * 2. Prepare data for UDP transmission (not included in library)
 * 3. Achieve maximum throughput on ESP32
 *
 * Note: This example uses the readFast() function which skips quality checks
 * and start bit detection for maximum speed. Use BasicScan if you need full data.
 *
 * Hardware connections (ESP32):
 * - RPLidar TX -> ESP32 RX (e.g., GPIO 16)
 * - RPLidar RX -> ESP32 TX (e.g., GPIO 17)
 * - RPLidar 5V -> ESP32 5V (use external 5V supply for S2L!)
 * - RPLidar GND -> ESP32 GND
 * - RPLidar MOTOR_PWM -> ESP32 PWM pin (for motor speed control)
 *
 * Author: RPLidar Arduino Library
 */

#include <RPLidar.h>

// WiFi and UDP includes (for UDP transmission example)
#ifdef ESP32
#include <WiFi.h>
#include <WiFiUdp.h>
#endif

// Create RPLidar object
RPLidar lidar;

// S2L uses 1000000 baud (this example targets high-rate models)
#define RPLIDAR_BAUD 1000000

// ---- Portable hardware-serial selection (high speed needs a hardware UART) --
#if defined(ESP32)
  #define RPLIDAR_SERIAL  Serial1
  #define LIDAR_UART_BEGIN()  do { Serial1.setRxBufferSize(2048); Serial1.begin(RPLIDAR_BAUD, SERIAL_8N1, 7, 8); } while (0)
#elif defined(HAVE_HWSERIAL1) || defined(__AVR_ATmega2560__) || defined(ARDUINO_AVR_MEGA2560)
  #define RPLIDAR_SERIAL  Serial1
  #define LIDAR_UART_BEGIN()  Serial1.begin(RPLIDAR_BAUD)
#else
  #warning "No dedicated second UART for this board; using Serial (shared with USB)."
  #define RPLIDAR_SERIAL  Serial
  #define LIDAR_UART_BEGIN()  Serial.begin(RPLIDAR_BAUD)
#endif

// Statistics
unsigned long measurementCount = 0;
unsigned long lastStatsTime = 0;
unsigned long startTime = 0;

// Optional: UDP configuration (uncomment to use)
// const char* ssid = "YOUR_WIFI_SSID";
// const char* password = "YOUR_WIFI_PASSWORD";
// const char* udpAddress = "192.168.1.100";
// const int udpPort = 12345;
// WiFiUDP udp;

// Buffer for batch UDP sending (reduces overhead)
struct ScanPoint {
    float angle;
    float distance;
};

#define BUFFER_SIZE 50
ScanPoint buffer[BUFFER_SIZE];
int bufferIndex = 0;

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for Serial port to connect
    }

    Serial.println("RPLidar Ultra Fast Scan Example (Optimized for S2L)");
    Serial.println("====================================================");
    Serial.println();

    // Optional: Initialize WiFi for UDP
    // setupWiFi();

    // Initialize the lidar UART at high speed, then bind it.
    LIDAR_UART_BEGIN();
    delay(1000);                 // give the lidar UART time to come up
    lidar.begin(RPLIDAR_SERIAL); // bind the already-configured stream
#if defined(ESP32)
    Serial.println("- CPU frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
    if (ESP.getCpuFreqMHz() < 240) {
        Serial.println("WARNING: set CPU frequency to 240 MHz for best performance");
    }
#endif
    Serial.println("Initialized.");

    // Quick health check
    RPLidarHealth health;
    if (lidar.getHealth(health)) {
        if (health.status != RPLIDAR_STATUS_OK) {
            Serial.println("WARNING: Device health status is not OK!");
        }
    }

    // Start scanning
    Serial.println("Starting ultra-fast scan...");
    if (lidar.startScan()) {
        Serial.println("Scan started successfully!");
        Serial.println();
        Serial.println("Performance Mode: ULTRA FAST");
        Serial.println("Data: Angle and Distance only (minimal overhead)");
        Serial.println("Quality checks: DISABLED for maximum speed");
        Serial.println();
    } else {
        Serial.println("Failed to start scan!");
        while (1);
    }

    startTime = millis();
    lastStatsTime = startTime;
}

void loop() {
    // Read data using optimized readFast() function
    float angle, distance;

    if (lidar.readFast(angle, distance)) {
        measurementCount++;

        // Store in buffer for batch processing/transmission
        buffer[bufferIndex].angle = angle;
        buffer[bufferIndex].distance = distance;
        bufferIndex++;

        // When buffer is full, process/send data
        if (bufferIndex >= BUFFER_SIZE) {
            processBatch();
            bufferIndex = 0;
        }

        // Optional: Uncomment for immediate processing (slower)
        // processPoint(angle, distance);
    }

    // Print statistics every second
    unsigned long currentTime = millis();
    if (currentTime - lastStatsTime >= 1000) {
        printStatistics(currentTime);
        lastStatsTime = currentTime;
    }
}

// Process a batch of measurements
void processBatch() {
    // Example 1: Send via UDP (uncomment to use)
    // sendBatchUDP();

    // Example 2: Filter and process data
    for (int i = 0; i < bufferIndex; i++) {
        // Filter out invalid distances (0 or too far)
        if (buffer[i].distance > 10.0 && buffer[i].distance < 40000.0) {
            // Process valid point
            // Your custom processing here...

            // Example: Detect obstacles in front (0-30 degrees or 330-360 degrees)
            if ((buffer[i].angle < 30.0 || buffer[i].angle > 330.0) && buffer[i].distance < 1000.0) {
                // Obstacle detected in front within 1 meter!
                // Add your obstacle avoidance logic here
            }
        }
    }
}

// Process a single measurement point
void processPoint(float angle, float distance) {
    // Example processing for immediate response
    // This is called for each point individually (higher overhead)

    // Your custom code here...
}

// Send batch via UDP (optional)
/*
void sendBatchUDP() {
    if (WiFi.status() == WL_CONNECTED) {
        udp.beginPacket(udpAddress, udpPort);

        // Send as binary data for efficiency
        udp.write((uint8_t*)buffer, bufferIndex * sizeof(ScanPoint));

        // Or send as JSON/text for readability:
        // for (int i = 0; i < bufferIndex; i++) {
        //     udp.printf("%.2f,%.2f\n", buffer[i].angle, buffer[i].distance);
        // }

        udp.endPacket();
    }
}
*/

// Setup WiFi connection (optional)
/*
void setupWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("UDP target: ");
    Serial.print(udpAddress);
    Serial.print(":");
    Serial.println(udpPort);
    Serial.println();
}
*/

// Print performance statistics
void printStatistics(unsigned long currentTime) {
    static unsigned long lastCount = 0;
    unsigned long pointsPerSecond = measurementCount - lastCount;
    lastCount = measurementCount;

    unsigned long elapsedSeconds = (currentTime - startTime) / 1000;
    float averageRate = elapsedSeconds > 0 ? (float)measurementCount / elapsedSeconds : 0;

    Serial.println("--- Performance Statistics ---");
    Serial.print("Points/sec: ");
    Serial.print(pointsPerSecond);
    Serial.print(" | Average: ");
    Serial.print(averageRate, 1);
    Serial.print(" | Total: ");
    Serial.print(measurementCount);

#ifdef ESP32
    Serial.print(" | Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" | CPU: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.print(" MHz");
#endif

    Serial.println();

    // Check if we're achieving expected performance
    if (pointsPerSecond < 5000 && elapsedSeconds > 5) {
        Serial.println("WARNING: Low data rate. Check:");
        Serial.println("- Serial baud rate matches your device");
        Serial.println("- Motor is spinning");
        Serial.println("- Device health status");
    }
}
