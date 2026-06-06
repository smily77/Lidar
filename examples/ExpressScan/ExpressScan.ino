/*
 * ExpressScan.ino - Express scan is NOT implemented in this library.
 *
 * The express-scan "capsuled"/"ultra-capsuled" payload decoder (start angle,
 * delta-encoded cabins, checksums, angle interpolation) is not implemented and
 * has not been verified against hardware. lidar.startExpressScan() therefore
 * returns false on purpose, so you never receive mis-decoded measurements.
 *
 * Use standard scan instead -- see BasicScan.ino. This sketch only documents
 * the current state and demonstrates the (expected) false return.
 */

#include <RPLidar.h>

RPLidar lidar;

#define LIDAR_BAUD 460800

#if defined(ESP32)
  #define LIDAR_SERIAL  Serial1
  #define LIDAR_UART_BEGIN()  Serial1.begin(LIDAR_BAUD, SERIAL_8N1, 7, 8)
#elif defined(HAVE_HWSERIAL1) || defined(__AVR_ATmega2560__) || defined(ARDUINO_AVR_MEGA2560)
  #define LIDAR_SERIAL  Serial1
  #define LIDAR_UART_BEGIN()  Serial1.begin(LIDAR_BAUD)
#else
  #warning "No dedicated second UART for this board; using Serial (shared with USB)."
  #define LIDAR_SERIAL  Serial
  #define LIDAR_UART_BEGIN()  Serial.begin(LIDAR_BAUD)
#endif

void setup() {
    Serial.begin(115200);
    LIDAR_UART_BEGIN();
    delay(1000);
    lidar.begin(LIDAR_SERIAL);

    Serial.println("Express scan is not implemented in this library.");
    Serial.print("lidar.startExpressScan(0) returns: ");
    Serial.println(lidar.startExpressScan(0) ? "true (unexpected)" : "false (expected)");
    Serial.println("Use standard scan (startScan) instead -- see BasicScan.ino.");
}

void loop() {
    delay(1000);
}
