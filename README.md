# 🍳 DodZero - Smart Gas Tank Monitor (IoT)

This project aims to solve a simple yet annoying problem: running out of gas in the middle of cooking.
The system utilizes an **ESP32-C3 Super Mini** controller and a mechanical pressure sensor to monitor the gas tank status in real-time and alert you when it empties.

## 📌 How It Works
The system is based on a **Mechanical Gas Pressure Switch** connected to the gas line.
- **Gas Present (High Pressure):** The electrical circuit is OPEN.
- **No Gas (Low Pressure):** The electrical circuit is CLOSED.

The ESP32 reads this state and triggers alerts accordingly.

## 🛠️ Hardware & Connections
* **Controller:** ESP32-C3 Super Mini
* **Sensor:** Mechanical Gas Pressure Switch (NC/NO)
* **Status LED:** Built-in or External

| Component | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| Sensor (Leg 1) | GND | Common Ground |
| Sensor (Leg 2) | GPIO 10 | Configured as INPUT_PULLUP |
| Status LED | GPIO 8 | Lights up on alert |

## 📂 Project Structure
The code is organized modularly for easy maintenance and readability:
* `src/main.cpp`: Main logic - Loop management, sensor reading, and decision making.
* `src/utils.cpp`: Helper functions implementation - Network scanning, WiFi connection, and reconnection logic.
* `include/utils.h`: Header file - Constant definitions (PINs), function declarations, and logic.

## ✅ Task List

| Task | Status | Owner | Notes |
| :--- | :--- | :--- | :--- |
| **Basic Sensor Connection** | ✅ Done | TBD | Digital Read and LED control |
| **WiFi Connection** | ✅ Done | TBD | Stable connection with Auto-Reconnect |
| **Network Scanning** | ✅ Done | TBD | Diagnostic function for RSSI scanning |
| **Modular Code Structure** | ✅ Done | TBD | Split into Header and Source files |
| **Sending Alerts (WhatsApp/Email)** | 🚧 In Progress | TBD | API integration for messaging |
| **3D Printed Case** | ⏳ Future | TBD | Case design for ESP and battery |
| **Power Management** | ⏳ Future | TBD | Deep Sleep implementation for battery saving |

---
**Developed by MagmaTeam | 2026**