# Captive Portal Provisioning Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a captive portal for easy provisioning of WiFi and MQTT credentials, with automatic device ID generation based on device name and MAC address for unique MQTT topic identification.

**Architecture:** Integrate WiFiManager library to provide captive portal functionality. Store credentials in ESP32 Preferences (NVS). Generate device ID as "{device_name}-{MAC_ADDRESS}" and use it as the first level in MQTT topics (e.g., "env_logger-DE:AD:BE:EF:12:34/temperature").

**Tech Stack:** 
- WiFiManager library (for captive portal)
- ESP32 Preferences (NVS) library (built-in)
- Existing Arduino core libraries (WiFi.h, PubSubClient.h, etc.)

## Global Constraints

- Must maintain backward compatibility with existing configured devices
- Must use ESP32 Preferences (NVS) for credential storage (no external storage dependencies)
- Must generate unique device IDs using device name + MAC address
- Must construct MQTT topics with device ID as first level
- Must maintain existing power efficiency characteristics (deep sleep between measurements)
- Must not break existing OTA update functionality
- Must follow existing code style and conventions in the project
- All new code must be well-documented with comments
- Implementation must fit within existing ESP32 flash and memory constraints

---

### Task 1: Add WiFiManager Library Dependency

**Files:**
- Create: `docs/superpowers/plans/2026-06-23-captive-portal-provisioning.md` (already exists)
- Modify: `docs/superpowers/specs/2026-06-23-captive-portal-provisioning-design.md` (to document dependency)

**Interfaces:**
- Consumes: None (initial setup)
- Produces: Documentation of WiFiManager library requirement

- [ ] **Step 1: Research WiFiManager library for ESP32**
  - Check library compatibility with ESP32 Arduino core
  - Verify library supports custom parameters and preferences storage
  - Confirm memory footprint is acceptable

- [ ] **Step 2: Document WiFiManager dependency in design spec**
  - Add section about required libraries
  - Specify version/commit to use
  - Note installation instructions

- [ ] **Step 3: Commit documentation update**
  - Commit updated design spec with dependency information

### Task 2: Create Preferences Management Functions

**Files:**
- Modify: `mcu_env_logger.ino` (add preferences handling code)
- Create: `docs/superpowers/plans/2026-06-23-captive-portal-provisioning.md` (task documentation)

**Interfaces:**
- Consumes: None
- Produces: Functions to read/write WiFi/MQTT credentials and device name to Preferences

- [x] **Step 1: Add required includes for Preferences**
  - Add `#include <Preferences.h>` to mcu_env_logger.ino
  - Create global Preferences object

- [x] **Step 2: Implement credentials loading functions**
  - Create `loadWiFiCredentials()` function
  - Create `loadMQTTCredentials()` function
  - Create `loadDeviceName()` function with default "env_logger"
  - Functions should return boolean success/failure

- [x] **Step 3: Implement credentials saving functions**
  - Create `saveWiFiCredentials()` function
  - Create `saveMQTTCredentials()` function
  - Create `saveDeviceName()` function
  - Functions should commit changes to Preferences

- [x] **Step 4: Implement credential clearing functions**
  - Create `clearCredentials()` function for factory reset
  - Clear all stored credentials and device name

- [x] **Step 5: Test preferences functions with serial output**
  - Add temporary test code to verify read/write works
  - Remove test code after verification

- [x] **Step 6: Commit preferences management functions to file**
  - Commit the added preferences management code

### Task 3: Implement Device ID Generation

**Files:**
- Modify: `mcu_env_logger.ino` (add device ID functions)

**Interfaces:**
- Consumes: Preferences device name, WiFi MAC address
- Produces: Unique device ID string

- [ ] **Step 1: Implement getDeviceId() function**
  - Retrieve device name from Preferences
  - Get MAC address from WiFi.macAddress()
  - Format as "devicename-MM:SS:SS:SS:SS:SS" (uppercase hex)
  - Return String object

- [ ] **Step 2: Add helper functions for MAC address formatting**
  - Create utility function to convert byte array to hex string
  - Ensure consistent formatting with colons

- [ ] **Step 3: Implement MQTT topic generation functions**
  - Create `getTemperatureTopic()` returning deviceId + "/temperature"
  - Create similar functions for humidity, pressure, tvoc, eco2, status
  - Consider creating a generic topic builder function

- [ ] **Step 4: Add serial output for debugging device ID generation**
  - Print generated device ID during setup for verification
  - Wrap in DEBUG flag if needed

- [ ] **Step 5: Test device ID generation**
  - Verify format matches expected pattern
  - Confirm uniqueness based on MAC address
  - Test with different device names

- [ ] **Step 6: Commit device ID implementation**
  - Commit the device ID and topic generation functions

### Task 4: Modify Connection Functions to Use Preferences

**Files:**
- Modify: `mcu_env_logger.ino` (update connectWiFi() and connectMQTT())

**Interfaces:**
- Consumes: Preferences credential functions
- Produces: Connection functions that use stored credentials

- [ ] **Step 1: Update connectWiFi() function**
  - Replace direct use of WIFI_SSID/WIFI_PASSWORD with preferences values
  - Add fallback to compile-time defaults if preferences not available
  - Add proper error handling and logging

- [ ] **Step 2: Update connectMQTT() function**
  - Replace direct use of MQTT_* constants with preferences values
  - Add fallback to compile-time defaults if preferences not available
  - Handle case where MQTT credentials might be empty strings
  - Add proper error handling and logging

- [ ] **Step 3: Add credential validation functions**
  - Create function to validate WiFi credentials (length, etc.)
  - Create function to validate MQTT broker/port
  - Use these before attempting connections

- [ ] **Step 4: Test connection functions with stored credentials**
  - Verify successful connection when credentials exist in Preferences
  - Verify fallback to compile-time defaults when needed
  - Test error cases (invalid credentials, missing data)

- [ ] **Step 5: Commit connection function updates**
  - Commit the modified connectWiFi() and connectMQTT() functions

### Task 5: Integrate WiFiManager Captive Portal

**Files:**
- Modify: `mcu_env_logger.ino` (add WiFiManager setup and handling)
- Create: `config/captive_portal.html` (optional custom page)

**Interfaces:**
- Consumes: WiFiManager library, preferences functions
- Produces: Captive portal functionality for credential configuration

- [ ] **Step 1: Add WiFiManager include and global object**
  - Add `#include <WiFiManager.h>` 
  - Create global WiFiManager object
  - Add DNSServer include if needed

- [ ] **Step 2: Implement WiFiManager configuration mode callback**
  - Create function to handle when portal starts
  - Set up custom parameters for MQTT and device name
  - Customize AP SSID to include device identifier

- [ ] **Step 3: Implement WiFiManager save callback**
  - Create function to handle when user saves configuration
  - Extract values from WiFiManager parameters
  - Save all credentials and device name to Preferences
  - Trigger restart after successful save

- [ ] **Step 4: Implement WiFiManager connection callback**
  - Create function to handle successful connection via portal
  - This can be minimal as we'll restart after save

- [ ] **Step 5: Implement WiFiManager configuration timeout handling**
  - Set timeout for portal (e.g., 3 minutes)
  - Define behavior when timeout occurs (restart and try normal connection)

- [ ] **Step 6: Add function to start configuration mode**
  - Create `startConfigurationMode()` function
  - Initialize WiFiManager with callbacks
  - Start the captive portal

- [ ] **Step 7: Test captive portal flow**
  - Verify portal starts when no credentials exist
  - Verify user can connect to AP and access configuration page
  - Verify saving credentials works and triggers restart
  - Verify credentials are properly stored and used after restart

- [ ] **Step 8: Commit WiFiManager integration**
  - Commit all WiFiManager related code

### Task 6: Update MQTT Topic Usage Throughout Code

**Files:**
- Modify: `mcu_env_logger.ino` (update all MQTT publish/subscribe calls)
- Modify: `mcu_env_logger.ino` (update MQTT client ID)

**Interfaces:**
- Consumes: Device ID generation functions
- Produces: MQTT communication using device-specific topics

- [ ] **Step 1: Update MQTT client ID**
  - Modify MQTT connection to use device ID as client ID
  - This ensures each device has unique identifier on broker

- [ ] **Step 2: Update publishMQTT() function**
  - Replace hardcoded topic constants with dynamic topic functions
  - Use getTemperatureTopic(), getHumidityTopic(), etc.
  - Maintain retained flag functionality

- [ ] **Step 3: Update any MQTT subscriptions (if applicable)**
  - Check for any existing subscriptions and update them
  - Though current code doesn't show subscriptions, verify

- [ ] **Step 4: Update MQTT connection logic**
  - Ensure client ID is set before connecting
  - Handle potential connection issues with unique client IDs

- [ ] **Step 5: Test MQTT communication with dynamic topics**
  - Verify topics are correctly formed with device ID
  - Verify data publishes to correct topics
  - Verify retained messages work correctly
  - Test with multiple devices to ensure no topic collisions

- [ ] **Step 6: Commit MQTT topic updates**
  - Commit all changes to MQTT usage throughout the code

### Task 7: Update Setup() Function Flow

**Files:**
- Modify: `mcu_env_logger.ino` (update setup() function)

**Interfaces:**
- Consumes: All previously implemented functions
- Produces: Proper initialization sequence with captive portal fallback

- [ ] **Step 1: Reorganize setup() function**
  - Move initialization to occur after credential checking
  - Start with Serial initialization
  - Initialize Preferences
  - Check for valid credentials
  - Attempt normal connection if credentials exist
  - Fall back to captive portal if needed

- [ ] **Step 2: Implement credential validation logic**
  - Create function to check if stored credentials are valid
  - Check for empty SSID, invalid format, etc.
  - Consider adding timestamp for credential expiration (optional)

- [ ] **Step 3: Implement connection attempt with fallback**
  - Try WiFi connection with stored credentials
  - If successful, try MQTT connection
  - If either fails, start captive portal mode
  - Add appropriate delays and retry logic if needed

- [ ] **Step 4: Ensure OTA setup still works**
  - Verify ArduinoOTA.setup() is called appropriately
  - Ensure OTA works in both normal and recovery modes
  - Hostname should use device ID or configurable name

- [ ] **Step 5: Move sensor/SD initialization after connection**
  - Only initialize sensors and SD card after successful connection
  - Or initialize them early but don't use until connected
  - Consider power implications of early initialization

- [ ] **Step 6: Test complete boot flow**
  - Test first boot (no credentials) -> portal -> config -> restart -> normal operation
  - Test subsequent boots with valid credentials -> direct connection -> normal operation
  - Test boot with invalid credentials -> fallback to portal
  - Test OTA functionality in both modes

- [ ] **Step 7: Commit updated setup() function**
  - Commit the reorganized setup() function with proper flow

### Task 8: Add Configuration Persistence and Reset Mechanism

**Files:**
- Modify: `mcu_env_logger.ino` (add button handling for reset)
- Modify: `docs/superpowers/specs/2026-06-23-captive-portal-provisioning-design.md` (document reset)

**Interfaces:**
- Consumes: Button input, preferences clearing functions
- Produces: Factory reset capability

- [ ] **Step 1: Add factory reset capability**
  - Choose GPIO pin for reset button (e.g., GPIO0 if available and safe)
  - Add button initialization in setup()
  - Add check for button press during boot to trigger factory reset
  - Implement reset procedure: clear preferences, restart

- [ ] **Step 2: Add button debouncing**
  - Implement software debouncing for reset button
  - Avoid false triggers from electrical noise

- [ ] **Step 3: Add visual feedback for reset mode**
  - If using status LED, blink pattern to indicate reset mode
  - Or use serial output to indicate reset triggered

- [ ] **Step 4: Test factory reset functionality**
  - Verify button press during boot clears all stored credentials
  - Verify device enters captive portal mode after reset
  - Verify normal operation is prevented until reconfigured
  - Test that reset doesn't interfere with normal operation when not pressed

- [ ] **Step 5: Document reset procedure in design spec**
  - Add section about factory reset capability
  - Explain button hold duration and procedure

- [ ] **Step 6: Commit reset mechanism implementation**
  - Commit the factory reset functionality

### Task 9: Update Configuration Portal Customization and User Experience

**Files:**
- Modify: `mcu_env_logger.ino` (enhance captive portal UI)
- Create: `docs/superpowers/specs/2026-06-23-captive-portal-provisioning-design.md` (update UX)

**Interfaces:**
- Consumes: WiFiManager customization options
- Produces: Improved user experience in captive portal

- [ ] **Step 1: Customize captive portal appearance**
  - Add custom HTML/CSS for better looking interface
  - Include project name/logo if desired
  - Improve layout and field labels

- [ ] **Step 2: Add WiFi network scanning capability**
  - Implement WiFi.scanNetworks() to show available networks
  - Allow user to select from list rather than manual entry
  - Handle refresh functionality

- [ ] **Step 3: Add device ID display in portal**
  - After device name is entered, show preview of generated device ID
  - Help user understand what their MQTT topics will look like

- [ ] **Step 4: Add connection status feedback**
  - Show connection progress in portal
  - Display error messages if configuration fails
  - Provide success confirmation before restart

- [ ] **Step 5: Implement timeout with user notification**
  - Show countdown timer in portal
  - Inform user of remaining configuration time
  - Handle graceful timeout

- [ ] **Step 6: Test enhanced user experience**
  - Verify all UI improvements work correctly
  - Test on mobile and desktop browsers
  - Ensure accessibility considerations (labeling, contrast, etc.)

- [ ] **Step 7: Commit UX enhancements**
  - Commit all captive portal user experience improvements

### Task 10: Final Integration and Testing

**Files:**
- Modify: `mcu_env_logger.ino` (final adjustments and cleanup)
- Create: `docs/superpowers/specs/2026-06-23-captive-portal-provisioning-design.md` (final updates)
- Create: `tests/` directory with test sketches (optional)

**Interfaces:**
- Consumes: All implemented features
- Produces: Complete, tested captive portal provisioning system

- [ ] **Step 1: Remove debug code and temporary test statements**
  - Clean up any Serial.print() statements added for debugging
  - Ensure only essential logging remains
  - Consider adding DEBUG macro for optional verbose output

- [ ] **Step 2: Optimize memory usage**
  - Check for memory leaks (especially with String objects)
  - Optimize string handling to prevent fragmentation
  - Verify PROGMEM usage for constant strings where appropriate

- [ ] **Step 3: Verify power consumption characteristics**
  - Ensure deep sleep current is unaffected
  - Verify active time is reasonable
  - Check that WiFiManager doesn't increase awake time significantly

- [ ] **Step 4: Verify backward compatibility**
  - Test that existing config.h based configuration still works
  - Or document that this replaces config.h method
  - If replacing, provide migration guide for existing users

- [ ] **Step 5: Comprehensive system testing**
  - Test multiple device scenarios (ensuring unique IDs/topics)
  - Test OTA updates work in both modes
  - Test edge cases (special characters in names, long SSIDs, etc.)
  - Test recovery from failed configurations
  - Test long-term operation (multiple sleep/wake cycles)

- [ ] **Step 6: Update documentation**
  - Update README.md with new configuration instructions
  - Update COMPILE_USER_GUIDE.md with captive portal usage
  - Ensure design spec reflects final implementation

- [ ] **Step 7: Final commit**
  - Commit all cleanup, optimization, and documentation updates

- [ ] **Step 8: Create release notes**
  - Document the new feature and its benefits
  - Note any breaking changes or migration steps