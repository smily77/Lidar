/*
 * RPLidarParser.h - Pure, Arduino-independent decoder for RPLidar nodes.
 *
 * Kept free of <Arduino.h> / Stream so the parsing logic can be unit-tested
 * on the host (see test/test_parser.cpp). Only depends on <stdint.h>.
 */

#ifndef RPLIDAR_PARSER_H
#define RPLIDAR_PARSER_H

#include <stdint.h>

// Measurement data structure for a single standard-scan sample.
struct RPLidarMeasurement {
    float angle;        // Angle in degrees (0..360)
    float distance;     // Distance in millimeters (0 = invalid / no return)
    uint8_t quality;    // Signal quality (0..63)
    bool startBit;      // True on the first node of a new 360-degree scan
};

// Decode + validate a 5-byte RPLIDAR standard-scan node.
//
// Layout (Slamtec standard scan, little-endian; verified on RPLIDAR C1):
//   byte0: bit0 = S, bit1 = !S, bits[7:2] = quality (6 bits)
//   byte1: bit0 = C (check bit, always 1), bits[7:1] = angle_q6[6:0]
//   byte2:                                  angle_q6[14:7]
//   byte3: distance_q2[7:0]
//   byte4: distance_q2[15:8]
//
//   angle_q6   = (byte1 >> 1) | (byte2 << 7)   -> angle    = angle_q6   / 64.0
//   distance_q2 = byte3 | (byte4 << 8)          -> distance = distance_q2 / 4.0
//
// Returns false when the node fails the sync checks (S must differ from !S,
// and the check bit must be set). Callers use a false result to resynchronise
// the byte stream by sliding one byte and retrying.
static inline bool rplidarParseStandardNode(const uint8_t b[5], RPLidarMeasurement& m) {
    bool s     = (b[0] & 0x01) != 0;
    bool sInv  = (b[0] & 0x02) != 0;
    bool check = (b[1] & 0x01) != 0;

    if (s == sInv) return false;   // S and !S must be complementary
    if (!check)    return false;   // check bit must be set

    m.startBit = s;
    m.quality  = (uint8_t)(b[0] >> 2);

    uint16_t angle_q6 = (uint16_t)(b[1] >> 1) | ((uint16_t)b[2] << 7);
    m.angle = angle_q6 / 64.0f;

    uint16_t dist_q2 = (uint16_t)b[3] | ((uint16_t)b[4] << 8);
    m.distance = dist_q2 / 4.0f;

    return true;
}

#endif // RPLIDAR_PARSER_H
