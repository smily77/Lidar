/*
 * RPLidar.h - Arduino library for Slamtec RPLidar laser scanner
 *
 * Tested: RPLIDAR C1 on ESP32-S3 (standard scan, 460800 baud).
 * Standard scan, device info/health and sample-rate queries are implemented
 * and protocol-correct. Express scan is NOT implemented (see startExpressScan).
 *
 * Features:
 * - Standard scan with byte-stream resynchronisation
 * - Device info / health / sample-rate queries with descriptor validation
 * - Custom command support
 * - No RTTI dependency (builds with -fno-rtti)
 *
 * License: MIT (see LICENSE)
 */

#ifndef RPLIDAR_H
#define RPLIDAR_H

#include <Arduino.h>
#include "RPLidarParser.h"   // RPLidarMeasurement + pure node decoder

// RPLidar Protocol Constants
#define RPLIDAR_CMD_SYNC_BYTE           0xA5
#define RPLIDAR_ANS_SYNC_BYTE1          0xA5
#define RPLIDAR_ANS_SYNC_BYTE2          0x5A

// RPLidar Commands
#define RPLIDAR_CMD_STOP                0x25
#define RPLIDAR_CMD_RESET               0x40
#define RPLIDAR_CMD_SCAN                0x20
#define RPLIDAR_CMD_EXPRESS_SCAN        0x82
#define RPLIDAR_CMD_FORCE_SCAN          0x21
#define RPLIDAR_CMD_GET_INFO            0x50
#define RPLIDAR_CMD_GET_HEALTH          0x52
#define RPLIDAR_CMD_GET_SAMPLERATE      0x59
#define RPLIDAR_CMD_GET_LIDAR_CONF      0x84

// Response Types
#define RPLIDAR_ANS_TYPE_DEVINFO        0x04
#define RPLIDAR_ANS_TYPE_DEVHEALTH      0x06
#define RPLIDAR_ANS_TYPE_MEASUREMENT    0x81
#define RPLIDAR_ANS_TYPE_SAMPLE_RATE    0x15

// Scan Modes
#define RPLIDAR_SCAN_MODE_STANDARD      0
#define RPLIDAR_SCAN_MODE_EXPRESS       1
#define RPLIDAR_SCAN_MODE_BOOST         2
#define RPLIDAR_SCAN_MODE_SENSITIVITY   3
#define RPLIDAR_SCAN_MODE_STABILITY     4

// Health Status
#define RPLIDAR_STATUS_OK               0x00
#define RPLIDAR_STATUS_WARNING          0x01
#define RPLIDAR_STATUS_ERROR            0x02

// Standard baud rates for different models
#define RPLIDAR_BAUD_A1M                115200
#define RPLIDAR_BAUD_A3                 256000
#define RPLIDAR_BAUD_C1                 460800
#define RPLIDAR_BAUD_C3                 460800
#define RPLIDAR_BAUD_S2                 1000000
#define RPLIDAR_BAUD_S2L                1000000
#define RPLIDAR_BAUD_S3                 1000000

// Device information structure
struct RPLidarDeviceInfo {
    uint8_t model;
    uint16_t firmware_version;
    uint8_t hardware_version;
    uint8_t serialNumber[16];
};

// Health information structure
struct RPLidarHealth {
    uint8_t status;
    uint16_t error_code;
};

// Response descriptor structure
struct RPLidarResponseDescriptor {
    uint32_t length;   // expected response data length (30-bit)
    uint8_t mode;      // send mode (0 = single, 1 = multiple)
    uint8_t type;      // data type
};

class RPLidar {
public:
    RPLidar();

    // Initialization.
    // begin(Stream&) only binds an already-configured stream (does NOT touch
    // the baud rate or pins) and returns true. Use this on ESP32 after calling
    // Serial1.begin(baud, SERIAL_8N1, rx, tx) yourself. It does not verify the
    // device is present -- call getHealth()/isConnected() for that.
    bool begin(Stream& serialObj);

    // Convenience overload for boards where the library may configure the UART:
    // calls serialObj.begin(baudRate) then binds it. Avoids dynamic_cast/RTTI.
    // NOTE: baudRate is intentionally NOT defaulted. With a default argument,
    // begin(Serial1) (one arg) would resolve to THIS overload (HardwareSerial&
    // is a better match than Stream&) and silently re-begin the UART at the
    // default baud on default pins. Requiring two args keeps begin(Serial1)
    // bound to the begin(Stream&) overload.
    bool begin(HardwareSerial& serialObj, uint32_t baudRate);

    // Device control commands
    bool stop();
    bool reset();
    bool startScan(uint8_t scanMode = RPLIDAR_SCAN_MODE_STANDARD);
    bool startExpressScan(uint8_t mode = 0);  // NOT IMPLEMENTED -> returns false
    bool forceScan();

    // Device information commands
    bool getDeviceInfo(RPLidarDeviceInfo& info);
    bool getHealth(RPLidarHealth& health);
    bool getSampleRate(uint16_t& standard_rate, uint16_t& express_rate);

    // Data reading functions
    bool waitPoint(uint32_t timeout = 1000);
    bool readMeasurement(RPLidarMeasurement& measurement);  // validates + resyncs

    // Convenience: angle + distance only (delegates to readMeasurement).
    bool readFast(float& angle, float& distance);

    // Raw data reading for custom processing
    bool readRawMeasurement(uint8_t* buffer, size_t length, uint32_t timeout = 1000);

    // Custom command support
    bool sendCommand(uint8_t cmd, const uint8_t* payload = nullptr, uint8_t payloadSize = 0);
    bool readResponse(uint8_t* buffer, size_t maxLength, uint32_t timeout = 1000);

    // Utility functions
    void flush();
    bool isConnected();
    uint32_t getDefaultBaudRate(const char* model);

private:
    Stream* _serial;
    bool _isScanning;
    uint8_t _currentScanMode;

    uint8_t _node[5];      // sliding window for measurement resynchronisation
    uint8_t _nodeLen;      // bytes currently buffered in _node

    // Internal communication functions
    bool _sendSimpleCommand(uint8_t cmd);
    bool _sendCommandWithPayload(uint8_t cmd, const uint8_t* payload, uint8_t payloadSize);
    bool _tryStartScan();   // single SCAN attempt: send + validate descriptor + confirm data
    bool _waitResponseHeader(RPLidarResponseDescriptor& descriptor, uint32_t timeout);
    bool _readResponseData(uint8_t* buffer, size_t length, uint32_t timeout);

    // Drain RX until no byte arrives for quietMs (cap at capMs). Used after
    // STOP to discard any in-flight scan data before the next command, so the
    // following response descriptor starts on a clean buffer.
    void _drainUntilQuiet(uint32_t quietMs, uint32_t capMs);
};

#endif // RPLIDAR_H
