// SPDX-License-Identifier: Apache-2.0
//
// Sankhya v4 Carrier Board — Reference Firmware
// Copyright (c) 2026 Sankhya Ventures LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ---------------------------------------------------------------------------
// PURPOSE
//   This is a minimal open-source reference firmware that demonstrates
//   standalone operation of the Sankhya v4 RS-485 Modbus sensor carrier
//   board. It powers the sensor terminal, reads one Modbus soil sensor
//   every few seconds, and prints values over the native USB-C serial
//   port.
//
//   It is intentionally minimal: it has no cloud connectivity, no
//   authentication, no proprietary protocol, and no agronomic logic. It
//   exists to demonstrate that the certified hardware fulfils its
//   essential function — reading an RS-485 Modbus sensor — without any
//   dependency on Sankhya's proprietary platform firmware or cloud
//   backend.
//
//   This is NOT the production firmware that powers the Sankhya Farms
//   platform. The production firmware and cloud backend are separate
//   proprietary products and are outside the scope of the open-hardware
//   certification.
//
// HARDWARE REQUIRED
//   • Sankhya v4 carrier board (ESP32-S3 chip-down)
//   • 12 V power source (LiFePO4 battery on JST or external supply)
//   • USB-C cable to a workstation for serial output
//   • One RS-485 Modbus soil sensor (factory defaults; e.g. ZTS-3002
//     7-in-1 at slave ID 1, 4800 8N1), wired to TB_1:
//       Sensor A (yellow)  → TB_1 right
//       Sensor B (blue)    → TB_1 middle
//       Sensor +12 V       → U9 +
//       Sensor GND         → U9 GND
//
// CONFIGURATION
//   The constants in the SENSOR section below match a factory-default
//   ZTS-3002. Change SENSOR_ID, SENSOR_BAUD, SENSOR_START_REG, and
//   SENSOR_READ_LEN if you are using a different sensor — its register
//   map and defaults come from the sensor manufacturer's datasheet.
//
// BUILD
//   Arduino IDE 2.x with the ESP32 board support package.
//   Board:               "ESP32S3 Dev Module"
//   USB CDC On Boot:     ENABLED
//   Upload via USB-C with the BOOT button held during connect.
//
// PROTOCOL NOTE
//   ZTS / JXCT-family RS-485 soil sensors sometimes return Modbus RTU
//   frames preceded by variable leading bytes — a turnaround / line-
//   state artifact, not a board fault. This firmware therefore scans
//   the RX buffer for a valid frame (correct address + function + byte
//   count + CRC) rather than assuming the frame begins at byte 0.
// ---------------------------------------------------------------------------

#include <Arduino.h>

// ── Pin map (matches the v4 board as fabricated) ─────────────────────────────
constexpr int PIN_LED          = 16;
constexpr int PIN_SENSOR_POWER = 18;   // U7 TPS1H100B high-side switch — HIGH = sensor 12 V on
constexpr int PIN_RS485_DIR    =  5;   // U4 SP3485 DE/RE# tied         — HIGH = transmit
constexpr int PIN_RS485_TX     = 43;   // U4 SP3485 DI
constexpr int PIN_RS485_RX     = 44;   // U4 SP3485 RO

// ── Sensor configuration ─────────────────────────────────────────────────────
constexpr uint8_t  SENSOR_ID            = 1;       // Modbus slave ID
constexpr uint32_t SENSOR_BAUD          = 4800;    // 4800 8N1 (ZTS-3002 factory default)
constexpr uint16_t SENSOR_START_REG     = 0x0000;  // First holding register
constexpr uint16_t SENSOR_READ_LEN      = 4;       // moisture, temperature, EC, pH

// ── Timing ───────────────────────────────────────────────────────────────────
constexpr uint32_t READ_INTERVAL_MS     = 5000;    // Time between polls
constexpr uint16_t RS485_TURNAROUND_US  = 300;     // Tx-to-Rx settle time
constexpr uint16_t RX_TIMEOUT_MS        = 1800;
constexpr uint16_t RX_SILENCE_MS        = 80;

HardwareSerial RS485Serial(1);

// ─────────────────────────────────────────────────────────────────────────────
// Modbus RTU CRC-16 (polynomial 0xA001, initial value 0xFFFF)
// ─────────────────────────────────────────────────────────────────────────────
static uint16_t modbusCRC(const uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; }
            else              { crc >>= 1; }
        }
    }
    return crc;
}

// ─────────────────────────────────────────────────────────────────────────────
// RS-485 direction control
// ─────────────────────────────────────────────────────────────────────────────
static inline void rs485TxMode() {
    digitalWrite(PIN_RS485_DIR, HIGH);
    delayMicroseconds(RS485_TURNAROUND_US);
}

static inline void rs485RxMode() {
    digitalWrite(PIN_RS485_DIR, LOW);
    delayMicroseconds(200);
}

// ─────────────────────────────────────────────────────────────────────────────
// Collect bytes from the RX line until a short silence terminates the frame
// ─────────────────────────────────────────────────────────────────────────────
static int rs485Collect(uint8_t *buf, int maxLen) {
    int idx = 0;
    unsigned long start    = millis();
    unsigned long lastByte = 0;
    while (millis() - start < RX_TIMEOUT_MS) {
        while (RS485Serial.available() && idx < maxLen) {
            buf[idx++] = RS485Serial.read();
            lastByte = millis();
        }
        if (idx > 0 && (millis() - lastByte) > RX_SILENCE_MS) break;
        delay(1);
    }
    return idx;
}

// ─────────────────────────────────────────────────────────────────────────────
// Scan the RX buffer for a valid Modbus read response and extract registers
// ─────────────────────────────────────────────────────────────────────────────
static bool findReadFrame(const uint8_t *buf, int len,
                          uint8_t id, uint8_t fn, uint16_t regCount,
                          uint16_t *outRegs) {
    const int frameLen = 5 + 2 * regCount;
    const uint8_t expectedByteCount = regCount * 2;
    for (int off = 0; off + frameLen <= len; off++) {
        if (buf[off]     != id)                continue;
        if (buf[off + 1] != fn)                continue;
        if (buf[off + 2] != expectedByteCount) continue;
        const uint16_t got  = ((uint16_t)buf[off + frameLen - 1] << 8)
                            |  (uint16_t)buf[off + frameLen - 2];
        const uint16_t calc = modbusCRC(buf + off, frameLen - 2);
        if (got != calc) continue;
        for (int i = 0; i < regCount; i++) {
            outRegs[i] = ((uint16_t)buf[off + 3 + 2 * i] << 8)
                       |  (uint16_t)buf[off + 4 + 2 * i];
        }
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Send Modbus function 0x03 (read holding registers) and decode the reply
// ─────────────────────────────────────────────────────────────────────────────
static bool readHoldingRegisters(uint8_t id, uint16_t startReg,
                                 uint16_t regCount, uint16_t *outRegs) {
    uint8_t req[8];
    req[0] = id;
    req[1] = 0x03;
    req[2] = (startReg >> 8) & 0xFF;
    req[3] =  startReg       & 0xFF;
    req[4] = (regCount >> 8) & 0xFF;
    req[5] =  regCount       & 0xFF;
    const uint16_t crc = modbusCRC(req, 6);
    req[6] =  crc       & 0xFF;
    req[7] = (crc >> 8) & 0xFF;

    rs485TxMode();
    RS485Serial.write(req, 8);
    RS485Serial.flush();
    rs485RxMode();

    uint8_t resp[64];
    const int n = rs485Collect(resp, sizeof(resp));
    if (n == 0) return false;
    return findReadFrame(resp, n, id, 0x03, regCount, outRegs);
}

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    pinMode(PIN_LED,          OUTPUT);
    pinMode(PIN_SENSOR_POWER, OUTPUT);
    pinMode(PIN_RS485_DIR,    OUTPUT);
    digitalWrite(PIN_LED,          LOW);
    digitalWrite(PIN_SENSOR_POWER, LOW);
    digitalWrite(PIN_RS485_DIR,    LOW);

    Serial.begin(115200);
    delay(2500);  // Wait for USB CDC enumeration

    Serial.println();
    Serial.println("Sankhya v4 - reference firmware (Apache 2.0)");
    Serial.println("Powering sensor terminal and waiting for warm-up...");

    digitalWrite(PIN_SENSOR_POWER, HIGH);
    digitalWrite(PIN_LED,          HIGH);
    delay(5000);  // Sensor warm-up

    RS485Serial.begin(SENSOR_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
    delay(50);
    while (RS485Serial.available()) RS485Serial.read();
    rs485RxMode();

    Serial.print("Polling Modbus slave ID ");
    Serial.print(SENSOR_ID);
    Serial.print(" at ");
    Serial.print(SENSOR_BAUD);
    Serial.print(" baud, function 0x03, regs 0x");
    Serial.print(SENSOR_START_REG, HEX);
    Serial.print(" length ");
    Serial.print(SENSOR_READ_LEN);
    Serial.println('.');
    Serial.println();
}

void loop() {
    uint16_t regs[8];
    const bool ok = readHoldingRegisters(SENSOR_ID, SENSOR_START_REG,
                                         SENSOR_READ_LEN, regs);

    if (ok) {
        // Scaling below matches the ZTS-3002 register map published by the
        // sensor manufacturer. Adjust for a different sensor.
        const uint16_t moisture    = regs[0];
        const int16_t  temperature = (int16_t)regs[1];
        const uint16_t ec          = regs[2];
        const uint16_t ph          = regs[3];

        Serial.print("Moisture:    "); Serial.print(moisture    / 10.0f, 1); Serial.println(" %");
        Serial.print("Temperature: "); Serial.print(temperature / 10.0f, 1); Serial.println(" C");
        Serial.print("EC:          "); Serial.print(ec);                     Serial.println(" uS/cm");
        Serial.print("pH:          "); Serial.println(ph         / 10.0f, 1);
        Serial.println();
    } else {
        Serial.println("No valid Modbus response.");
        Serial.println();
    }

    delay(READ_INTERVAL_MS);
}
