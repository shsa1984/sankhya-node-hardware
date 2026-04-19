# sankhya-node-hardware

Open hardware for [Sankhya Intelligence](https://sankhyafarms.com) — solar-powered ESP32-S3 sensor nodes for per-tree precision agriculture.

RS-485/Modbus soil sensors, LiFePO4 power, 4G connectivity. Gerbers, BOM, and schematics for JLCPCB fabrication. **Intelligence layer is proprietary; hardware is yours to build.**

---

## What this is

This repository contains the design files for the Sankhya sensor node — a solar-powered, battery-backed ESP32-S3 carrier board that polls RS-485/Modbus soil sensors and reports to the Sankhya Intelligence backend over WiFi (via a local 4G USB dongle or LAN).

It is deployed in production at a 2,100-tree Alphonso mango orchard in Patdi, Gujarat. One node per tree, polling soil EC, moisture, temperature, pH, and NPK on the nocturnal irrigation cycle.

## What's in this repository
Production design files for the Sankhya node (current revision, v3):

- **`Gerbers.zip`** — PCB manufacturing files, upload directly to JLCPCB
- **`BOM.csv`** — Bill of materials for SMT assembly
- **`PickAndPlace.csv`** — Component placement file for SMT assembly
- **`Schematic.jpg`** through **`Schematic4.jpg`** — Schematic sheets (pages 1–4)

To fabricate: upload `Gerbers.zip`, `BOM.csv`, and `PickAndPlace.csv` to JLCPCB. Select standard 2-layer FR-4, 1.6mm, HASL finish.

## Hardware overview

- **MCU:** ESP32-S3-Nano (WiFi, dual-core, USB-native)
- **RS-485 transceiver:** MAX485 half-duplex, auto-direction
- **Power:** 20W solar panel → LiFePO4 3.2V pack with integrated BMS → LM2596S buck to 5V/3.3V rails
- **Sensor interface:** Screw terminals for RS-485 A/B + 12V sensor power
- **Enclosure:** IP65-rated, pole-mount or tree-strap
- **Connectivity:** WiFi to local 4G USB dongle or orchard-wide TP-Link extender mesh

## Fabrication

The boards are designed for direct JLCPCB assembly:

1. Upload `Gerbers.zip` to JLCPCB
2. Select standard 2-layer FR-4, 1.6mm, HASL finish
3. For assembled boards, upload `BOM.csv` and `PickAndPlace.csv` for SMT assembly
4. Through-hole components (terminal blocks, headers) hand-soldered after receipt

Typical cost: ~$5-8/board in small quantities, ~$3/board at 100+ units.

## Compatible sensors

Any RS-485/Modbus RTU soil sensor at 4800 or 9600 baud. Tested families:

- **ZTS/SN series** (Shandong Renke) — 4800 baud, widely available
- **JXCT/JXBS series** — 9600 baud, lower cost
- **DFRobot SEN0601** — laboratory-grade

The firmware handles register-map differences per sensor family. See the [Sankhya sensor library](https://sankhyafarms.com/sensors) for the current supported list.

## Firmware

Firmware is **not** included in this repository. Nodes ship blank and are flashed via the Web Serial flasher at [flash.sankhyafarms.com](https://flash.sankhyafarms.com) after device registration. This pairs each board to a customer account and the Sankhya Intelligence backend.

## Positioning

We open-source the hardware because hardware is the acquisition layer, not the moat. The value is in the longitudinal per-tree dataset, the uptake index, and the AI-driven agronomic reasoning — all of which live behind the backend API.

If you want to fabricate these boards for your own orchard, research project, or commercial deployment: go ahead. The license permits it. If you want them connected to Sankhya Intelligence, sign up at [sankhyafarms.com](https://sankhyafarms.com).

## License

Hardware designs are licensed under **CERN Open Hardware License v2 - Permissive (CERN-OHL-P v2)**. See `LICENSE` for the full text.

You may fabricate, modify, and redistribute these designs, including commercially, subject to the license terms. Attribution is appreciated but not required.

## Contact

- Project: [sankhyafarms.com](https://sankhyafarms.com)
-- Technical: [sankhyafarms.com/technical](https://sankhyafarms.com/technical)
- Email: shaunak@sayta.com

---

*Sankhya Ventures LLC · Delaware, USA*
