/*
 * RPLidar.cpp - Arduino library for Slamtec RPLidar laser scanner
 */

#include "RPLidar.h"

RPLidar::RPLidar() : _serial(nullptr), _isScanning(false), _currentScanMode(RPLIDAR_SCAN_MODE_STANDARD) {
}

bool RPLidar::begin(Stream& serialObj, uint32_t baudRate) {
    _serial = &serialObj;

    // For HardwareSerial, set baud rate
    HardwareSerial* hwSerial = dynamic_cast<HardwareSerial*>(_serial);
    if (hwSerial) {
        hwSerial->begin(baudRate);
        delay(100);
    }

    flush();

    // Stop any ongoing scan
    stop();
    delay(10);

    return true;
}

bool RPLidar::stop() {
    if (!_serial) return false;

    bool result = _sendSimpleCommand(RPLIDAR_CMD_STOP);
    _isScanning = false;
    delay(10);
    flush();

    return result;
}

bool RPLidar::reset() {
    if (!_serial) return false;

    bool result = _sendSimpleCommand(RPLIDAR_CMD_RESET);
    _isScanning = false;
    delay(2000); // Wait for device to reset

    flush();
    return result;
}

bool RPLidar::startScan(uint8_t scanMode) {
    if (!_serial) return false;

    stop();
    delay(10);

    bool result = _sendSimpleCommand(RPLIDAR_CMD_SCAN);
    if (result) {
        _isScanning = true;
        _currentScanMode = scanMode;
    }

    return result;
}

bool RPLidar::startExpressScan(uint8_t mode) {
    if (!_serial) return false;

    stop();
    delay(10);

    uint8_t payload[5] = {0};
    payload[0] = mode;

    bool result = _sendCommandWithPayload(RPLIDAR_CMD_EXPRESS_SCAN, payload, 5);
    if (result) {
        _isScanning = true;
        _currentScanMode = RPLIDAR_SCAN_MODE_EXPRESS;
    }

    return result;
}

bool RPLidar::forceScan() {
    if (!_serial) return false;

    stop();
    delay(10);

    bool result = _sendSimpleCommand(RPLIDAR_CMD_FORCE_SCAN);
    if (result) {
        _isScanning = true;
    }

    return result;
}

bool RPLidar::getDeviceInfo(RPLidarDeviceInfo& info) {
    if (!_serial) return false;

    // Send GET_INFO command
    if (!_sendSimpleCommand(RPLIDAR_CMD_GET_INFO)) {
        return false;
    }

    // Wait for response descriptor
    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, 1000)) {
        return false;
    }

    // Check response type
    if (descriptor.type != RPLIDAR_ANS_TYPE_DEVINFO) {
        return false;
    }

    // Read device info (20 bytes)
    uint8_t buffer[20];
    if (!_readResponseData(buffer, 20, 1000)) {
        return false;
    }

    // Parse device info
    info.model = buffer[0];
    info.firmware_version = buffer[2] | (buffer[1] << 8);
    info.hardware_version = buffer[3];
    memcpy(info.serialNumber, &buffer[4], 16);

    return true;
}

bool RPLidar::getHealth(RPLidarHealth& health) {
    if (!_serial) return false;

    // Send GET_HEALTH command
    if (!_sendSimpleCommand(RPLIDAR_CMD_GET_HEALTH)) {
        return false;
    }

    // Wait for response descriptor
    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, 1000)) {
        return false;
    }

    // Check response type
    if (descriptor.type != RPLIDAR_ANS_TYPE_DEVHEALTH) {
        return false;
    }

    // Read health data (3 bytes)
    uint8_t buffer[3];
    if (!_readResponseData(buffer, 3, 1000)) {
        return false;
    }

    // Parse health info
    health.status = buffer[0];
    health.error_code = buffer[1] | (buffer[2] << 8);

    return true;
}

bool RPLidar::getSampleRate(uint16_t& standard_rate, uint16_t& express_rate) {
    if (!_serial) return false;

    // Send GET_SAMPLERATE command
    if (!_sendSimpleCommand(RPLIDAR_CMD_GET_SAMPLERATE)) {
        return false;
    }

    // Wait for response descriptor
    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, 1000)) {
        return false;
    }

    // Read sample rate data (4 bytes)
    uint8_t buffer[4];
    if (!_readResponseData(buffer, 4, 1000)) {
        return false;
    }

    // Parse sample rate
    standard_rate = buffer[0] | (buffer[1] << 8);
    express_rate = buffer[2] | (buffer[3] << 8);

    return true;
}

bool RPLidar::waitPoint(uint32_t timeout) {
    if (!_serial) return false;

    uint32_t startTime = millis();
    while (millis() - startTime < timeout) {
        if (_serial->available() >= 5) {
            return true;
        }
        delayMicroseconds(100);
    }

    return false;
}

bool RPLidar::readMeasurement(RPLidarMeasurement& measurement) {
    if (!_serial || !_isScanning) return false;

    // Wait for at least 5 bytes (standard measurement packet)
    if (!waitPoint(500)) {
        return false;
    }

    // Read measurement data
    uint8_t buffer[5];
    buffer[0] = _serial->read();
    buffer[1] = _serial->read();
    buffer[2] = _serial->read();
    buffer[3] = _serial->read();
    buffer[4] = _serial->read();

    return _parseMeasurementNode(buffer, measurement);
}

bool RPLidar::readFast(float& angle, float& distance) {
    if (!_serial || !_isScanning) return false;

    // Optimized for minimal overhead - inline reading and parsing
    // Wait for data with minimal delay
    uint32_t startTime = millis();
    while (_serial->available() < 5) {
        if (millis() - startTime > 500) return false;
        // No delay - maximum speed
    }

    // Read bytes directly
    uint8_t byte0 = _serial->read();
    uint8_t byte1 = _serial->read();
    uint8_t byte2 = _serial->read();
    uint8_t byte3 = _serial->read();
    uint8_t byte4 = _serial->read();

    // Parse angle (inline for speed)
    // Angle is in bytes 1-2: (byte2 >> 1) | (byte1 << 7)
    uint16_t angle_raw = ((byte2 >> 1) | (byte1 << 7));
    angle = angle_raw / 64.0f;

    // Parse distance (inline for speed)
    // Distance is in bytes 3-4
    uint16_t distance_raw = byte3 | (byte4 << 8);
    distance = distance_raw / 4.0f;

    return true;
}

bool RPLidar::readRawMeasurement(uint8_t* buffer, size_t length, uint32_t timeout) {
    if (!_serial) return false;

    uint32_t startTime = millis();
    size_t bytesRead = 0;

    while (bytesRead < length && (millis() - startTime < timeout)) {
        if (_serial->available() > 0) {
            buffer[bytesRead++] = _serial->read();
        }
    }

    return bytesRead == length;
}

bool RPLidar::sendCommand(uint8_t cmd, const uint8_t* payload, uint8_t payloadSize) {
    if (payloadSize == 0) {
        return _sendSimpleCommand(cmd);
    } else {
        return _sendCommandWithPayload(cmd, payload, payloadSize);
    }
}

bool RPLidar::readResponse(uint8_t* buffer, size_t maxLength, uint32_t timeout) {
    if (!_serial) return false;

    // Wait for response descriptor
    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, timeout)) {
        return false;
    }

    // Read response data
    size_t bytesToRead = min(descriptor.length, (uint32_t)maxLength);
    return _readResponseData(buffer, bytesToRead, timeout);
}

void RPLidar::flush() {
    if (!_serial) return;

    while (_serial->available() > 0) {
        _serial->read();
    }
}

bool RPLidar::isConnected() {
    if (!_serial) return false;

    RPLidarHealth health;
    return getHealth(health);
}

uint32_t RPLidar::getDefaultBaudRate(const char* model) {
    if (strstr(model, "A1") || strstr(model, "A2") || strstr(model, "A3")) {
        return RPLIDAR_BAUD_A1M;
    } else if (strstr(model, "C1") || strstr(model, "C3")) {
        return RPLIDAR_BAUD_C1;
    } else if (strstr(model, "S2") || strstr(model, "S3")) {
        return RPLIDAR_BAUD_S2;
    }
    return RPLIDAR_BAUD_A1M; // Default
}

// Private methods

bool RPLidar::_sendSimpleCommand(uint8_t cmd) {
    if (!_serial) return false;

    _serial->write(RPLIDAR_CMD_SYNC_BYTE);
    _serial->write(cmd);
    _serial->flush();

    return true;
}

bool RPLidar::_sendCommandWithPayload(uint8_t cmd, const uint8_t* payload, uint8_t payloadSize) {
    if (!_serial) return false;

    // Calculate checksum
    uint8_t checksum = 0;
    checksum ^= RPLIDAR_CMD_SYNC_BYTE;
    checksum ^= cmd;
    checksum ^= payloadSize;
    for (uint8_t i = 0; i < payloadSize; i++) {
        checksum ^= payload[i];
    }

    // Send command packet
    _serial->write(RPLIDAR_CMD_SYNC_BYTE);
    _serial->write(cmd);
    _serial->write(payloadSize);
    _serial->write(payload, payloadSize);
    _serial->write(checksum);
    _serial->flush();

    return true;
}

bool RPLidar::_waitResponseHeader(RPLidarResponseDescriptor& descriptor, uint32_t timeout) {
    if (!_serial) return false;

    uint32_t startTime = millis();

    // Wait for start bytes
    while (millis() - startTime < timeout) {
        if (_serial->available() < 2) {
            delayMicroseconds(100);
            continue;
        }

        uint8_t byte1 = _serial->read();
        if (byte1 != RPLIDAR_ANS_SYNC_BYTE1) {
            continue;
        }

        uint8_t byte2 = _serial->read();
        if (byte2 != RPLIDAR_ANS_SYNC_BYTE2) {
            continue;
        }

        // Read descriptor (5 bytes remaining)
        uint32_t descStartTime = millis();
        while (_serial->available() < 5 && (millis() - descStartTime < 100)) {
            delayMicroseconds(100);
        }

        if (_serial->available() < 5) {
            return false;
        }

        uint8_t descBuffer[5];
        for (int i = 0; i < 5; i++) {
            descBuffer[i] = _serial->read();
        }

        // Parse descriptor
        descriptor.length = descBuffer[0] | (descBuffer[1] << 8) | (descBuffer[2] << 16) | (descBuffer[3] << 24);
        descriptor.length &= 0x3FFFFFFF; // 30-bit length
        descriptor.mode = (descBuffer[3] >> 6) | ((descBuffer[4] & 0x03) << 2);
        descriptor.type = descBuffer[4];

        return true;
    }

    return false;
}

bool RPLidar::_readResponseData(uint8_t* buffer, size_t length, uint32_t timeout) {
    if (!_serial) return false;

    uint32_t startTime = millis();
    size_t bytesRead = 0;

    while (bytesRead < length && (millis() - startTime < timeout)) {
        if (_serial->available() > 0) {
            buffer[bytesRead++] = _serial->read();
        } else {
            delayMicroseconds(100);
        }
    }

    return bytesRead == length;
}

inline bool RPLidar::_parseMeasurementNode(const uint8_t* buffer, RPLidarMeasurement& measurement) {
    // Parse standard scan measurement packet (5 bytes)
    // Byte 0: [S(1bit)][!S(1bit)][Quality(6bits)]
    // Byte 1-2: [C(1bit)][Angle_q6(15bits)]
    // Byte 3-4: [Distance_q2(16bits)]

    uint8_t quality = buffer[0] & 0x3F;
    bool startBit = (buffer[0] & 0x01) != 0;
    bool checkBit = (buffer[1] & 0x01) != 0;

    // Check bit validation
    if (checkBit != ((buffer[0] & 0x01) ^ 0x01)) {
        return false;
    }

    uint16_t angle_q6 = ((buffer[2] >> 1) | (buffer[1] << 7)) & 0x7FFF;
    uint16_t distance_q2 = buffer[3] | (buffer[4] << 8);

    measurement.quality = quality;
    measurement.startBit = startBit;
    measurement.angle = angle_q6 / 64.0f;
    measurement.distance = distance_q2 / 4.0f;

    return true;
}

uint8_t RPLidar::_calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}
