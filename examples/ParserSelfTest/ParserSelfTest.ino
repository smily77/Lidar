/*
 * ParserSelfTest.ino - runs the standard-scan node decoder against known
 * 5-byte fixtures and prints PASS/FAIL over USB Serial. No lidar hardware
 * required. Mirrors test/test_parser.cpp for boards without a host compiler.
 */

#include <RPLidarParser.h>

struct Fixture {
  const char* name;
  uint8_t bytes[5];
  bool expectValid;
  bool startBit;
  uint8_t quality;
  float angle;
  float distance;
};

static const Fixture fixtures[] = {
  { "real C1 node",        {0x8E,0x6D,0x20,0xAC,0x12}, true,  false, 35, 64.84375f, 1195.0f },
  { "exact 90deg/1000mm",  {0x52,0x01,0x2D,0xA0,0x0F}, true,  false, 20, 90.0f,     1000.0f },
  { "start bit node",      {0x01,0x01,0x00,0x00,0x00}, true,  true,   0,  0.0f,        0.0f },
  { "descriptor A5 5A",    {0xA5,0x5A,0x05,0x00,0x00}, false, false,  0,  0.0f,        0.0f },
  { "S/!S not complement", {0x00,0x01,0x00,0x00,0x00}, false, false,  0,  0.0f,        0.0f },
  { "check bit cleared",   {0x02,0x00,0x00,0x00,0x00}, false, false,  0,  0.0f,        0.0f },
};

static bool feq(float a, float b) { return fabs(a - b) <= 0.01f; }

void setup() {
  Serial.begin(115200);
  delay(300);
}

void loop() {
  int failures = 0;
  const int n = sizeof(fixtures) / sizeof(fixtures[0]);
  Serial.println("=== RPLidar parser self-test ===");
  for (int i = 0; i < n; i++) {
    const Fixture& f = fixtures[i];
    RPLidarMeasurement m;
    bool valid = rplidarParseStandardNode(f.bytes, m);
    bool ok = (valid == f.expectValid);
    if (f.expectValid && valid) {
      ok = ok && m.startBit == f.startBit && m.quality == f.quality
              && feq(m.angle, f.angle) && feq(m.distance, f.distance);
    }
    if (!ok) failures++;
    Serial.print(ok ? "  PASS  " : "  FAIL  ");
    Serial.println(f.name);
  }
  Serial.print(failures == 0 ? "ALL PASSED" : "FAILED");
  Serial.print(" failures=");
  Serial.println(failures);
  delay(2000);
}
