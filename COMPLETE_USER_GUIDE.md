# MCU Environmental Data Logger - Complete User Guide

## Introduction

This comprehensive user guide provides detailed instructions for setting up, configuring, operating, and maintaining the MCU Environmental Data Logger project. This ESP32-based environmental monitoring node measures temperature, humidity, barometric pressure, and air quality (TVOC/eCO2), logs data to an SD card, and transmits readings via Wi-Fi to an MQTT broker for home automation integration.

## Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Quick Start](#quick-start)
4. [Hardware Requirements](#hardware-requirements)
5. [Wiring Guide](#wiring-guide)
6. [Software Setup](#software-setup)
7. [Configuration](#configuration)
8. [Operation](#operation)
9. [Power Consumption and Battery Life](#power-consumption-and-battery-life)
10. [Customization Options](#customization-options)
11. [Troubleshooting](#troubleshooting)
12. [Maintenance](#maintenance)
13. [Example Applications](#example-applications)
14. [Integration with Home Assistant](#integration-with-home-assistant)
15. [License](#license)

## Overview

The MCU Environmental Data Logger is a low-power, wireless environmental monitoring node designed for long-term deployment. It combines multiple sensors with local storage and wireless connectivity to provide comprehensive environmental data logging capabilities.

### Key Features:
- **Multi-sensor sensing**: BME280 (temperature, humidity, pressure) + CCS811 (TVOC, eCO2)
- **Local storage**: Timestamped CSV logging to microSD card (Wi-Fi independent)
- **Wireless connectivity**: Wi-Fi + MQTT for real-time data transmission
- **Power optimization**: Deep sleep between measurements (configurable interval)
- **Remote updates**: OTA (Over-The-Air) firmware updates
- **Visual status**: RGB LED for connection/error indicators (optional)
- **Open source**: Arduino-compatible code with clear documentation
- **Extensible design**: Easy to add additional sensors or features

## Quick Start

For users who want to get started quickly:

### 1. Hardware Setup
- Connect ESP32, BME280, CCS811, and microSD module according to the wiring guide
- Insert a FAT32-formatted microSD card (up to 32GB)
- Optional: Connect WS2812B LED to GPIO27 for status indication

### 2. Software Setup
- Install Arduino IDE or VS Code with PlatformIO
- Install ESP32 board package (Arduino IDE: Boards Manager → search "esp32")
- Install required libraries: Adafruit BME280, Adafruit CCS811, PubSubClient, Adafruit NeoPixel (if using LED)
- Copy `config.example.h` to `config.h`
- Edit `config.h` to set your WiFi SSID/password and MQTT broker details

### 3. Upload and Run
- Connect ESP32 via USB
- Select "ESP32 Dev Module" in Arduino IDE
- Select the correct COM port
- Click Upload
- Monitor serial output (115200 baud) for startup messages
- The CCS811 sensor requires 20 minutes warm-up for accurate readings

### 4. First Measurements
- Device will take measurements every 5 minutes (default)
- Data is logged to SD card as `LOG_YYYYMMDD.CSV`
- If MQTT enabled, data is published to `envlogger/` topics
- Check serial output for connection status and measurements

## Features

- **Multi-sensor sensing**: Measures temperature (°C), humidity (%), pressure (hPa), TVOC (ppb), and eCO2 (ppm)
- **Local storage**: Data logged to microSD card in CSV format with timestamps
- **Wireless connectivity**: Connects to Wi-Fi and publishes data to MQTT broker
- **Power optimization**: Uses ESP32 deep sleep mode to minimize power consumption
- **OTA updates**: Firmware can be updated wirelessly after initial USB setup
- **Status indication**: Optional RGB LED shows system status (Wi-Fi, MQTT, errors)
- **Configurable intervals**: Measurement interval adjustable from 1 minute to 1 hour
- **Robust logging**: Creates new log file daily (LOG_YYYYMMDD.CSV)
- **MQTT retention**: Uses retained MQTT messages so last value is always available
- **Extensible design**: Easy to add additional sensors or features

## Hardware Requirements

- **ESP32 Dev Board** (e.g., ESP32-WROOM-32)
- **BME280 sensor** (I2C interface)
- **CCS811 sensor** (I2C interface)
- **microSD card module** (SPI interface)
- **microSD card** (FAT32 formatted, up to 32GB capacity)
- **Breadboard and jumper wires**
- **Optional**: RGB WS2812B LED for status indication
- **Optional**: 3.7V LiPo battery + solar panel for remote deployment

## Wiring Guide

Connect the components as follows:

| ESP32 GPIO | Function      | BME280 | CCS811 | microSD | WS2812B LED (Optional) |
|------------|---------------|--------|--------|---------|------------------------|
| 3.3V       | Power         | VCC    | VCC    | VCC     | VCC                    |
| GND        | Ground        | GND    | GND    | GND     | GND                    |
| GPIO22     | I2C SCL       | SCL    | SCL    | -       | -                      |
| GPIO21     | I2C SDA       | SDA    | SDA    | -       | -                      |
| GPIO5      | SD CS         | -      | -      | CS      | -                      |
| GPIO18     | SD SCK        | -      | -      | SCK     | -                      |
| GPIO19     | SD MISO       | -      | -      | MISO    | -                      |
| GPIO23     | SD MOSI       | -      | -      | MOSI    | -                      |
| GPIO27     | LED Data      | -      | -      | -       | DI (Data In)           |
| GPIO4      | CCS811 WAK    | -      | WAK    | -       | -                      |

### Important Notes:

1. **GPIO Pin Assignments**: The code uses these specific GPIO pins by default:
   - I2C Sensors: SCL=GPIO22, SDA=GPIO21 (for both BME280 and CCS811)
   - SD Card: CS=GPIO5, SCK=GPIO18, MISO=GPIO19, MOSI=GPIO23
   - Status LED: GPIO27 (optional, requires uncommenting in config.h)
   - CCS811 Wake: GPIO4 (connect to WAK pin, or set to -1 in config.h if not used)

2. **CCS811 WAK Pin**: For always-on operation without manual wake control, connect the WAK pin to GPIO4 through a resistor (or keep connected to GPIO4 as shown). If not connecting WAK to a GPIO, set `CCS811_WAKE_PIN = -1` in config.h.

3. **Status LED**: To enable the optional WS2812B status LED:
   - Connect LED data line to GPIO27 (as shown)
   - Uncomment `#define USE_STATUS_LED 1` in config.h
   - Ensure Adafruit NeoPixel library is installed

4. **Flexibility**: You can change GPIO pin assignments by modifying the values in `config.h` and updating the wiring accordingly.

5. **Power**: All sensors and modules should be powered from the ESP32's 3.3V output.

## Software Setup

### 1. Install Development Environment
1. Install [Arduino IDE](https://www.arduino.cc/en/software) (version 1.8.13 or newer) or [VS Code with PlatformIO](https://platformio.org/)
2. Install ESP32 board package:
   - In Arduino IDE: Files > Preferences > Additional Boards Manager URLs: 
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Then: Tools > Board > Boards Manager > Search for "esp32" and install

### 2. Install Required Libraries
Using Arduino Library Manager (Sketch > Include Library > Manage Libraries):
- Adafruit BME280 Library by Adafruit
- Adafruit CCS811 Library by Adafruit
- PubSubClient by Nick O'Leary
- ArduinoOTA (built-in with ESP32 package)
- Adafruit NeoPixel (if using status LED)

### 3. Configure the Project
1. Copy `config.example.h` to `config.h` (or edit config.h directly)
2. Update the following values in `config.h`:
   - `WIFI_SSID` and `WIFI_PASSWORD`: Your WiFi credentials
   - `MQTT_BROKER`: IP address or hostname of your MQTT broker (e.g., "192.168.1.100" or "homeassistant.local")
   - `MQTT_PORT`: MQTT broker port (default 1883)
   - `MQTT_USER` and `MQTT_PASSWORD`: MQTT credentials (if required, otherwise leave empty strings)
   - `DEVICE_HOSTNAME`: Unique name for this device (e.g., "envlogger_kitchen")
   - `MEASUREMENT_INTERVAL_MS`: Time between measurements in milliseconds (default 300000 = 5 minutes)
   - `ENABLE_MQTT`: Set to 0 to disable MQTT and use SD logging only
   - `BME280_I2C_ADDR`: I2C address of BME280 sensor (0x76 or 0x77, default 0x76)
   - `CCS811_WAKE_PIN`: GPIO connected to CCS811 WAK pin (default -1 for not connected)
   - `SD_CS_PIN`: GPIO for SD card chip select (default 5)
   - `FIRMWARE_VERSION`: Version string for OTA reporting (default "1.0.0")
   - `USE_STATUS_LED`: Set to 1 to enable status LED on GPIO27 (requires NeoPixel library)

### 4. Upload the Code
1. Connect your ESP32 to your computer via USB
2. Select the correct board: Tools > Board > ESP32 Dev Module
3. Select the correct port: Tools > Port
4. Click the Upload button (right arrow)

### 5. First Boot Notes
- The CCS811 sensor requires a 20-minute warm-up period for accurate readings
- Initial readings may show high TVOC/eCO2 values - this is normal
- The device will create a new log file each day on the SD card (format: LOG_YYYYMMDD.CSV)
- If using MQTT, check your broker for incoming messages on the configured topics
- On first boot, the device will attempt to connect to WiFi and MQTT, then enter deep sleep
- Serial monitor (115200 baud) shows detailed startup information and measurement data

## Operation

### Normal Operation
1. **On power-up**: Initialize sensors, SD card, and WiFi
2. **Measurement cycle**: Take measurement → Log to SD card → Publish to MQTT (if enabled)
3. **Deep sleep**: Enter deep sleep for configured interval
4. **Repeat**: Continue until power loss

### Deep Sleep Behavior
- The ESP32 enters true deep sleep between measurements
- Power consumption drops to ~15µA during sleep
- Wakeup is timer-based using the ESP32's RTC timer (no external triggers needed)
- Upon wakeup, execution restarts from `setup()` (all variables reset except stored SD card data)
- Measurement interval is configurable via `MEASUREMENT_INTERVAL_MS` in config.h

### Data Storage
- SD card logs are stored as CSV files in the root directory
- Filename format: `LOG_YYYYMMDD.CSV` (e.g., LOG_20260622.CSV)
- Each line contains: `timestamp,temperature_C,humidity_%,pressure_hPa,TVOC_ppb,eCO2_ppm`
- Timestamp format: `YYYYMMDD_HHMMSS` (24-hour time)
- Files are automatically created when first needed each day
- Header row is written automatically when a new file is created

### MQTT Topics
If MQTT is enabled (`ENABLE_MQTT = 1`), the device publishes to:
- `envlogger/temperature` (float, °C)
- `envlogger/humidity` (float, %)
- `envlogger/pressure` (float, hPa)
- `envlogger/tvoc` (int, ppb)
- `envlogger/eco2` (int, ppm)
- `envlogger/status` (string: "ok"/"error")
- `envlogger/version` (string: firmware version, if defined)

All messages are sent with the `retain` flag so the last value is always available.

Note: MQTT topic names can be customized in config.h if needed.

## Power Consumption

| State           | Current Consumption | Notes                          |
|-----------------|---------------------|--------------------------------|
| Active Measurement | ~120mA             | Sensors reading + WiFi/MQTT    |
| Processing      | ~80mA              | Data logging and publishing    |
| Deep Sleep      | ~15µA              | Timer-based wakeup             |
| **Average**     | **~500µA**         | At 5-minute intervals          |

### Power Consumption Details

The average current consumption depends on:
1. **Measurement interval**: Longer intervals = lower average power
2. **Active time duration**: Time taken to read sensors, connect to WiFi/MQTT, log data, and publish
3. **Deep sleep current**: ~15µA during sleep mode

**Typical 5-minute interval breakdown**:
- Active measurement & WiFi connect: ~3 seconds at ~100mA average
- Data processing & MQTT publish: ~2 seconds at ~80mA average  
- Deep sleep: ~295 seconds at ~15µA
- **Calculated average**: ~450-550µA (varies based on signal strength and sensor response times)

### Battery Life Estimation
- 18650 battery (2600mAh): ~0.6 years (7 months) at 5-minute intervals
- 2x AA LiFeS2 (3000mAh): ~0.7 years (8.5 months)
- With solar panel: Indefinite operation possible with small panel (e.g., 50mA solar cell)

**To reduce power consumption**:
- Increase measurement interval (e.g., to 15-30 minutes for ~200µA average)
- Ensure strong WiFi signal to reduce connection time and retries
- Use ESP32 models with lower deep sleep current if available

## Customization Options

### Changing Measurement Interval
Edit `MEASUREMENT_INTERVAL_MS` in `config.h`:
- 60000  = 1 minute
- 300000 = 5 minutes (default)
- 900000 = 15 minutes
- 1800000 = 30 minutes
- 3600000 = 1 hour

### Adding More Sensors
The code is designed to be extensible:
1. Connect additional sensors to available GPIOs
2. Initialize them in `initializeSensors()` function
3. Read values in `takeMeasurement()` function
4. Add logging fields in `logToSD()` and MQTT topics in `publishMQTT()`

### Deep Sleep Wake via Button
To wake on button press instead of timer:
1. Connect a button between GPIO0 and GND
2. Replace the deep sleep lines in `loop()` with:
   ```cpp
   esp_sleep_enable_ext0_wakeup(GPIO0, 0); // Wake on low level
   esp_deep_sleep_start();
   ```
3. Note: This disables timer-based wakeup

### Local Display
Add an OLED display (SSD1306, I2C):
1. Install Adafruit SSD1306 and GFX libraries
2. Initialize in `setup()`
3. Display readings in `takeMeasurement()` before sleeping
4. Update display on wakeup if desired

## Troubleshooting

### Systematic Approach
When troubleshooting, follow this order:
1. Check serial monitor (115200 baud) for detailed error messages
2. Verify all connections against the wiring guide
3. Confirm correct board and port selection in Arduino IDE
4. Ensure all required libraries are installed
5. Validate configuration in `config.h`

### Common Issues and Solutions

#### SD Card Issues
- **"SD card initialization failed!"**
  - Verify CS pin wiring (GPIO5 by default)
  - Ensure SD card is formatted as FAT32 (not exFAT or NTFS)
  - Try a different SD card (some brands have compatibility issues)
  - Check that `SD_CS_PIN` in config.h matches your wiring
  - Ensure adequate power supply to the SD module

- **"No data on SD card"**
  - Check serial output for file open errors
  - Verify SD card has sufficient free space
  - Confirm you're checking the correct directory on the SD card (root)
  - Try reformatting the SD card as FAT32
  - Ensure header row is present in new files (written automatically)

#### Sensor Issues
- **"Could not find BME280 sensor!"**
  - Check I2C wiring: SCL=GPIO22, SDA=GPIO21 (default)
  - Verify sensor is powered with 3.3V (not 5V)
  - Try both I2C addresses: 0x76 and 0x77
  - Add 4.7kΩ pull-up resistors on SCL/SDA lines if missing
  - Ensure proper grounding between ESP32 and sensor

- **"Could not find CCS811 sensor!"**
  - Same I2C checks as BME280 (SCL=GPIO22, SDA=GPIO21)
  - Allow 20+ minutes warm-up time for accurate readings
  - Verify WAK pin connection (GPIO4 by default, or set to -1 in config.h)
  - Some CCS811 clones require longer initialization delay
  - Check for proper sensor orientation (pin layout varies by manufacturer)

#### Connectivity Issues
- **WiFi connection fails**
  - Double-check SSID and password (case-sensitive)
  - Ensure router is broadcasting on 2.4GHz band (ESP32 doesn't support 5GHz)
  - Confirm router allows new device connections
  - Increase `WIFI_TIMEOUT_MS` in config.h if needed (default 15000ms)
  - ESP32 may require 5-10 seconds to establish connection after boot
  - Try moving closer to the router to improve signal strength

- **MQTT connection fails**
  - Verify MQTT broker IP address or hostname is correct
  - Confirm MQTT port matches broker configuration (default 1883)
  - Check username/password credentials (if required by broker)
  - Ensure broker allows new client connections
  - Test broker connectivity with: `mosquitto_sub -h <broker_ip> -t "#"`
  - Monitor serial output for MQTT connection state and error codes

#### Power and Stability Issues
- **Device resets repeatedly**
  - Check power supply quality and current capability (minimum 500mA recommended)
  - Verify adequate grounding between all components
  - Add decoupling capacitors (0.1µF) near ESP32 power pins if needed
  - Check for shorts or incorrect wiring, especially on power lines
  - Monitor serial output for reset cause information

- **Inconsistent or erratic sensor readings**
  - Allow sufficient warm-up time (especially CCS811: 20 minutes)
  - Keep sensors away from direct heat sources or drafts
  - Ensure stable power supply to prevent brownouts
  - For CCS811, consider periodic baseline reset in fresh air
  - Verify I2C bus isn't overloaded with too many devices

### Advanced Debugging
Enable verbose output by uncommenting:
```cpp
#define DEBUG_SERIAL 1
```
Then add `#ifdef DEBUG_SERIAL` blocks around Serial prints you want to keep.

## Maintenance

### SD Card Management
- Log files grow slowly (~1KB/day at 5-min intervals)
- A 2GB card will last ~5+ years
- To retrieve data: Remove SD card and read on computer
- Files can be opened in Excel, Google Sheets, or any text editor

### Firmware Updates

**Via USB:**
1. Simply re-upload new code through Arduino IDE

**Via OTA (Over-The-Air):**
1. Ensure `ArduinoOTA` is enabled in code (it is by default)
2. Deploy initial firmware via USB
3. For subsequent updates:
   - Select "Network Ports" in Arduino IDE Tools > Port menu
   - Choose your device's IP address
   - Upload as normal (first OTA upload requires USB to set up)

### Sensor Recalibration
**BME280:** Generally stable, no calibration needed

**CCS811:**
- Baseline saves to flash every 24 hours
- To force baseline reset: power cycle with sensor in fresh air for 20+ minutes
- For high-accuracy applications, consider periodic baseline reset

## Example Applications

### Home Air Quality Monitor
- Place in bedroom/living room
- Monitor for VOCs from cleaning products, furniture
- Trigger air purifier when TVOC rises
- Log data to correlate with health symptoms

### Greenhouse Monitor
- Track temperature/humidity for plant health
- Monitor CO2 levels for optimal photosynthesis
- Log data to optimize ventilation/heating schedules
- Add solar power for remote greenhouse deployment

### Workshop Environment Logger
- Monitor for harmful vapors from solvents/paints
- Ensure proper ventilation during projects
- Historical data for safety compliance
- Set up alerts when thresholds exceeded

### Weather Station
- Basic barometric pressure tracking
- Temperature/humidity for comfort monitoring
- Combine with external sensors (rain, wind) for full station

## Integration with Home Assistant

If using Home Assistant, add this to your `configuration.yaml`:

```yaml
mqtt:
  broker: YOUR_MQTT_BROKER
  port: 1883
  username: YOUR_USERNAME
  password: YOUR_PASSWORD

sensor:
  - platform: mqtt
    name: "Office Temperature"
    state_topic: "envlogger/temperature"
    unit_of_measurement: "°C"
    value_template: "{{ value | float }}"

  - platform: mqtt
    name: "Office Humidity"
    state_topic: "envlogger/humidity"
    unit_of_measurement: "%"
    value_template: "{{ value | float }}"

  - platform: mqtt
    name: "Office Pressure"
    state_topic: "envlogger/pressure"
    unit_of_measurement: "hPa"
    value_template: "{{ value | float }}"

  - platform: mqtt
    name: "Office TVOC"
    state_topic: "envlogger/tvoc"
    unit_of_measurement: "ppb"
    value_template: "{{ value | int }}"

  - platform: mqtt
    name: "Office eCO2"
    state_topic: "envlogger/eco2"
    unit_of_measurement: "ppm"
    value_template: "{{ value | int }}"

binary_sensor:
  - platform: mqtt
    name: "EnvLogger Status"
    state_topic: "envlogger/status"
    payload_on: "ok"
    payload_off: "error"
```

Note: Adjust the topic names in the configuration above to match those in your `config.h` file if you've customized them.

## License

MIT License

Copyright (c) 2026 MCU Environmental Data Logger Project

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
---

*Happy monitoring! Breathe easy knowing your environment is tracked.*
