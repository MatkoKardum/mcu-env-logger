# Captive Portal Provisioning Design

## Overview
This document describes the implementation of a captive portal for easy provisioning of WiFi and MQTT credentials in the MCU Environmental Data Logger project. The solution includes automatic generation of unique device IDs based on device name and MAC address, which form part of the MQTT topic structure to enable easy identification of multiple sensors on the broker.

## Problem Statement
Currently, users must manually edit the `config.h` file to configure WiFi and MQTT credentials, which requires recompiling and re-uploading the firmware. This process is not user-friendly, especially for non-technical users. Additionally, managing multiple sensors on an MQTT broker requires manually ensuring unique topic names, which is error-prone.

## Solution Overview
Implement a captive portal that:
1. Automatically starts when no valid credentials are found or connection fails
2. Provides a web interface for configuring WiFi SSID/password, MQTT broker details, and optional device name
3. Stores credentials persistently using ESP32 Preferences (NVS)
4. Generates a unique device ID using the format: `{device_name}-{MAC_ADDRESS}`
5. Constructs MQTT topics using the device ID as the first level: `{device_id}/{sensor_type}`

## Detailed Design

### Architecture Overview
```
[Power On] 
     ↓
[Check Preferences for Valid Credentials] 
     ↙                     ↘
[Valid Credentials]        [Invalid/Missing Credentials]
   ↓                           ↓
[Attempt WiFi/MQTT Connection] [Start Captive Portal Mode]
   ↓                           ↓
[Connection Successful?] ←----[Serve Configuration Portal] ← [User Submission]
   ↙                     ↘
[Yes]                     [No]
  ↓                       ↓
[Normal Operation]    [Show Error, Allow Retry]
```

### Components

#### 1. Preferences Storage
Credentials and device name will be stored in ESP32 Preferences (NVS) using the following keys:
- `wifi_ssid` (String)
- `wifi_pass` (String)  
- `mqtt_broker` (String)
- `mqtt_port` (UInt16, default 1883)
- `mqtt_user` (String)
- `mqtt_pass` (String)
- `device_name` (String, default "env_logger")

#### 2. Device ID Generation
```cpp
String generateDeviceId() {
    String baseName = preferences.getString("device_name", "env_logger");
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return baseName + "-" + String(macStr);
}
```
Example output: `env_logger-DE:AD:BE:EF:12:34`

#### 3. MQTT Topic Construction
Topics will be dynamically generated using the device ID:
```cpp
String getDeviceId() {
    // Returns formatted device ID as above
}

String getTemperatureTopic() {
    return getDeviceId() + "/temperature";
}

String getHumidityTopic() {
    return getDeviceId() + "/humidity";
}
// Similar for pressure, tvoc, eco2, status
```

#### 4. WiFiManager Integration
- Use WiFiManager library to handle captive portal functionality
- Add custom parameters for:
  - MQTT broker address
  - MQTT port
  - MQTT username
  - MQTT password
  - Device name (optional, defaults to "env_logger")
- On successful save, store all parameters to Preferences
- Set the WiFiManager AP SSID to include device name for easy identification: `ESP_DeviceName_XXXXXX`

### Integration Points

#### Modified `setup()` Flow
1. Initialize Serial and basic hardware
2. Initialize Preferences namespace
3. Check for valid credentials in Preferences
4. If valid credentials exist:
   - Attempt WiFi connection
   - If successful, attempt MQTT connection
   - If both successful, proceed to sensor initialization
5. If no valid credentials or connection fails:
   - Start WiFiManager in configuration mode
   - Serve captive portal for user configuration
   - On successful save, store credentials and restart
6. Initialize sensors, SD card, etc.
7. Set up OTA updates
8. Enter main loop

#### Modified Connection Functions
- `connectWiFi()`: Retrieve credentials from Preferences instead of config.h
- `connectMQTT()`: Retrieve credentials from Preferences instead of config.h
- Update MQTT topic defines to be runtime-generated functions

### Data Flow

#### Configuration Flow
1. User powers on unconfigured device
2. Device detects no valid credentials → starts captive portal AP
3. User connects to AP (e.g., "ESP_env_logger_A1B2C3")
4. User visits 192.168.4.1 in browser
5. User fills in WiFi, MQTT, and optional device name fields
6. User clicks "Save"
7. WiFiManager validates input and calls save callback
8. Save callback stores all parameters to Preferences
9. Device restarts
10. On boot, device finds valid credentials → connects to WiFi/MQTT
11. Device begins normal operation (sensing, logging, publishing)

#### Operational Flow
1. Device boots and reads credentials from Preferences
2. Device generates device ID from stored name + MAC address
3. Device connects to WiFi using stored credentials
4. Device connects to MQTT using stored credentials and device ID as client ID
5. In measurement loop:
   - Read sensors
   - Log to SD card
   - Publish to MQTT topics using dynamically generated topic strings
6. Enter deep sleep until next interval

### Error Handling & Edge Cases

#### Credential Validation
- Validate WiFi SSID length (1-32 chars)
- Validate WiFi password length (0-63 chars)
- Validate MQTT broker (non-empty string)
- Validate MQTT port (1-65535)
- Sanitize device name for MQTT topic validity (remove invalid characters)

#### Connection Failure Handling
- WiFi connection failure: Clear WiFi credentials from Preferences, restart portal after timeout
- MQTT connection failure: retain WiFi credentials, retry MQTT periodically
- Portal timeout: Return to normal operation after configurable period (default 3 minutes)
- Storage corruption: Provide mechanism to clear Preferences (e.g., hold button on boot)

#### Special Cases
- **Device rename**: Changing device name generates new device ID and topics
- **MAC address change**: Extremely unlikely on ESP32, but would change device ID
- **Special characters in device name**: Sanitize for MQTT topic compatibility
- **Multiple device deployment**: Each gets unique ID based on name+MAC

### User Experience

#### First-Time Setup
1. Power on device → creates WiFi AP: `ESP_env_logger_A1B2C3` (last 3 bytes of MAC)
2. Connect smartphone/laptop to this AP
3. Open browser to 192.168.4.1
4. Configure:
   - WiFi Network: [dropdown/scan] + manual entry
   - WiFi Password: [input]
   - MQTT Broker: [input, e.g., 192.168.1.100]
   - MQTT Port: [input, default 1883]
   - MQTT Username: [input, optional]
   - MQTT Password: [input, optional]
   - Device Name: [input, default "env_logger"]
5. Click "Save"
6. Device restarts and connects to configured networks
7. Device begins publishing to topics like:
   - `env_logger-A1:B2:C3:D4:E5:F6/temperature`
   - `env_logger-A1:B2:C3:D4:E5:F6/humidity`
   - etc.

#### Subsequent Boots
1. Device boots, finds valid credentials in Preferences
2. Skips captive portal, attempts direct connection
3. Proceeds to normal operation if successful
4. Falls back to portal only if connection fails

## Implementation Plan Reference
This design will be implemented according to the plan generated by the `writing-plans` skill, which will detail:
- Specific file modifications
- Library dependencies to add
- Code changes for each function
- Testing procedures
- Migration path for existing users

## Benefits
1. **User-Friendly Provisioning**: No need to modify code or recompile for initial setup
2. **Guaranteed Unique Identification**: Device ID combines user-friendly name with unique MAC address
3. **Hierarchical MQTT Topics**: Easy to subscribe to all sensors of a type (`+/temperature`) or all data from one device (`device_id/#`)
4. **Robust Credential Storage**: Uses ESP32 Preferences with wear-leveling
5. **Fallback Mechanism**: Graceful degradation to captive portal on connection failure
6. **Backward Compatible**: Existing configured devices continue to work unchanged

## Open Questions
1. Should we add a factory reset mechanism (e.g., button hold on boot)?
2. What should be the captive portal timeout duration?
3. Should we add WiFi network scanning capability to the portal?
4. Should we display the generated device ID in the portal after generation?
5. What default values should we use for optional fields?

---

*This design document follows the MCU Environmental Data Logger project conventions and extends the existing architecture to provide user-friendly provisioning while maintaining the project's core functionality and power efficiency principles.*