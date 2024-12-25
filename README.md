# Neonatal Incubator Prototype

This repository contains the design, implementation, and testing of a neonatal incubator prototype developed as part of an academic project for the **CBM 426 - Integration Workshop** at the **Universidad de Valparaíso, Faculty of Engineering, School of Biomedical Engineering**. The primary objective of the project was to create a controlled thermal environment for neonates, integrating hardware and software for precise temperature regulation, data monitoring, and user interaction.

## Table of Contents
- [Introduction](#introduction)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Installation](#installation)
- [Usage](#usage)
- [Acknowledgments](#acknowledgments)

---

## Introduction

The neonatal incubator prototype provides an automatic temperature control system that ensures a stable and safe environment for neonates. The system includes:
- A **12V lamp** as the heat source.
- A **12V fan** for air circulation.
- An **NTC temperature sensor** for monitoring temperature.
- An **Arduino-based control system** using PID algorithms.
- A **Python-based graphical interface** for real-time monitoring and control.

## Features

- **Automatic and Manual Modes**: Control temperature and fan speed manually or via an automatic PID control.
- **Real-Time Monitoring**: View temperature trends, fan speed, and power levels through a PyQt6-based interface.
- **Data Recording**: Log environmental parameters and system performance to a CSV file or MySQL database.
- **Alarm System**: LED and buzzer notifications for critical temperature deviations or hardware issues.

## System Architecture

### Hardware Components
- **Arduino Uno**: Microcontroller for sensor readings and actuator control.
- **NTC Thermistor**: Temperature sensing.
- **12V Incandescent Lamp**: Heat source.
- **12V Fan**: Ensures uniform air distribution.
- **LED Indicators**: Visual alarm system for status notifications.

### Software
1. **Embedded Code** (C++/Arduino):
   - Implements PID control for stable temperature.
   - Controls lamp power and fan speed using PWM.
   - Sends real-time data to the Python application via serial communication.

2. **Python Application**:
   - Graphical interface using PyQt6 for system monitoring and manual control.
   - Data visualization with pyqtgraph.
   - Logging and database integration for performance analysis.

### Circuit Design
The system incorporates transistors and optocouplers for safe and efficient control of actuators. The detailed circuit diagram is available in the project documentation.

## Installation

### Hardware Setup
1. Assemble the components as per the provided schematic.
2. Upload the Arduino code (`main.cpp`) to the microcontroller using the Arduino IDE.
3. Connect the Arduino to the host PC via USB.

### Software Setup
1. Install Python 3.10 or higher.
2. Install required Python libraries:
   ```bash
   pip install pyqt6 pyqtgraph numpy mysql-connector-python
   ```
3. Clone this repository:
   ```bash
   git clone https://github.com/MaximilianoAntonio/incubator-prototype.git
   cd incubator-prototype
   ```
4. Open and configure `Plataforma.py`:
   - Update the serial port (default is `COM8`).
   - Configure MySQL connection credentials.

## Usage

1. **Run the Python Application**:
   ```bash
   python Plataforma.py
   ```
2. Use the graphical interface to:
   - Adjust light and fan settings.
   - Monitor real-time data.
   - Log environmental parameters.

3. **Arduino Control**:
   - Switch between automatic and manual modes using the interface.
   - Monitor serial output for debugging.

## Documentation

For a detailed explanation of the system design, implementation, and experimental results, refer to the **[Reporte Final Incubadora.pdf](./Reporte%20Final%20Incubadora.pdf)** included in this repository. This document covers:
- Hardware design and circuit schematics.
- Code functionality for Arduino and Python.
- Performance tests and results.
- Mathematical modeling and system analysis.
