# Host tests

Pure, Arduino-free unit tests for the standard-scan node decoder
(`src/RPLidarParser.h`).

## Run

Any host C++ compiler works; RTTI is disabled to match the Arduino/ESP32
build flags:

```sh
g++ -std=c++11 -fno-rtti -Wall -I../src test_parser.cpp -o test_parser
./test_parser
```

Exit code `0` = all fixtures passed, `1` = at least one failure.

The same fixtures also run on-device via the `ParserSelfTest` example, which
is useful when no host compiler is available.
