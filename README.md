# MCU Environmental Data Logger

<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Arduino](https://img.shields.io/badge/Arduino-Compatible-green)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/ESP32-Supported-blue)](https://www.espressif.com/)

**Real-time environmental monitoring with ESP32 - measures temperature, humidity, pressure, VOCs, and CO₂. Logs to SD card. Integrates with MQTT & Home Assistant.**

[⭐ If this project helps you, please star it!](#)  
[💝 Support ongoing development](#-support-this-project)

</div>

---

## 🚀 Features

- **Multi-sensor monitoring**: BME280 (temp/humidity/pressure) + CCS811 (VOC/CO₂)
- **Local data logging**: CSV to microSD card (no cloud dependency, works offline)
- **Wireless integration**: WiFi + MQTT for real-time data to Home Assistant, Node-RED, etc.
- **Power efficient**: Deep sleep between measurements (~15µA), 1.5+ years on battery
- **OTA updates**: Wireless firmware updates without physical access
- **Extensible**: Easily add more sensors with modular code
- **Production ready**: Error handling, reconnection logic, status LED support
- **Fully documented**: Setup guides, wiring diagrams, customization examples

## 📊 What You Can Monitor

- **Temperature** (°C) - BME280
- **Humidity** (%) - BME280
- **Barometric Pressure** (hPa) - BME280
- **VOCs** (ppb) - CCS811 (air quality indicator)
- **CO₂ equivalent** (ppm) - CCS811
- **Timestamps** - All data entries include precise timestamps

Perfect for: home air quality, greenhouses, labs, server rooms, weather stations, environmental research.

## 🛠 Hardware Requirements

| Component | Qty | Notes |
|-----------|-----|-------|
| ESP32 Dev Board | 1 | Any ESP32-WROOM variant |
| BME280 sensor | 1 | I2C temperature/humidity/pressure |
| CCS811 sensor | 1 | I2C VOC/CO₂ sensor |
| microSD module | 1 | SPI, with card (FAT32, up to 32GB) |
| Breadboard + wires | 1 | For prototyping |
| USB cable | 1 | For power and programming |

**Optional:**
- WS2812B RGB LED for status indicators
- 3.7V LiPo battery + solar panel for remote deployment

**Estimated cost: $18-35** (components available from Aliexpress, Amazon)

## 🔌 Wiring

```
ESP32        BME280      CCS811      microSD      LED (opt)
3.3V    ←----VCC------VCC------VCC           
GND     ←----GND------GND------GND------GND
GPIO22  ←----SCL------SCL
GPIO21  ←----SDA------SDA
GPIO5   ←----------------------------CS
GPIO18  ←----------------------------SCK
GPIO19  ←----------------------------MISO
GPIO23  ←----------------------------MOSI
GPIO15  ←----------------------------------DIN
GPIO4   ←----------WAKE
GPIO27  ←----------------------------------VCC (LED only)
```

See [COMPLETE_USER_GUIDE.md](COMPLETE_USER_GUIDE.md) for detailed diagrams.

## 📝 Quick Start

### 1. Install Arduino IDE / PlatformIO
- [Arduino IDE](https://www.arduino.cc/en/software) - beginner-friendly
- [VS Code + PlatformIO](https://platformio.org/) - advanced

### 2. Add ESP32 Board Support
In Arduino IDE:
- Files → Preferences → Additional Boards Manager URLs:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Tools → Board Manager → Search "esp32" → Install

### 3. Install Libraries
Sketch → Include Library → Manage Libraries (search and install):
- `Adafruit BME280` by Adafruit
- `Adafruit CCS811` by Adafruit  
- `PubSubClient` by Nick O'Leary
- `Adafruit NeoPixel` (optional, for LED)

### 4. Configure & Upload
```bash
# Clone this repo or download as ZIP
git clone https://github.com/MatkoKardum/mcu-env-logger.git
cd mcu-env-logger

# Copy config template
cp config.example.h config.h

# Edit config.h with your WiFi, MQTT, GPIO pins
nano config.h

# Upload: Select Board → Port → Upload
```

See [COMPLETE_USER_GUIDE.md](COMPLETE_USER_GUIDE.md) for full setup guide.

## 📡 MQTT Topics

If MQTT enabled, device publishes to:
```
envlogger/temperature  → 23.5
envlogger/humidity     → 45.2
envlogger/pressure     → 1013.2
envlogger/tvoc         → 150
envlogger/eco2         → 450
envlogger/status       → ok
```

**Home Assistant integration** - automatically discovers via MQTT Discovery!

## 💾 Data Logging

CSV format on SD card: `LOG_20260622.CSV`
```
timestamp,temperature_C,humidity_%,pressure_hPa,TVOC_ppb,eCO2_ppm
20260622_143022,23.5,45.2,1013.2,150,450
20260622_143322,23.4,45.1,1013.1,148,448
```

## ⚡ Power Consumption

| State | Current | Notes |
|-------|---------|-------|
| Active reading | ~120mA | WiFi + sensors active |
| Processing | ~80mA | Logging and publishing |
| Deep sleep | **~15µA** | Timer-based wakeup |
| **Average** (5min intervals) | **~200µA** | ~1.5 years on 18650 battery |

## 🎯 Use Cases

- 🏠 **Home Air Quality Monitor** - Track VOCs while cooking, cleaning, burning candles
- 🌱 **Greenhouse** - Monitor growing conditions, log to SD, trigger automations
- 🔬 **Lab/Workspace** - Continuous logging for environmental studies
- 🖥️ **Server Room** - Alert on temperature/humidity extremes
- 🌦️ **Weather Station** - Pressure trending for weather prediction
- 🔋 **Remote Sensing** - Solar-powered deployment for field research

## 🔧 Customization

### Change measurement interval
Edit `config.h`:
```cpp
#define MEASUREMENT_INTERVAL_MS 300000  // 5 minutes
// Options: 60000 (1 min), 900000 (15 min), 3600000 (1 hour)
```

### Add a new sensor
1. Initialize in `initializeSensors()`
2. Read in `takeMeasurement()`
3. Log in `logToSD()` and `publishMQTT()`

### Enable status LED
Uncomment in code and configure GPIO:
```cpp
#define STATUS_LED_PIN 27
#define STATUS_LED_COUNT 1
```

See [COMPLETE_USER_GUIDE.md](COMPLETE_USER_GUIDE.md) for advanced customization.

## 📚 Documentation

- **[README.md](README.md)** - This file
- **[COMPLETE_USER_GUIDE.md](COMPLETE_USER_GUIDE.md)** - Detailed setup, troubleshooting, advanced config
- **[USER_GUIDE.md](USER_GUIDE.md)** - Quick reference
- **[config.example.h](config.example.h)** - Configuration template
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - How to contribute

## 🤝 Contributing

Contributions welcome! Report bugs, suggest features, or submit code:
- 🐛 [Report a bug](../../issues/new)
- 💡 [Request a feature](../../issues/new)
- 🔧 [Submit a pull request](../../pulls)

See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

---

## 💝 Support This Project

If this project saves you time or money, please consider supporting continued development.

### For Users in Bosnia & Herzegovina

**Recommended Option: Wise (TransferWise)**
- Works reliably in Bosnia & Herzegovina
- Low fees for international transfers
- Guide: https://wise.com/guide/send-money-to-bosnia-and-herzegovina

**Direct Bank Transfer**
- Open an issue on GitHub to request IBAN details
- Can arrange direct transfer to Bosnian bank account

### Other Options (if available in your country)

- 💜 **GitHub Sponsors** (when available)
- 👏 **Patreon** (alternative payment methods)
- 🎁 **Ko-fi** (if payment method works in your region)

### Other Ways to Help
- ⭐ **Star this repo** - Helps discovery and shows support
- 🔄 **Share** - Tell colleagues, friends, online communities
- 🐛 **Report bugs** - Issues help make this better
- 🔧 **Contribute** - Code, docs, examples, translations
- 💬 **Feedback** - What features would help you?

Even small support makes a big difference! Thank you 🙏

---

## 📄 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file.

## ⚠️ Disclaimer

This project is provided as-is without warranty. Users are responsible for:
- Proper ESP32 power supply and wiring
- Sensor calibration as needed
- Secure MQTT broker configuration
- Compliance with local WiFi regulations
- Data privacy and security

## 🙌 Acknowledgments

- Built with ❤️ using [Arduino](https://www.arduino.cc/) and Adafruit libraries
- Inspired by home automation communities and DIY sensor projects
- Special thanks to contributors and everyone who has starred/forked

---

**Questions?** Open an [Issue](../../issues) or check [COMPLETE_USER_GUIDE.md](COMPLETE_USER_GUIDE.md).

**Happy monitoring! 🌡️📊**
