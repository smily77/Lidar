# RPLidar Arduino Library

Eine umfassende Arduino-Bibliothek für Slamtec RPLidar Laserscanner mit seriellem Interface.

## Status / getestet

- **Getestet:** RPLIDAR **C1** an **ESP32-S3** (Standard-Scan, 460800 Baud).
- Protokoll-korrekt implementiert: Standard-Scan, Geräte-Info/Health, Sample-Rate.
- **Express-Scan ist NICHT implementiert** (`startExpressScan()` gibt absichtlich
  `false` zurück) — siehe Hinweis weiter unten.
- `GET_LIDAR_CONF` (0x84) ist nur als Konstante vorhanden, keine High-Level-API.

## Unterstützte Modelle

Das Standard-Scan-Protokoll ist bei A-, C- und S-Serie gleich; die Bibliothek
sollte daher mit A1M/A2/A3, C1/C3 und S2/S2L/S3 im **Standard-Scan** funktionieren.
Verifiziert wurde bisher nur der C1 (siehe oben). Modelle, die zwingend Express-Scan
brauchen, werden derzeit nicht unterstützt.

## Features

- ✅ Standard-Scan mit Byte-Stream-**Resynchronisierung** (robuste Ausrichtung)
- ✅ Geräteinformationen, Gesundheitsstatus, Sample-Rate (mit Descriptor-Prüfung)
- ✅ Benutzerdefinierte Befehle (`sendCommand`/`readResponse`)
- ✅ Reiner, host-testbarer Node-Parser (`RPLidarParser.h`, siehe `test/`)
- ✅ Ohne RTTI baubar (`-fno-rtti`) — läuft auf ESP32
- ⛔ Express-Scan: **nicht implementiert**

## Behobene Probleme (1.1.0)

Diese Version behebt mehrere Protokoll- und Robustheitsfehler, durch die der
C1 an ESP32 keine bzw. fehlerhafte Daten lieferte:

- **`begin()` startet die UART nicht mehr ungewollt neu.** `begin(Stream&)`
  *bindet* nur noch den bereits konfigurierten Stream (kein `dynamic_cast`,
  RTTI-frei) und lässt Baudrate **und Pins** unangetastet — entscheidend auf
  ESP32 mit eigenen RX/TX-Pins. Der neue Overload
  `begin(HardwareSerial&, baudRate)` hat **bewusst kein** Default-Argument,
  damit `lidar.begin(Serial1)` nicht versehentlich die UART auf Default-Pins
  neu startet.
- **Korrektes Node-Parsing.** Die Signalqualität wird wieder aus den Bits 2–7
  gelesen (vorher fälschlich `& 0x3F`). Der reine Decoder liegt jetzt in
  `RPLidarParser.h` (ohne Arduino-Abhängigkeit) und ist host-getestet (`test/`,
  Beispiel `ParserSelfTest`).
- **Byte-Stream-Resynchronisierung** in `readMeasurement()`: ein 5-Byte-
  Sliding-Window richtet sich bei ungültigen Nodes byteweise neu aus, statt
  blind fünf Bytes zu lesen und so dauerhaft aus dem Takt zu geraten.
- **Korrektes Parsen des Response-Descriptors.** Länge (30 Bit) und Send-Mode
  (2 Bit) werden sauber getrennt (vorher floss das Typ-Byte in das Mode-Feld);
  Descriptor-**Typ und Mindestlänge** werden für Scan/Info/Health/Sample-Rate
  validiert.
- **Robusterer `startScan()`.** Sendet SCAN, validiert den Descriptor und
  bestätigt, dass tatsächlich Nodes eintreffen. Hängt/auto-scannt das Gerät,
  wird automatisch ein `reset()` + genau ein erneuter Versuch ausgeführt. Das
  frühere STOP/SCAN-„Churn" (an dem manche C1-Einheiten hängen blieben)
  entfällt.
- **`stop()` leert bis Ruhe** (`_drainUntilQuiet`), damit der nächste
  Befehls-Descriptor auf einem sauberen Puffer beginnt. Info-/Health-/
  Sample-Rate-Abfragen stoppen vorher automatisch einen laufenden Scan.
- **`readFast()`** nutzt jetzt denselben validierten Pfad wie
  `readMeasurement()` (vorher ungeprüftes Roh-Parsen ohne Check-Bit/Resync).
- **Express-Scan** ist ehrlich als *nicht implementiert* markiert
  (`startExpressScan()` → `false`), statt mis-dekodierte Messwerte zu liefern.
- Kleinkram: Null-Pointer-Checks in den Custom-Command-/Raw-APIs, A3-Baudrate
  (256000) ergänzt, durchgängig `-fno-rtti`-tauglich.

## Installation

### Arduino IDE

1. Klonen Sie dieses Repository:
   ```bash
   git clone https://github.com/smily77/Lidar.git
   ```

2. Kopieren Sie den Ordner `Lidar` in Ihr Arduino libraries Verzeichnis:
   - **Windows**: `Dokumente\Arduino\libraries\`
   - **Mac**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`

3. Starten Sie die Arduino IDE neu

4. Die Bibliothek sollte nun unter `Sketch > Include Library > RPLidar` verfügbar sein

### Alternativ: ZIP-Installation

1. Laden Sie das Repository als ZIP herunter
2. In Arduino IDE: `Sketch > Include Library > Add .ZIP Library...`
3. Wählen Sie die heruntergeladene ZIP-Datei

## Hardware-Verbindung

### ESP32 Beispiel

```
RPLidar TX  ->  ESP32 RX (z.B. GPIO 16)
RPLidar RX  ->  ESP32 TX (z.B. GPIO 17)
RPLidar 5V  ->  5V (externe Stromversorgung für S2L empfohlen!)
RPLidar GND ->  GND
RPLidar MOTOR_PWM -> Optional für Motorsteuerung
```

### Arduino Mega

```
RPLidar TX  ->  Arduino Mega RX1 (Pin 19)
RPLidar RX  ->  Arduino Mega TX1 (Pin 18)
RPLidar 5V  ->  5V
RPLidar GND ->  GND
```

## Baud-Raten

Die richtige Baud-Rate ist **kritisch** für erfolgreiche Kommunikation:

| Modell | Baud-Rate | Konstante |
|--------|-----------|-----------|
| A1M, A2 | 115200 | `RPLIDAR_BAUD_A1M` |
| A3 | 256000 | `RPLIDAR_BAUD_A3` |
| C1, C3 | 460800 | `RPLIDAR_BAUD_C1` |
| S2, S2L, S3 | 1000000 | `RPLIDAR_BAUD_S2L` |

## Schnellstart

```cpp
#include <RPLidar.h>

RPLidar lidar;

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);  // Für A1M

    lidar.begin(Serial2, RPLIDAR_BAUD_A1M);
    lidar.startScan();
}

void loop() {
    RPLidarMeasurement measurement;

    if (lidar.readMeasurement(measurement)) {
        Serial.print("Winkel: ");
        Serial.print(measurement.angle);
        Serial.print(" Distanz: ");
        Serial.println(measurement.distance);
    }
}
```

## API-Referenz

### Initialisierung

#### `begin(Stream& serialObj)`
Bindet einen **bereits konfigurierten** Stream. Setzt **weder** Baudrate noch Pins
(wichtig auf ESP32, wo eigene RX/TX-Pins nötig sind) und prüft nicht, ob ein Gerät
antwortet — dafür `getHealth()`/`isConnected()` nutzen. Gibt `true` zurück.

```cpp
Serial1.begin(RPLIDAR_BAUD_C1, SERIAL_8N1, 7, 8);  // eigene Pins zuerst
delay(1000);                                        // UART hochfahren lassen
lidar.begin(Serial1);                               // dann binden
```

#### `begin(HardwareSerial& serialObj, uint32_t baudRate = RPLIDAR_BAUD_A1M)`
Komfort-Overload für Boards mit Default-UART-Pins: ruft `serialObj.begin(baudRate)`
auf und bindet den Stream. Verwendet **kein** `dynamic_cast` (RTTI-frei). Auf ESP32
mit eigenen RX/TX-Pins stattdessen die `begin(Stream&)`-Variante verwenden.

### Gerätekontrolle

#### `stop()`
Stoppt den laufenden Scan.

#### `reset()`
Setzt das RPLidar zurück (Neustart).

#### `startScan(uint8_t scanMode = RPLIDAR_SCAN_MODE_STANDARD)`
Startet Standard-Scan-Modus.

#### `startExpressScan(uint8_t mode = 0)`
**Nicht implementiert** — gibt immer `false` zurück. Der Express-Cabin-Decoder
ist nicht vorhanden/verifiziert; nutzen Sie `startScan()` (Standard-Scan).

#### `forceScan()`
Startet Scan ohne Rotation Check (für Tests).

### Geräteinformationen

#### `getDeviceInfo(RPLidarDeviceInfo& info)`
Liest Geräteinformationen.

**Beispiel:**
```cpp
RPLidarDeviceInfo info;
if (lidar.getDeviceInfo(info)) {
    Serial.print("Modell: ");
    Serial.println(info.model);
    Serial.print("Firmware: ");
    Serial.println(info.firmware_version);
}
```

#### `getHealth(RPLidarHealth& health)`
Liest Gesundheitsstatus des Geräts.

**Beispiel:**
```cpp
RPLidarHealth health;
if (lidar.getHealth(health)) {
    if (health.status == RPLIDAR_STATUS_OK) {
        Serial.println("Gerät OK");
    } else {
        Serial.print("Fehler: ");
        Serial.println(health.error_code);
    }
}
```

#### `getSampleRate(uint16_t& standard_rate, uint16_t& express_rate)`
Liest Abtastraten für Standard- und Express-Modi.

### Datenauslesen

#### `readMeasurement(RPLidarMeasurement& measurement)`
Liest eine vollständige Messung (Winkel, Distanz, Qualität, Start-Bit).

**Beispiel:**
```cpp
RPLidarMeasurement measurement;
if (lidar.readMeasurement(measurement)) {
    Serial.print("Winkel: ");
    Serial.print(measurement.angle);      // 0-360 Grad
    Serial.print(" Distanz: ");
    Serial.print(measurement.distance);    // mm
    Serial.print(" Qualität: ");
    Serial.print(measurement.quality);     // 0-63
    if (measurement.startBit) {
        Serial.println(" [NEUER SCAN]");
    }
}
```

#### `readFast(float& angle, float& distance)`
Komfort-Variante, die nur Winkel und Distanz liefert. Delegiert intern an
`readMeasurement()` — verwendet also denselben validierten Parser **inklusive**
Check-Bit-Prüfung und Resynchronisierung (kein ungeprüftes Parsen mehr).

```cpp
float angle, distance;
if (lidar.readFast(angle, distance)) {
    sendViaUDP(angle, distance);
}
```

#### `readRawMeasurement(uint8_t* buffer, size_t length, uint32_t timeout)`
Liest rohe Messdaten für benutzerdefinierte Verarbeitung.

### Benutzerdefinierte Befehle

#### `sendCommand(uint8_t cmd, const uint8_t* payload, uint8_t payloadSize)`
Sendet benutzerdefinierten Befehl an RPLidar.

#### `readResponse(uint8_t* buffer, size_t maxLength, uint32_t timeout)`
Liest Antwort auf benutzerdefinierten Befehl.

**Beispiel:**
```cpp
// Eigenen Befehl senden
uint8_t customCmd = 0x84;
uint8_t payload[2] = {0x00, 0x01};
lidar.sendCommand(customCmd, payload, 2);

// Antwort lesen
uint8_t response[64];
if (lidar.readResponse(response, sizeof(response), 1000)) {
    // Antwort verarbeiten
}
```

### Hilfsfunktionen

#### `flush()`
Leert den Empfangspuffer.

#### `isConnected()`
Prüft ob Verbindung zum RPLidar besteht.

#### `getDefaultBaudRate(const char* model)`
Gibt empfohlene Baud-Rate für Modellname zurück.

## Datenstrukturen

### RPLidarMeasurement

```cpp
struct RPLidarMeasurement {
    float angle;        // Winkel in Grad (0-360)
    float distance;     // Distanz in mm (0 = ungültig / kein Echo)
    uint8_t quality;    // Signalqualität (0-63)
    bool startBit;      // true wenn Start eines neuen Scans
};
```

> Hinweis: `RPLidarMeasurement` und der reine Decoder
> `rplidarParseStandardNode()` sind in `src/RPLidarParser.h` definiert (ohne
> Arduino-Abhängigkeit, damit host-testbar — siehe `test/`).

### RPLidarDeviceInfo

```cpp
struct RPLidarDeviceInfo {
    uint8_t model;
    uint16_t firmware_version;
    uint8_t hardware_version;
    uint8_t serialNumber[16];
};
```

### RPLidarHealth

```cpp
struct RPLidarHealth {
    uint8_t status;        // RPLIDAR_STATUS_OK, _WARNING, oder _ERROR
    uint16_t error_code;
};
```

## Beispiele

### 1. BasicScan
Grundlegende Scan-Funktionalität mit vollständiger Messdaten-Ausgabe.

```arduino
#include <RPLidar.h>
// ... siehe examples/BasicScan/BasicScan.ino
```

### 2. DeviceInfo
Zeigt alle Geräteinformationen, Gesundheitsstatus und Abtastraten.

### 3. UltraFastScan ⚡
Zeigt hohen Durchsatz mit `readFast()`. Hinweis: `readFast()` delegiert seit
1.1.0 an den **validierten** Parser (mit Check-Bit-Prüfung und Resync), liefert
also geprüfte statt roher Werte. Die S2L-Hinweise unten sind nicht auf C1-
Hardware verifiziert.
- Batch-Verarbeitung
- UDP-Übertragung (Beispiel)
- Performance-Statistiken

### 4. ExpressScan
Dokumentiert, dass Express-Scan **nicht implementiert** ist: zeigt den
(erwarteten) `false`-Rückgabewert von `startExpressScan()` und verweist auf den
Standard-Scan (`BasicScan`).

### 5. CustomCommand
Zeigt wie man benutzerdefinierte Befehle sendet und Antworten liest.

## Performance-Optimierung für S2L

Der S2L kann bis zu **32.000 Samples/Sekunde** liefern. Um diese Datenrate zu bewältigen:

### 1. ESP32-Konfiguration

```cpp
// CPU auf 240 MHz setzen (in Arduino IDE: Tools > CPU Frequency > 240 MHz)
Serial2.begin(1000000, SERIAL_8N1, RX_PIN, TX_PIN);
Serial2.setRxBufferSize(2048);  // Buffer vergrößern
```

### 2. readFast() verwenden

```cpp
float angle, distance;
while (lidar.readFast(angle, distance)) {
    // Keine Verzögerungen hier!
    buffer[index++] = {angle, distance};

    // Batch-Verarbeitung
    if (index >= BUFFER_SIZE) {
        processBatch(buffer, index);
        index = 0;
    }
}
```

### 3. Batch-Verarbeitung für UDP

```cpp
struct ScanPoint {
    float angle;
    float distance;
};

ScanPoint buffer[50];
int bufferIndex = 0;

void loop() {
    if (lidar.readFast(angle, distance)) {
        buffer[bufferIndex++] = {angle, distance};

        if (bufferIndex >= 50) {
            // Sende alle 50 Punkte auf einmal
            udp.write((uint8_t*)buffer, sizeof(buffer));
            bufferIndex = 0;
        }
    }
}
```

### 4. Vermeiden Sie

- ❌ `Serial.print()` in der Loop (sehr langsam!)
- ❌ `delay()` während des Scannens
- ❌ Blockierende Operationen
- ❌ Einzelne UDP-Pakete pro Punkt

## Fehlerbehebung

### Problem: Keine Daten empfangen

**Lösung:**
1. Prüfen Sie die Baud-Rate (muss zum Modell passen!)
2. Prüfen Sie RX/TX Verbindungen (sind sie vertauscht?)
3. Motor muss sich drehen (MOTOR_PWM Anschluss prüfen)
4. Gerät-Gesundheit prüfen: `lidar.getHealth()`

### Problem: Niedrige Datenrate bei S2L

**Lösung:**
1. CPU-Frequenz auf 240 MHz erhöhen
2. `readFast()` statt `readMeasurement()` verwenden
3. Serial Buffer vergrößern: `Serial2.setRxBufferSize(2048)`
4. Batch-Verarbeitung implementieren
5. Keine `Serial.print()` Aufrufe in Loop

### Problem: "Failed to get device info"

**Lösung:**
1. Baud-Rate prüfen
2. RX/TX Pins vertauscht?
3. 5V Stromversorgung ausreichend?
4. `lidar.reset()` versuchen

### Problem: Verbindung bricht ab

**Lösung:**
1. Stromversorgung prüfen (S2L braucht externe 5V!)
2. USB-Kabel Qualität prüfen
3. Längere Timeouts verwenden
4. Serial Buffer vergrößern bei hohen Datenraten

## Leistungsdaten

| Modell | Max. Abtastrate | Reichweite | Baud-Rate |
|--------|----------------|------------|-----------|
| A1M | 2000 Hz | 12 m | 115200 |
| A2M6 | 4000 Hz | 6 m | 115200 |
| A2M8 | 4000 Hz | 8 m | 115200 |
| A3 | 8000 Hz | 25 m | 256000 |
| C1 | 8000 Hz | 12 m | 460800 |
| C3 | 8000 Hz | 12 m | 460800 |
| S2 | 15600 Hz | 30 m | 1000000 |
| S2L | 32000 Hz | 30 m | 1000000 |
| S3 | 16000 Hz | 40 m | 1000000 |

## Lizenz

MIT License - siehe LICENSE Datei

## Credits

- Basiert auf dem [Slamtec RPLidar SDK](https://github.com/Slamtec/rplidar_sdk)
- Optimiert für Arduino und ESP32
- Entwickelt für maximale Performance mit S2L

## Support

Bei Problemen oder Fragen:
1. Prüfen Sie die Beispiele
2. Lesen Sie die Fehlerbehebung
3. Öffnen Sie ein Issue auf GitHub

## Repository klonen

Um die Bibliothek zu klonen und in der Arduino IDE zu verwenden:

```bash
cd ~/Arduino/libraries/
git clone https://github.com/smily77/Lidar.git RPLidar
```

Oder für einen spezifischen Branch:

```bash
git clone -b claude/rplidar-arduino-library-011CUS8fWFpYoAmZxSyL4AXX https://github.com/smily77/Lidar.git RPLidar
```

Danach Arduino IDE neu starten.

## Version

**Version 1.1.0** — Protokoll- und Robustheitsfixes (siehe
[Behobene Probleme](#behobene-probleme-110)).
- Korrektes Node- und Descriptor-Parsing, host-getesteter Parser
- Byte-Stream-Resynchronisierung in `readMeasurement()`
- `begin(Stream&)` bindet nur (RTTI-frei), `startScan()` mit Auto-Recovery
- Express-Scan ehrlich als nicht implementiert markiert
- Verifiziert auf RPLIDAR C1 an ESP32-S3

Version 1.0.0 — Erste Release (vor den oben genannten Fixes).
