/*
 * RPLidar.cpp - Arduino library for Slamtec RPLidar laser scanner
 */

#include "RPLidar.h"
#include <string.h>

RPLidar::RPLidar()
    : _serial(nullptr), _isScanning(false),
      _currentScanMode(RPLIDAR_SCAN_MODE_STANDARD), _nodeLen(0) {
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

bool RPLidar::begin(Stream& serialObj) {
    _serial = &serialObj;
    _isScanning = false;
    _nodeLen = 0;

    // Only bind + clear the buffer here. We deliberately do NOT send STOP:
    // some units (e.g. RPLIDAR C1) auto-scan on power-up, and stopping spins
    // the motor down -- startScan() handles the stop/settle/scan sequence.
    flush();
    return true;
}

bool RPLidar::begin(HardwareSerial& serialObj, uint32_t baudRate) {
    // static dispatch on HardwareSerial -> no dynamic_cast / no RTTI needed.
    serialObj.begin(baudRate);
    delay(100);
    return begin(static_cast<Stream&>(serialObj));
}

// ---------------------------------------------------------------------------
// Device control
// ---------------------------------------------------------------------------

bool RPLidar::stop() {
    if (!_serial) return false;

    bool result = _sendSimpleCommand(RPLIDAR_CMD_STOP);
    _isScanning = false;
    _nodeLen = 0;
    delay(10);
    // Discard any in-flight scan data so the next command's response
    // descriptor is not preceded by scan bytes (which can contain A5/5A).
    _drainUntilQuiet(30, 300);
    return result;
}

bool RPLidar::reset() {
    if (!_serial) return false;

    bool result = _sendSimpleCommand(RPLIDAR_CMD_RESET);
    _isScanning = false;
    _nodeLen = 0;
    delay(2500); // Wait for device to reboot before it accepts new commands
    flush();
    return result;
}

// Send SCAN, validate the response descriptor, and confirm nodes start
// flowing. Does NOT send STOP (stopping spins the motor down and some C1 units
// wedge on STOP/SCAN churn).
bool RPLidar::_tryStartScan() {
    flush();
    _nodeLen = 0;
    if (!_sendSimpleCommand(RPLIDAR_CMD_SCAN)) return false;

    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, 1500)) return false;
    if (descriptor.type != RPLIDAR_ANS_TYPE_MEASUREMENT) return false;

    // Confirm measurement nodes actually arrive (motor up to speed).
    return waitPoint(1500);
}

bool RPLidar::startScan(uint8_t scanMode) {
    if (!_serial) return false;
    _currentScanMode = scanMode;

    // First try a plain SCAN (works when the unit is in a clean state).
    if (_tryStartScan()) { _isScanning = true; return true; }

    // Otherwise the unit may be wedged or auto-scanning without emitting a
    // fresh descriptor. A RESET returns it to a known state; retry once.
    reset();                 // RESET + reboot wait + flush
    if (_tryStartScan()) { _isScanning = true; return true; }

    _isScanning = false;
    return false;
}

bool RPLidar::forceScan() {
    if (!_serial) return false;

    flush();
    _nodeLen = 0;

    if (!_sendSimpleCommand(RPLIDAR_CMD_FORCE_SCAN)) return false;

    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, 2000)) return false;
    if (descriptor.type != RPLIDAR_ANS_TYPE_MEASUREMENT) return false;

    _isScanning = true;
    _currentScanMode = RPLIDAR_SCAN_MODE_STANDARD;
    return true;
}

bool RPLidar::startExpressScan(uint8_t /*mode*/) {
    // NOT IMPLEMENTED.
    //
    // Express scan returns "capsuled"/"ultra-capsuled" payloads (start angle,
    // delta-encoded cabins, checksums) that need a dedicated decoder. That
    // decoder is not implemented here and could not be verified against the
    // available hardware (RPLIDAR C1 uses standard scan). Returning false
    // avoids handing the caller mis-decoded measurements.
    //
    // Use startScan() (standard scan) instead.
    return false;
}

// ---------------------------------------------------------------------------
// Device information
// ---------------------------------------------------------------------------

bool RPLidar::getDeviceInfo(RPLidarDeviceInfo& info) {
    if (!_serial) return false;
    if (_isScanning) stop();   // request/response commands must not run mid-scan

    if (!_sendSimpleCommand(RPLIDAR_CMD_GET_INFO)) return false;

    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, 1000)) return false;
    if (descriptor.type != RPLIDAR_ANS_TYPE_DEVINFO || descriptor.length < 20) {
        return false;
    }

    uint8_t buffer[20];
    if (!_readResponseData(buffer, 20, 1000)) return false;

    info.model = buffer[0];
    info.firmware_version = buffer[2] | (buffer[1] << 8);
    info.hardware_version = buffer[3];
    memcpy(info.serialNumber, &buffer[4], 16);
    return true;
}

bool RPLidar::getHealth(RPLidarHealth& health) {
    if (!_serial) return false;
    if (_isScanning) stop();

    if (!_sendSimpleCommand(RPLIDAR_CMD_GET_HEALTH)) return false;

    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, 1000)) return false;
    if (descriptor.type != RPLIDAR_ANS_TYPE_DEVHEALTH || descriptor.length < 3) {
        return false;
    }

    uint8_t buffer[3];
    if (!_readResponseData(buffer, 3, 1000)) return false;

    health.status = buffer[0];
    health.error_code = buffer[1] | (buffer[2] << 8);
    return true;
}

bool RPLidar::getSampleRate(uint16_t& standard_rate, uint16_t& express_rate) {
    if (!_serial) return false;
    if (_isScanning) stop();

    if (!_sendSimpleCommand(RPLIDAR_CMD_GET_SAMPLERATE)) return false;

    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, 1000)) return false;
    if (descriptor.type != RPLIDAR_ANS_TYPE_SAMPLE_RATE || descriptor.length < 4) {
        return false;
    }

    uint8_t buffer[4];
    if (!_readResponseData(buffer, 4, 1000)) return false;

    standard_rate = buffer[0] | (buffer[1] << 8);
    express_rate = buffer[2] | (buffer[3] << 8);
    return true;
}

// ---------------------------------------------------------------------------
// Data reading
// ---------------------------------------------------------------------------

bool RPLidar::waitPoint(uint32_t timeout) {
    if (!_serial) return false;

    uint32_t startTime = millis();
    while (millis() - startTime < timeout) {
        if (_serial->available() >= 5) return true;
        delayMicroseconds(100);
    }
    return false;
}

bool RPLidar::readMeasurement(RPLidarMeasurement& measurement) {
    if (!_serial || !_isScanning) return false;

    uint32_t deadline = millis() + 500;
    while ((int32_t)(millis() - deadline) < 0) {
        // Top the sliding window up to a full 5-byte node.
        while (_nodeLen < 5) {
            if (_serial->available() > 0) {
                _node[_nodeLen++] = (uint8_t)_serial->read();
            } else if ((int32_t)(millis() - deadline) >= 0) {
                return false;
            }
        }

        if (rplidarParseStandardNode(_node, measurement)) {
            _nodeLen = 0;     // node consumed
            return true;
        }

        // Invalid node: drop the oldest byte and try to realign on the next.
        _node[0] = _node[1];
        _node[1] = _node[2];
        _node[2] = _node[3];
        _node[3] = _node[4];
        _nodeLen = 4;
    }
    return false;
}

bool RPLidar::readFast(float& angle, float& distance) {
    RPLidarMeasurement m;
    if (!readMeasurement(m)) return false;
    angle = m.angle;
    distance = m.distance;
    return true;
}

bool RPLidar::readRawMeasurement(uint8_t* buffer, size_t length, uint32_t timeout) {
    if (!_serial || !buffer || length == 0) return false;

    uint32_t startTime = millis();
    size_t bytesRead = 0;
    while (bytesRead < length && (millis() - startTime < timeout)) {
        if (_serial->available() > 0) {
            buffer[bytesRead++] = _serial->read();
        }
    }
    return bytesRead == length;
}

// ---------------------------------------------------------------------------
// Custom commands
// ---------------------------------------------------------------------------

bool RPLidar::sendCommand(uint8_t cmd, const uint8_t* payload, uint8_t payloadSize) {
    if (payloadSize > 0 && payload == nullptr) return false;  // invalid args
    if (payloadSize == 0) {
        return _sendSimpleCommand(cmd);
    }
    return _sendCommandWithPayload(cmd, payload, payloadSize);
}

bool RPLidar::readResponse(uint8_t* buffer, size_t maxLength, uint32_t timeout) {
    if (!_serial || !buffer || maxLength == 0) return false;

    RPLidarResponseDescriptor descriptor;
    if (!_waitResponseHeader(descriptor, timeout)) return false;

    size_t toRead = (descriptor.length < (uint32_t)maxLength)
                        ? (size_t)descriptor.length : maxLength;
    if (!_readResponseData(buffer, toRead, timeout)) return false;

    // Discard any response bytes that did not fit, so the stream stays aligned.
    uint32_t startTime = millis();
    for (uint32_t i = toRead; i < descriptor.length; i++) {
        while (_serial->available() == 0) {
            if (millis() - startTime > timeout) return false;
        }
        _serial->read();
    }
    return true;
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

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
    if (!model) return RPLIDAR_BAUD_A1M;
    if (strstr(model, "A3")) return RPLIDAR_BAUD_A3;                 // 256000
    if (strstr(model, "A1") || strstr(model, "A2")) return RPLIDAR_BAUD_A1M;
    if (strstr(model, "C1") || strstr(model, "C3")) return RPLIDAR_BAUD_C1;
    if (strstr(model, "S2") || strstr(model, "S3")) return RPLIDAR_BAUD_S2;
    return RPLIDAR_BAUD_A1M; // Default
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool RPLidar::_sendSimpleCommand(uint8_t cmd) {
    if (!_serial) return false;
    _serial->write(RPLIDAR_CMD_SYNC_BYTE);
    _serial->write(cmd);
    _serial->flush();
    return true;
}

bool RPLidar::_sendCommandWithPayload(uint8_t cmd, const uint8_t* payload, uint8_t payloadSize) {
    if (!_serial) return false;

    uint8_t checksum = 0;
    checksum ^= RPLIDAR_CMD_SYNC_BYTE;
    checksum ^= cmd;
    checksum ^= payloadSize;
    for (uint8_t i = 0; i < payloadSize; i++) {
        checksum ^= payload[i];
    }

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

    // Find the two start bytes (A5 5A), tolerating leading garbage.
    while (millis() - startTime < timeout) {
        if (_serial->available() < 1) { delayMicroseconds(100); continue; }

        if ((uint8_t)_serial->read() != RPLIDAR_ANS_SYNC_BYTE1) continue;

        // Wait for the second sync byte.
        while (_serial->available() < 1) {
            if (millis() - startTime >= timeout) return false;
            delayMicroseconds(100);
        }
        if ((uint8_t)_serial->read() != RPLIDAR_ANS_SYNC_BYTE2) {
            // Not a real header; keep scanning for the next A5.
            continue;
        }

        // Read the 5-byte descriptor body.
        uint8_t descBuffer[5];
        uint32_t descStart = millis();
        int got = 0;
        while (got < 5) {
            if (_serial->available() > 0) {
                descBuffer[got++] = (uint8_t)_serial->read();
            } else if (millis() - descStart > 100) {
                return false;
            }
        }

        // 32-bit field: bits[0..29] length, bits[30..31] send mode; +1 type byte.
        descriptor.length = (uint32_t)descBuffer[0]
                          | ((uint32_t)descBuffer[1] << 8)
                          | ((uint32_t)descBuffer[2] << 16)
                          | ((uint32_t)(descBuffer[3] & 0x3F) << 24);
        descriptor.mode = (uint8_t)(descBuffer[3] >> 6);
        descriptor.type = descBuffer[4];
        return true;
    }
    return false;
}

void RPLidar::_drainUntilQuiet(uint32_t quietMs, uint32_t capMs) {
    if (!_serial) return;
    uint32_t cap = millis() + capMs;
    uint32_t lastByte = millis();
    while ((int32_t)(millis() - cap) < 0) {
        if (_serial->available() > 0) {
            _serial->read();
            lastByte = millis();
        } else if (millis() - lastByte >= quietMs) {
            break;
        }
    }
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
