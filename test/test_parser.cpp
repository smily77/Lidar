/*
 * Host-side unit tests for the RPLIDAR standard-scan node decoder.
 *
 * Builds without Arduino (RPLidarParser.h only depends on <stdint.h>).
 *
 *   g++ -std=c++11 -fno-rtti -Wall -I../src test_parser.cpp -o test_parser
 *   ./test_parser
 *
 * Exits 0 when all cases pass, 1 otherwise.
 */

#include "../src/RPLidarParser.h"
#include <cstdio>
#include <cmath>

static int g_failures = 0;

static void check(bool cond, const char* what) {
    if (!cond) { printf("  FAIL: %s\n", what); g_failures++; }
    else       { printf("  ok:   %s\n", what); }
}

static bool feq(float a, float b, float tol = 0.01f) {
    return std::fabs(a - b) <= tol;
}

// Each fixture: the 5 raw bytes, whether it should validate, and (if valid)
// the expected decoded values.
struct Fixture {
    const char* name;
    uint8_t bytes[5];
    bool expectValid;
    bool startBit;
    uint8_t quality;
    float angle;
    float distance;
};

int main() {
    Fixture fixtures[] = {
        // Real C1 node captured over USB: ang 64.84, dist 1195, q 35.
        { "real C1 node",      {0x8E,0x6D,0x20,0xAC,0x12}, true,  false, 35, 64.84375f, 1195.0f },
        // Hand-computed: angle exactly 90 deg, distance exactly 1000 mm, q 20.
        { "exact 90deg/1000mm",{0x52,0x01,0x2D,0xA0,0x0F}, true,  false, 20, 90.0f,     1000.0f },
        // Start-of-scan node (S=1), angle 0, distance 0.
        { "start bit node",    {0x01,0x01,0x00,0x00,0x00}, true,  true,   0,  0.0f,        0.0f },
        // Scan response descriptor leading bytes: check bit (0x5A&1) == 0 -> reject.
        { "descriptor A5 5A",  {0xA5,0x5A,0x05,0x00,0x00}, false, false,  0,  0.0f,        0.0f },
        // S and !S both 0 -> reject.
        { "S/!S not complement",{0x00,0x01,0x00,0x00,0x00}, false, false, 0,  0.0f,        0.0f },
        // Valid S/!S but check bit cleared -> reject.
        { "check bit cleared", {0x02,0x00,0x00,0x00,0x00}, false, false,  0,  0.0f,        0.0f },
    };

    const int n = (int)(sizeof(fixtures) / sizeof(fixtures[0]));
    for (int i = 0; i < n; i++) {
        const Fixture& f = fixtures[i];
        printf("[%s]\n", f.name);
        RPLidarMeasurement m;
        bool valid = rplidarParseStandardNode(f.bytes, m);
        check(valid == f.expectValid, "validity matches expectation");
        if (f.expectValid && valid) {
            check(m.startBit == f.startBit, "startBit");
            check(m.quality == f.quality,   "quality");
            check(feq(m.angle, f.angle),    "angle");
            check(feq(m.distance, f.distance), "distance");
        }
    }

    printf("\n%s (%d failure%s)\n",
           g_failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
           g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
