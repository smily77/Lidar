/*
 * CustomCommand.ino - sending custom commands and reading raw responses.
 *
 * Demonstrates:
 *  1. sendCommand() + readResponse() for a request/response command (GET_INFO)
 *  2. reading and decoding raw 5-byte measurement nodes during a scan
 */

#include <RPLidar.h>
#include <RPLidarParser.h>   // rplidarParseStandardNode() for raw decoding

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

    Serial.println("CustomCommand example");
    Serial.println("=====================");

    // 1) GET_INFO via the generic command/response API.
    Serial.println("\n[1] GET_INFO via sendCommand()/readResponse()");
    if (lidar.sendCommand(RPLIDAR_CMD_GET_INFO)) {
        uint8_t buf[20];
        if (lidar.readResponse(buf, sizeof(buf), 1000)) {
            Serial.print("  Model: ");    Serial.println(buf[0]);
            Serial.print("  Firmware: "); Serial.println(buf[2] | (buf[1] << 8));
            Serial.print("  Hardware: "); Serial.println(buf[3]);
        } else {
            Serial.println("  No response (check baud/wiring).");
        }
    }

    // 2) Raw measurement reading + correct decode.
    Serial.println("\n[2] Raw measurement nodes during scan");
    if (lidar.startScan()) {
        for (int i = 0; i < 8; i++) {
            uint8_t raw[5];
            if (!lidar.readRawMeasurement(raw, 5, 1000)) { Serial.println("  read failed"); break; }
            RPLidarMeasurement m;
            if (rplidarParseStandardNode(raw, m)) {
                Serial.print("  A="); Serial.print(m.angle, 2);
                Serial.print(" D="); Serial.print(m.distance, 1);
                Serial.print(" Q="); Serial.print(m.quality);
                Serial.println(m.startBit ? " [START]" : "");
            } else {
                Serial.println("  (unaligned node - resync needed; use readMeasurement)");
            }
        }
        lidar.stop();
    }
    Serial.println("\nDone.");
}

void loop() {
    delay(1000);
}
