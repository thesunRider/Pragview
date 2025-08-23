# Pragview

**Pragview** is a simple yet powerful digital photo frame built on the **ESP32** platform. It features **SD card storage**, **Wi-Fi connectivity**, **OTA updates**, multiple **sensor integrations**, and a user-friendly **2.8" touchscreen display**. Pragview is designed to be lightweight yet feature-rich, making it easy to upload, display, and manage your photo memories.

---

## Features

### 🖼️ Display & Storage
- **2.8" Touchscreen display** for photo viewing and settings navigation.
- **SD card support** for storing images locally (up to 16GB supported).
- **Portrait and landscape display** with automatic rotation via tilt sensor.
- **Photo change interval** and **display timeout** configuration.

### 📡 Connectivity
- **Wi-Fi support** for remote control and updates.
- **FTP server** for uploading photos wirelessly:  
  `ftp://pragview:1234@192.168.54.1:21/`
- **Remote OTA firmware updates**.
- Future plans for **Google Photos sync** and **Wi-Fi dashboard**.

### 🔋 Power Management
- **TP4056 charging module** with charge/discharge protection.
- **Charging status indicators** and battery voltage detection.
- **Switch for power on/off control**.

### ⏱️ Clock & Sensors
- **Real-Time Clock (RTC)** for accurate timekeeping.
- **Light sensor (LDR)** for brightness control and dark mode support.
- **Tilt sensor** for auto screen orientation.
- **Motion sensors** for screen wake-up or interactions.

### ⚙️ Device Configuration
- **Menu options** include:
  - Network Settings (Wi-Fi On/Off, FTP On/Off)
  - Clock Settings (Display, Sync, Timezone)
  - Device Settings (Photo intervals, Light sensor options, Tilt sensor settings, Storage clear)
  - Cloud Photos Settings (SD or Google Photos album selection)
  - Firmware Update & About section  

---

## Default Configuration

- **Default Wi-Fi**: `Pragview`  
- **Config File**: `config.ini`  
- **FTP String**: `ftp://pragview:1234@192.168.54.1:21/`

---

## PC Script

A simple **PC script** is provided for uploading files to the SD card over FTP.  

---

## To-Do / Future Improvements

- [ ] **Better RAM management** for smoother performance.
- [ ] **ESP32 Wi-Fi dashboard** for SD card upload instead of FTP.
- [ ] **Google Photos sync integration** for cloud albums.
- [ ] **Wi-Fi syncing options** for photo updates.
- [ ] Improved **UI/UX for touchscreen interface**.

---

## Hardware Summary

| Component            | Description                                       |
|----------------------|---------------------------------------------------|
| Microcontroller       | ESP32 (4MB Flash, 500KB RAM)                      |
| Display               | 2.8" Touchscreen LCD                              |
| Storage               | SD Card (10GB/16GB Supported)                     |
| Charging Circuit       | TP4056 Module with protection                     |
| Sensors               | Motion sensors, Tilt sensor, LDR light sensor     |
| Power Indicators       | Charging status LEDs                             |
| Connectivity          | Wi-Fi, FTP server, OTA updates                    |

---

## Menu Structure

Menu:
Network Settings:
- Connected/Disconnected
- Wi-Fi: On/Off
- FTP Server: On/Off
- FTP Details: ftp://pragview:1234@192.168.54.1:21/
- Connect to Pragview
- IP Address: 192.168.54.1

Clock Settings:
- Display Clock
- Sync Clock
- Set Timezone (CET)

Device Settings:
- Set Photo Change Interval
- Display Timeout
- Enable Light Sensor
- Set Dark Light Intensity
- Enable Tilt Sensor
- Display Portrait/Landscape on Tilt
- Clear Storage
- Storage Capacity: 10GB/16GB Used

Cloud Photos Settings:
- Select Photo Source: SD / Google Photos
- Current GPhotos ID:
- Current GPhotos Album Selected:

Firmware Update
About

---

## How to Upload Photos

1. Connect Pragview to Wi-Fi.  
2. Use the FTP details to upload images:  
   `ftp://pragview:1234@192.168.54.1:21/`  
Or use the provided PC script for automated upload. 
1. Turn on ftp in device
2. Copy JPEG image files to code/win/src
3. Run code/win/main.bat 
4. Enter Ip address of device 

---

## License

This project is open-source under the **MIT License**.  

---

## Roadmap

Pragview aims to become a **fully connected smart photo frame** with seamless **cloud integration**, **Wi-Fi dashboard**, and **intelligent power management**.

---