# sankhya-node-hardware

Open hardware for [Sankhya Intelligence](https://sankhyafarms.com) — solar-powered ESP32-S3 sensor nodes for per-tree precision agriculture.

RS-485/Modbus soil sensors, LiFePO4 power, 4G connectivity. Gerbers, BOM, and schematics for JLCPCB fabrication. **Intelligence layer is proprietary; hardware is yours to build.**

---

## What this is

This repository contains the design files for the Sankhya sensor node — a solar-powered, battery-backed ESP32-S3 carrier board that polls RS-485/Modbus soil sensors and reports to the Sankhya Intelligence backend over WiFi (via a local 4G USB dongle or LAN).

It is deployed in production at a 2,100-tree Alphonso mango orchard in Patdi, Gujarat. One node per tree, polling soil EC, moisture, temperature, pH, and NPK on the nocturnal irrigation cycle.

## What's in this repository
