Sankhya v4 Carrier Board — Reference Firmware
Minimal, open-source reference firmware demonstrating standalone operation of the
Sankhya v4 RS-485 Modbus sensor carrier board. It powers the sensor terminal,
reads one RS-485 Modbus soil sensor, and prints the values over the native USB-C
serial port.
It is intentionally minimal. It has no cloud connectivity, no authentication, no
proprietary protocol, and no agronomic logic. Its only purpose is to demonstrate
that the certified hardware fulfils its essential function — reading an RS-485
Modbus sensor — with no dependency on Sankhya's proprietary platform.
> **Scope.** This is **not** the production firmware that powers the Sankhya Farms
> platform. The production firmware and the cloud backend are separate proprietary
> products and are **outside the scope** of the open-source hardware certification.
> This repository and this reference firmware cover the **v4 carrier PCB** only.
License: Apache-2.0 (see SPDX header in the source file).
---
Hardware required
Sankhya v4 carrier board (ESP32-S3 chip-down, `DOIT ESPS3-32E-N16R8`)
A 12 V source on a barrel jack (LiFePO4 battery or bench supply) — the sensor 12 V
rail is derived from this, so USB-C alone will not power the sensor
USB-C cable to a workstation (power for the MCU + serial output)
One RS-485 Modbus soil sensor at factory defaults (example: ZTS-3002 7-in-1,
slave ID 1, 4800 8N1)
Wiring (TB_1 screw terminal)
Terminal	Signal	Sensor wire (typical)
TB_1 right (pin-1 dot side)	RS-485 A	yellow
TB_1 middle	RS-485 B	blue
TB_1 left	GND / not used in this test	—
U9 +	Switched sensor +12 V	brown
U9 GND	Sensor ground	black
For thin sensor wires, strip ~5–6 mm, twist tightly, and fold the copper back on
itself if the screw-terminal grip is weak.
Pin map (v4 board, as fabricated)
Firmware constant	GPIO	Hardware role
`PIN_RS485_TX`	GPIO43	ESP32 TXD0 → U4 SP3485 DI
`PIN_RS485_RX`	GPIO44	U4 SP3485 RO → ESP32 RXD0
`PIN_RS485_DIR`	GPIO5	U4 SP3485 DE/RE# (tied) — HIGH = transmit
`PIN_SENSOR_POWER`	GPIO18	U7 TPS1H100B high-side switch — HIGH = sensor 12 V on
`PIN_LED`	GPIO16	Status LED via R12
(USB CDC)	GPIO19 / GPIO20	Native USB-C serial/logging (frees GPIO43/44 for RS-485)
INA219 monitoring (GPIO11 SDA / GPIO12 SCL) is present on the board but not used by
this reference firmware.
Build & flash
Arduino IDE 2.x with the ESP32 board support package:
Board: ESP32S3 Dev Module
USB CDC On Boot: Enabled (logging goes over native USB, leaving GPIO43/44 free)
To use the Arduino IDE, rename the source to `.ino` (or open the folder with
PlatformIO).
Connect over USB-C, hold BOOT during connect to enter download mode, and upload.
Expected serial output
```
Sankhya v4 - reference firmware (Apache 2.0)
Powering sensor terminal and waiting for warm-up...
Polling Modbus slave ID 1 at 4800 baud, function 0x03, regs 0x0 length 4.

Moisture:    95.2 %
Temperature: 17.5 C
EC:          900 uS/cm
pH:          5.8
```
Using a different sensor
The constants `SENSOR_ID`, `SENSOR_BAUD`, `SENSOR_START_REG`, and `SENSOR_READ_LEN`
near the top of the source match a factory-default ZTS-3002. Change them for another
sensor — its register map, scaling, and default address come from that sensor
manufacturer's datasheet, not from this repository.
Protocol note
ZTS / JXCT-family RS-485 soil sensors sometimes return Modbus RTU frames preceded by
a few leading bytes (a bus turnaround / line-state artifact, not a board fault). This
firmware therefore scans the receive buffer for a valid frame — correct address,
function code, byte count, and CRC — rather than assuming the frame starts at byte 0.
Files
`sankhya_v4.cpp` — the reference firmware (Apache-2.0)
