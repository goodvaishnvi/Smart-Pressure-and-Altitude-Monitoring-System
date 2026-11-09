# Smart Pressure & Altitude Monitoring System

## 📌 Project Overview

The **Smart Pressure & Altitude Monitoring System** is an embedded system designed to continuously monitor atmospheric pressure, temperature, and altitude using the BMP280 pressure sensor and MSPM0+ microcontroller.

The system uses an OLED display to show the measured values and implements low-power monitoring to reduce energy consumption. The OLED automatically turns OFF during normal monitoring and turns ON when a significant change in pressure, temperature, or altitude is detected.

This project is suitable for industrial process monitoring, portable instrumentation, environmental monitoring, and IoT-based applications.

## 🎯 Objectives

- Measure atmospheric pressure using the BMP280 sensor.
- Monitor temperature and calculate altitude.
- Display real-time readings on an OLED display.
- Implement low-power operation using the MSPM0+ microcontroller.
- Detect significant changes in environmental parameters.
- Reduce power consumption by controlling the OLED display.

## ⚙️ Hardware Components

| Component | Purpose |
|---|---|
| MSPM0+ Microcontroller | Main processing unit |
| BMP280 Sensor | Pressure and temperature measurement |
| OLED Display | Displays pressure, temperature, and altitude |
| Connecting Wires | Hardware connections |
| Power Supply | Supplies power to the system |

## 🔌 Communication and Pin Connections

The BMP280 and OLED communicate with the MSPM0+ through the I²C communication protocol.

| Signal | MSPM0+ Connection |
|---|---|
| SCL | PB2 |
| SDA | PB3 |
| VCC | Pin 1 |
| GND | Pin 20 |

**I²C Device Addresses:**
- BMP280: 0x76 or 0x77
- OLED: 0x3C

## 🧠 Working Principle

1. **System Initialization:** Initialize the MSPM0+ microcontroller, GPIO, I²C, and OLED display.
2. **Sensor Detection:** Detect the BMP280 sensor using its I²C address.
3. **Sensor Configuration:** Read calibration data and configure the BMP280.
4. **Data Acquisition:** Measure temperature and atmospheric pressure.
5. **Altitude Calculation:** Calculate altitude from atmospheric pressure.
6. **Display Output:** Display pressure, temperature, and altitude on the OLED.
7. **Low-Power Mode:** Turn OFF the OLED and put the microcontroller into a low-power waiting state using `__WFI()`.
8. **Change Detection:** Wake up approximately every second and compare new readings with previous reference values.
9. **Display Update:** If a significant change is detected, turn ON the OLED and display the updated readings.

## 📊 Change Detection Thresholds

The system updates the OLED when any of the following changes are detected:

| Parameter | Change Threshold |
|---|---:|
| Temperature | ≥ 10°C |
| Pressure | ≥ 10 hPa |
| Altitude | ≥ 20 m |

The OLED remains ON for approximately 10 seconds after an update and then turns OFF again.

## 📐 Altitude Calculation

Altitude is calculated using the barometric formula:

\[
Altitude = 44330 \times
\left(1-\left(\frac{P}{P_0}\right)^{0.1903}\right)
\]

Where:

- P = Measured atmospheric pressure (hPa)
- P₀ = Reference sea-level pressure (1013.25 hPa)
- Altitude = Estimated altitude in meters

## 💻 Technologies Used

- Embedded C
- MSPM0+ Microcontroller
- BMP280 Pressure Sensor
- I²C Communication Protocol
- OLED Display
- Low-Power Embedded Programming

## 🌍 Applications

- Weather monitoring
- Industrial pressure monitoring
- Altitude measurement
- Drones and robotics
- Indoor navigation
- Environmental monitoring
- Portable instrumentation
- HVAC systems

## ⭐ Key Features

- Real-time pressure and temperature monitoring.
- Altitude estimation using atmospheric pressure.
- OLED-based data visualization.
- Automatic display ON/OFF control.
- Significant environmental change detection.
- Low-power monitoring using MSPM0+.

## 🚀 Future Scope

- Add wireless data transmission using Wi-Fi or Bluetooth.
- Develop a cloud-based monitoring dashboard.
- Store pressure and altitude readings for historical analysis.
- Add mobile notifications for significant pressure changes.
- Optimize the system for battery-powered portable applications.

## 👨‍💻 Project Information

**Project Title:** Smart Pressure & Altitude Monitoring System

**Platform:** MSPM0+ Microcontroller

**Sensor:** BMP280

**Programming Language:** Embedded C

**Communication:** I²C

**Project Domain:** Embedded Systems and Low-Power Instrumentation

---

**Developed as part of the EnerGenius – Ultra-Low-Power Embedded System Development Using MSPM0+ initiative.**


