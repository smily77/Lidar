/*
 * RPLidar.h - Arduino library for Slamtec RPLidar laser scanner
 *
 * Supports: A1M, C1, C3, S2, S2L, S3 and compatible models
 *
 * Features:
 * - All standard RPLidar commands
 * - Optimized high-speed data reading (minimal overhead)
 * - Custom command support
 * - Express scan mode support
 *
 * Author: RPLidar Arduino Library
 * License: MIT
 */

#ifndef RPLIDAR_H
#define RPLIDAR_H

#include <Arduino.h>

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
#define RPLIDAR_BAUD_C1                 460800
#define RPLIDAR_BAUD_C3                 460800
#define RPLIDAR_BAUD_S2                 1000000
#define RPLIDAR_BAUD_S2L                1000000
#define RPLIDAR_BAUD_S3                 1000000

// Measurement data structure (optimized for minimal overhead)
struct RPLidarMeasurement {
    float angle;        // Angle in degrees (0-360)
    float distance;     // Distance in millimeters
    uint8_t quality;    // Signal quality (0-255)
    bool startBit;      // True if this is the start of a new scan
};

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
    uint32_t length;
    uint8_t mode;
    uint8_t type;
};

class RPLidar {
public:
    // Constructor
    RPLidar();

    // Initialization
    bool begin(Stream& serialObj, uint32_t baudRate = RPLIDAR_BAUD_A1M);

    // Device control commands
    bool stop();
    bool reset();
    bool startScan(uint8_t scanMode = RPLIDAR_SCAN_MODE_STANDARD);
    bool startExpressScan(uint8_t mode = 0);
    bool forceScan();

    // Device information commands
    bool getDeviceInfo(RPLidarDeviceInfo& info);
    bool getHealth(RPLidarHealth& health);
    bool getSampleRate(uint16_t& standard_rate, uint16_t& express_rate);

    // Data reading functions
    bool waitPoint(uint32_t timeout = 1000);
    bool readMeasurement(RPLidarMeasurement& measurement);

    // Optimized high-speed reading (minimal overhead - only distance and angle)
    // Returns: true if data read successfully
    // This function is optimized for ESP32 with S2L high data rate
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

    // Internal communication functions
    bool _sendSimpleCommand(uint8_t cmd);
    bool _sendCommandWithPayload(uint8_t cmd, const uint8_t* payload, uint8_t payloadSize);
    bool _waitResponseHeader(RPLidarResponseDescriptor& descriptor, uint32_t timeout);
    bool _readResponseData(uint8_t* buffer, size_t length, uint32_t timeout);

    // Fast inline parsing for optimized reading
    inline bool _parseMeasurementNode(const uint8_t* buffer, RPLidarMeasurement& measurement);

    // Checksum calculation
    uint8_t _calculateChecksum(const uint8_t* data, size_t length);
};

#endif // RPLIDAR_H
