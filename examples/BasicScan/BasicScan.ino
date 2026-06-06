/*
 * BasicScan.ino - Basic RPLidar standard-scan example
 *
 * Continuously reads measurements and prints them to the Serial Monitor.
 *
 * Wiring (RPLidar <-> MCU): Lidar TX -> MCU RX, Lidar RX -> MCU TX, plus 5V/GND.
 *
 * Set LIDAR_BAUD to match your model: A1M 115200, A3 256000, C1/C3 460800,
 * S-series 1000000.
 */

#include <RPLidar.h>

RPLidar lidar;

#define LIDAR_BAUD 460800   // RPLIDAR C1 default

// ---- Portable hardware-serial selection ------------------------------------
#if defined(ESP32)
  #define LIDAR_SERIAL  Serial1
  #define LIDAR_RX      7
  #define LIDAR_TX      8
  #define LIDAR_UART_BEGIN()  Serial1.begin(LIDAR_BAUD, SERIAL_8N1, LIDAR_RX, LIDAR_TX)
#elif defined(HAVE_HWSERIAL1) || defined(__AVR_ATmega2560__) || defined(ARDUINO_AVR_MEGA2560)
  #define LIDAR_SERIAL  Serial1
  #define LIDAR_UART_BEGIN()  Serial1.begin(LIDAR_BAUD)
#else
  #warning "No dedicated second UART for this board; using Serial (shared with USB). Edit LIDAR_SERIAL for your board."
  #define LIDAR_SERIAL  Serial
  #define LIDAR_UART_BEGIN()  Serial.begin(LIDAR_BAUD)
#endif

void setup() {
    Serial.begin(115200);

    LIDAR_UART_BEGIN();
    delay(1000);                 // give the lidar UART time to come up
    lidar.begin(LIDAR_SERIAL);   // bind the already-configured stream

    RPLidarHealth health;
    if (lidar.getHealth(health)) {
        Serial.print("Health status: ");
        Serial.println(health.status == RPLIDAR_STATUS_OK ? "OK" :
                       health.status == RPLIDAR_STATUS_WARNING ? "WARNING" : "ERROR");
    } else {
        Serial.println("Could not read health (check baud rate / wiring).");
    }

    if (!lidar.startScan()) {
        Serial.println("Failed to start scan!");
        while (1) { delay(1000); }
    }
    Serial.println("Angle | Distance | Quality | NewScan");
}

void loop() {
    RPLidarMeasurement m;
    if (lidar.readMeasurement(m)) {
        if (m.startBit) Serial.println("--- new scan ---");
        Serial.print(m.angle, 2);
        Serial.print(" | ");
        Serial.print(m.distance, 2);
        Serial.print(" | ");
        Serial.print(m.quality);
        Serial.print(" | ");
        Serial.println(m.startBit ? "START" : "");
    }
}
