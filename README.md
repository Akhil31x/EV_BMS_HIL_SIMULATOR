# EV_BMS_HIL_SIMULATOR
# Automotive BMS Hardware-in-the-Loop Simulator Using ESP32-S3

## Overview

This project implements a Battery Management System (BMS) Hardware-in-the-Loop (HIL) simulator using the Heltec WiFi 32 V3 (ESP32-S3).

The system emulates thermal behavior, battery load conditions, dynamic power derating, fault injection, and safety-critical monitoring typically found in electric vehicle battery management systems.

The project demonstrates real-time thermal protection strategies using a dual-core ESP32 architecture and an OLED-based diagnostic dashboard.

## Features

* Dual-core thermal load simulation
* Real-time temperature monitoring
* Multi-stage thermal derating
* Dynamic power limitation
* Peak temperature tracking
* Runtime monitoring
* Event logging
* Battery fault injection
* Safety isolation demonstration
* OLED diagnostic dashboard
* FreeRTOS task-based architecture

## Hardware

* Heltec WiFi 32 V3 (ESP32-S3)
* 0.96" SSD1306 OLED Display
* USB Power Supply

## Software

* Arduino IDE
* ESP32 Core 3.3.8
* FreeRTOS
* Adafruit SSD1306 Library
* Adafruit GFX Library

## System Architecture

Battery Load Simulation Tasks
↓
Thermal Monitoring
↓
State Machine
↓
Power Derating Logic
↓
Fault Detection
↓
OLED Dashboard

## Thermal States

SAFE

* Full Power

DERATE1

* 90% Power

DERATE2

* 70% Power

DERATE3

* 50% Power

DERATE4

* 25% Power

CRITICAL

* Emergency Shutdown

## Fault Injection

Pressing the PRG button toggles battery fault mode.

During fault mode:

* Battery simulation pauses
* BMS monitoring remains active
* OLED remains operational
* Thermal telemetry continues

This demonstrates safety-critical subsystem isolation.

## Results

The prototype successfully demonstrated:

* Thermal rise under computational load
* Dynamic power derating
* Real-time state transitions
* Event logging
* Fault injection and recovery
* Stable dual-core operation

## Future Work

* CAN Bus Integration
* External Temperature Sensors
* Data Logging to SD Card
* LoRa Telemetry
* Web Dashboard
* Cloud Analytics

