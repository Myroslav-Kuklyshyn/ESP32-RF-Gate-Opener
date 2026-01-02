# 🚪 Smart Gate IoT Controller

**Turn your smartphone into a universal gate remote!** 📱🔓
This project bridges the gap between old-school 433MHz gates and modern IoT. It clones your existing remote signals and allows you to trigger them from anywhere in the world via a secure Web Dashboard.

## 🔥 Key Features
* **Signal Cloning:** "Learning Mode" captures and saves 433MHz signals (Static Code) to ESP32 memory.
* **Global Control:** Open your gate from anywhere using Firebase Realtime Database.
* **Web App:** A clean, responsive interface for easy control.
* **Hardware Core:** Powered by **ESP32** + **CC1101** (High-precision RF module).

## 🛠️ Tech Stack
* **Hardware:** ESP32, CC1101 Transceiver, Push Button, LED.
* **Firmware:** C++ (Arduino IDE), `RCSwitch`, `ELECHOUSE_CC1101`.
* **Cloud & Web:** Firebase Realtime Database, HTML5, CSS3, JavaScript.

## ⚙️ Quick Setup

### 1. Hardware Wiring
Connect CC1101 to ESP32:
* `SCK` → GPIO 18 | `MISO` → GPIO 19 | `MOSI` → GPIO 23
* `CSN` → GPIO 5  | `GDO0` → GPIO 4
* *Button:* GPIO 27 | *LED:* GPIO 2

### 2. Firmware (ESP32)
1.  Navigate to `/firmware` folder.
2.  Open `GateController.ino`.
3.  **Edit Config:** Fill in your `WIFI_SSID`, `PASSWORD`, and `FIREBASE_API_KEY`.
4.  Upload to board.

### 3. Web App
1.  Navigate to `/web-app` folder.
2.  Open `index.html`.
3.  **Edit Config:** Paste your Firebase Web Config (API Key, Project ID) into the script section.
4.  Run locally or host on GitHub Pages.

## 🎮 How to Use
1.  **Teach:** Hold the physical button (3s) → Press your old remote. The LED flashes (Signal Saved!).
2.  **Control:** Open the Web App → Click **OPEN**. The gate opens via Wi-Fi!

---
*Disclaimer: For educational use only. Respect local radio frequency regulations.*
