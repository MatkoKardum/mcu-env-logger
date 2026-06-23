/**
 * MCU Environmental Data Logger
 * ESP32-based multi-sensor environmental monitor with SD logging and MQTT
 *
 * Sensors: BME280 (temp/hum/pressure) + CCS811 (TVOC/eCO2)
 * Storage: microSD card (CSV logging)
 * Communication: Wi-Fi + MQTT
 * Power: Deep sleep between measurements
 *
 * Created: 2026-06-22
 * License: MIT
 */

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_CCS811.h>
#include <SD.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include "config.h"

// Optional: Status LED (uncomment if using)
// #include <Adafruit_NeoPixel.h>
// #define STATUS_LED_PIN    27
// #define STATUS_LED_COUNT  1
// Adafruit_NeoPixel statusLED(STATUS_LED_COUNT, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

// Preferences object for NVS storage
Preferences preferences;

// WiFiManager object for captive portal
WiFiManager wifiManager;

// Sensor objects
Adafruit_BME280 bme;
Adafruit_CCS811 ccs811;

// Network objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// File object for SD logging
File dataFile;

// Timing variables
unsigned long lastMeasurement = 0;
const unsigned long measurementInterval = MEASUREMENT_INTERVAL_MS; // From config.h

// State tracking
bool wifiConnected = false;
bool mqttConnected = false;
bool sdInitialized = false;
bool sensorsInitialized = false;
// Device ID and topic functions
String getDeviceId();
String getTemperatureTopic();
String getHumidityTopic();
String getPressureTopic();
String getTvocTopic();
String getEco2Topic();
String getStatusTopic;
// Forward declarations
bool initializeSensors();
bool initializeSD();
bool connectWiFi();
bool connectMQTT();
void takeMeasurement();
void logToSD(float temp, float hum, float pres, uint16_t tvoc, uint16_t eco2);
void publishMQTT(float temp, float hum, float pres, uint16_t tvoc, uint16_t eco2);
void handleOTA();
void setStatusLED(uint32_t color); // Uncomment if using LED

// Preferences management functions
bool loadWiFiCredentials(char* ssid, size_t ssidSize, char* password, size_t passSize);
bool loadMQTTCredentials(char* broker, size_t brokerSize, uint16_t* port,
                        char* user, size_t userSize, char* password, size_t passSize);
bool loadDeviceName(char* name, size_t nameSize);
bool saveWiFiCredentials(const char* ssid, const char* password);
bool saveMQTTCredentials(const char* broker, uint16_t port,
                        const char* user, const char* password);
bool saveDeviceName(const char* name);
bool clearCredentials();

// WiFiManager callback functions
void wifiConfigModeCallback(WiFiManager *myWiFiManager);
void wifiSaveCallback();
void wifiConnectCallback();

// Function to start configuration mode
void startConfigurationMode();

// Device ID generation functions

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== MCU Environmental Data Logger ===");
    Serial.println("Version: 1.0");
    Serial.println("Built: " __DATE__ " " __TIME__);

    // Initialize status LED if used
    // statusLED.begin();
    // statusLED.show(); // Initialize all pixels to 'off'
    // setStatusLED(0xFF0000); // Red = initializing

    // Initialize Preferences
    preferences.begin("env_logger", false);
    preferences.end();

    // Print device ID for debugging
    String deviceID = getDeviceId();
    Serial.printf("Device ID: %s\n", deviceID.c_str());

    // Check if we have valid credentials in Preferences
    bool hasCredentials = false;
    char ssid[32] = {0};
    char pass[64] = {0};

    if (loadWiFiCredentials(ssid, sizeof(ssid), pass, sizeof(pass))) {
        if (strlen(ssid) > 0) {
            hasCredentials = true;
            Serial.println("Found WiFi credentials in Preferences");
        }
    }

    // If we have credentials, try to connect
    if (hasCredentials) {
        Serial.println("Attempting to connect with stored credentials...");
        wifiConnected = connectWiFi();

        if (wifiConnected) {
            Serial.println("WiFi connected successfully");

            // Try to connect to MQTT
            Serial.println("Connecting to MQTT...");
            mqttConnected = connectMQTT();

            if (mqttConnected) {
                Serial.println("MQTT connected successfully");
            } else {
                Serial.println("MQTT connection failed - will retry in loop");
            }
        } else {
            Serial.println("WiFi connection failed - starting configuration mode");
            startConfigurationMode();
            return; // Skip normal setup, will restart after config
        }
    } else {
        Serial.println("No valid credentials found - starting configuration mode");
        startConfigurationMode();
        return; // Skip normal setup, will restart after config
    }

    // Initialize components only if we have a connection
    Serial.println("Initializing sensors...");
    sensorsInitialized = initializeSensors();

    Serial.println("Initializing SD card...");
    sdInitialized = initializeSD();

    if (wifiConnected)
    {
        // Setup OTA updates
        ArduinoOTA.setHostname(DEVICE_HOSTNAME);
        ArduinoOTA.onStart([])
                           { Serial.println("OTA Start"); };
        ArduinoOTA.onEnd([])
                         { Serial.println("\nOTA End"); };
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                              { Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total); });
        ArduinoOTA.onError([](ota_error_t error)
                           {
        Serial.printf("OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
        ArduinoOTA.begin();
        Serial.println("OTA updates enabled");
    }

    // setStatusLED(0x00FF00); // Green = ready (if using LED)
    Serial.println("Setup complete. Entering main loop.");
}

void loop()
{
    // Handle OTA updates
    handleOTA();

    // Check if it's time for a measurement
    if (millis() - lastMeasurement >= measurementInterval)
    {
        Serial.println("\n--- Starting measurement cycle ---");
        lastMeasurement = millis();

        // Take measurement from sensors
        if (sensorsInitialized)
        {
            takeMeasurement();
        }
        else
        {
            Serial.println("Sensors not initialized - skipping measurement");
        }

        // Reconnect WiFi/MQTT if needed
        if (!wifiConnected)
        {
            wifiConnected = connectWiFi();
            if (wifiConnected)
            {
                mqttConnected = connectMQTT();
            }
        }
        else if (!mqttConnected)
        {
            mqttConnected = connectMQTT();
        }

        // Enter deep sleep to save power
        Serial.println("Entering deep sleep...");
        // setStatusLED(0x000000); // Off (if using LED)
        esp_sleep_enable_timer_wakeup(measurementInterval * 1000ULL); // Convert ms to µs
        esp_deep_sleep_start();
        // Note: Code execution resumes here after wakeup (from setup())
    }

    // Small delay to prevent watchdog issues
    delay(10);
}

// WiFiManager callback functions
void wifiConfigModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered configuration mode");
    Serial.print("Config SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
    Serial.print("Config IP: ");
    Serial.println(WiFi.softAPIP());
}

// Callback to save WiFiManager parameters to Preferences
void wifiSaveCallback() {
    Serial.println("Saving configuration from WiFiManager");

    // Save WiFi credentials
    String wifiSSID = WiFi.SSID();
    String wifiPass = WiFi.psk();
    saveWiFiCredentials(wifiSSID.c_str(), wifiPass.c_str());

    // For custom parameters (MQTT, device name), we would retrieve them here
    // But since we're using autoConnect, WiFiManager will save the basic credentials

    Serial.println("Configuration saved to Preferences");

    // Delay to allow serial output to be sent before restart
    delay(1000);

    // Restart the device to apply new settings
    ESP.restart();
}

// Callback for successful connection via WiFiManager
void wifiConnectCallback() {
    Serial.println("Connected successfully via WiFiManager");
}

// Function to start configuration mode
void startConfigurationMode() {
    Serial.println("Starting configuration mode");

    // Set config portal timeout (optional)
    wifiManager.setConfigPortalTimeout(180); // 3 minutes timeout

    // Set callback functions
    wifiManager.setAPCallback(wifiConfigModeCallback);
    wifiManager.setSaveConfigCallback(wifiSaveCallback);

    // Start the configuration portal
    Serial.println("Starting WiFi configuration portal");
    bool result = wifiManager.autoConnect(); // This will block until config is saved or timeout

    if (!result) {
        Serial.println("Failed to connect or timeout occurred");
        // TODO: Handle timeout - maybe try to connect with existing credentials?
    } else {
        Serial.println("Connected successfully via WiFiManager");
        // Note: The save callback will handle saving and restarting
    }

    // If we get here, the connection attempt failed or timed out
    Serial.println("Configuration portal exited");
}

bool initializeSensors()
{
    // Initialize BME280
    if (!bme.begin(BME280_I2C_ADDR))
    {
        Serial.println("Could not find BME280 sensor!");
        return false;
    }
    Serial.println("BME280 initialized");

    // Initialize CCS811
    if (!ccs811.begin())
    {
        Serial.println("Could not find CCS811 sensor!");
        return false;
    }

    // Wait for CCS811 to be ready (requires ~20ms)
    while (!ccs811.available())
    {
        delay(10);
    }
    Serial.println("CCS811 initialized");

    return true;
}

bool initializeSD()
{
    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println("SD card initialization failed!");
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
        Serial.println("No SD card detected");
        return false;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC)
        Serial.println("MMC");
    else if (cardType == CARD_SD)
        Serial.println("SDSC");
    else if (cardType == CARD_SDHC)
        Serial.println("SDHC");
    else
        Serial.println("UNKNOWN");

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    sdInitialized = true;
    return true;
}

bool connectWiFi()
{
    // Try to load WiFi credentials from Preferences first
    char ssid[32] = {0};
    char password[64] = {0};

    bool useSavedCredentials = loadWiFiCredentials(ssid, sizeof(ssid), password, sizeof(password));

    if (useSavedCredentials) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        Serial.print("Connecting to WiFi (from Preferences): ");
        Serial.println(ssid);
    } else {
        // Fallback to hardcoded credentials from config.h
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.print("Connecting to WiFi (from config.h): ");
        Serial.println(WIFI_SSID);
    }

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        // setStatusLED(0x0000FF); // Blue = WiFi connected (if using LED)
        return true;
    }
    else
    {
        Serial.println("\nWiFi connection failed!");
        // setStatusLED(0xFF0000); // Red = WiFi failed (if using LED)
        return false;
    }
}

bool connectMQTT()
{
    // Try to load MQTT credentials from Preferences first
    char broker[64] = {0};
    char user[32] = {0};
    char pass[32] = {0};
    uint16_t port = 1883;

    bool useSavedCredentials = loadMQTTCredentials(broker, sizeof(broker), &port,
                                                  user, sizeof(user), pass, sizeof(pass));

    if (useSavedCredentials) {
        mqttClient.setServer(broker, port);
        Serial.print("Connecting to MQTT broker (from Preferences): ");
        Serial.print(broker);
        Serial.print(":");
        Serial.println(port);
    } else {
        // Fallback to hardcoded credentials from config.h
        mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
        Serial.print("Connecting to MQTT broker (from config.h): ");
        Serial.print(MQTT_BROKER);
        Serial.print(":");
        Serial.println(MQTT_PORT);
    }

    mqttClient.setCallback([](char *topic, byte *payload, unsigned int length)
                           {
    // Handle incoming MQTT messages if needed
    Serial.print("MQTT message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println(); });

    String clientId = DEVICE_HOSTNAME;
    // Try to get device name for MQTT client ID
    char deviceName[32] = {0};
    if (loadDeviceName(deviceName, sizeof(deviceName))) {
        clientId = String(deviceName);
    }

    if (mqttClient.connect(clientId.c_str(),
                          useSavedCredentials ? user : MQTT_USER,
                          useSavedCredentials ? pass : MQTT_PASSWORD))
    {
        Serial.println("MQTT connected");
        // setStatusLED(0x00FF00); // Green = MQTT connected (if using LED)
        return true;
    }
    else
    {
        Serial.print("MQTT connection failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" - will retry next measurement cycle");
        // setStatusLED(0xFFA500); // Orange = MQTT failed (if using LED)
        return false;
    }
}

void takeMeasurement()
{
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F; // Convert Pa to hPa

    // Check if any reads failed
    if (isnan(temperature) || isnan(humidity) || isnan(pressure))
    {
        Serial.println("Failed to read from BME280 sensor!");
        return;
    }

    // Read CCS811
    uint16_t tvoc = 0, eco2 = 0;
    if (ccs811.available())
    {
        if (!ccs811.readData())
        {
            tvoc = ccs811.getTVOC();
            eco2 = ccs811.geteCO2();
        }
        else
        {
            Serial.println("CCS811 data error!");
            // Use last known values or zeros
        }
    }

    // Print to serial
    Serial.printf("Temp: %.1f°C  Hum: %.1f%%  Pres: %.1fhPa  TVOC: %dppb  eCO2: %dppm\n",
                  temperature, humidity, pressure, tvoc, eco2);

    // Log to SD card
    if (sdInitialized)
    {
        logToSD(temperature, humidity, pressure, tvoc, eco2);
    }

#if defined(ENABLE_MQTT) && ENABLE_MQTT
    // Publish to MQTT
    if (mqttConnected)
    {
        publishMQTT(temperature, humidity, pressure, tvoc, eco2);
    }
#endif
}

void logToSD(float temp, float hum, float pres, uint16_t tvoc, uint16_t eco2)
{
    // Create filename based on date (YYYYMMDD.CSV)
    char filename[13];
    time_t now = time(nullptr);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    strftime(filename, sizeof(filename), "LOG_%Y%m%d.CSV", &timeinfo);

    // Open file for appending
    dataFile = SD.open(filename, FILE_APPEND);

    if (!dataFile)
    {
        Serial.println("Error opening log file for writing");
        return;
    }

    // Write header if file is new
    if (dataFile.size() == 0)
    {
        dataFile.println("timestamp,temperature_C,humidity_%,pressure_hPa,TVOC_ppb,eCO2_ppm");
    }

    // Write data line
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);
    dataFile.printf("%s,%.1f,%.1f,%.1f,%d,%d\n",
                    timestamp, temp, hum, pres, tvoc, eco2);

    dataFile.close();
    Serial.printf("Logged to %s\n", filename);
}

void publishMQTT(float temp, float hum, float pres, uint16_t tvoc, uint16_t eco2)
{
    // Publish individual sensor readings
    mqttClient.publish(getTemperatureTopic().c_str(), String(temp).c_str(), true);
    mqttClient.publish(getHumidityTopic().c_str(), String(hum).c_str(), true);
    mqttClient.publish(getPressureTopic().c_str(), String(pres).c_str(), true);
    mqttClient.publish(getTvocTopic().c_str(), String(tvoc).c_str(), true);
    mqttClient.publish(getEco2Topic().c_str(), String(eco2).c_str(), true);

    // Publish status
    mqttClient.publish(getStatusTopic().c_str(), "ok", true);

    Serial.println("Data published to MQTT");
}

void handleOTA()
{
    ArduinoOTA.handle();
}

// void setStatusLED(uint32_t color) {
// #if defined(USE_STATUS_LED) && USE_STATUS_LED
//   statusLED.setPixelColor(0, color);
//   statusLED.show();
// #endif
// }

// Preferences management functions
bool loadWiFiCredentials(char* ssid, size_t ssidSize, char* password, size_t passSize) {
    preferences.begin("env_logger", false);
    bool success = false;

    // Read WiFi SSID
    String storedSsid = preferences.getString("wifi_ssid", "");
    if (storedSsid.length() > 0 && storedSsid.length() < ssidSize) {
        storedSsid.toCharArray(ssid, ssidSize);

        // Read WiFi password
        String storedPass = preferences.getString("wifi_pass", "");
        if (storedPass.length() < passSize) {
            storedPass.toCharArray(password, passSize);
            success = true;
            Serial.println("WiFi credentials loaded from Preferences");
        } else {
            Serial.println("WiFi password buffer too small");
        }
    } else {
        Serial.println("No WiFi SSID found in Preferences or buffer too small");
    }

    preferences.end();
    return success;
}

bool loadMQTTCredentials(char* broker, size_t brokerSize, uint16_t* port,
                        char* user, size_t userSize, char* password, size_t passSize) {
    preferences.begin("env_logger", false);
    bool success = false;

    // Read MQTT broker
    String storedBroker = preferences.getString("mqtt_broker", "");
    if (storedBroker.length() > 0 && storedBroker.length() < brokerSize) {
        storedBroker.toCharArray(broker, brokerSize);

        // Read MQTT port
        *port = preferences.getUShort("mqtt_port", 1883);

        // Read MQTT username
        String storedUser = preferences.getString("mqtt_user", "");
        if (storedUser.length() < userSize) {
            storedUser.toCharArray(user, userSize);

            // Read MQTT password
            String storedPass = preferences.getString("mqtt_pass", "");
            if (storedPass.length() < passSize) {
                storedPass.toCharArray(password, passSize);
                success = true;
                Serial.println("MQTT credentials loaded from Preferences");
            } else {
                Serial.println("MQTT password buffer too small");
            }
        } else {
            Serial.println("MQTT username buffer too small");
        }
    } else {
        Serial.println("No MQTT broker found in Preferences or buffer too small");
    }

    preferences.end();
    return success;
}

bool loadDeviceName(char* name, size_t nameSize) {
    preferences.begin("env_logger", false);
    bool success = false;

    String storedName = preferences.getString("device_name", "env_logger");
    if (storedName.length() < nameSize) {
        storedName.toCharArray(name, nameSize);
        success = true;
        Serial.printf("Device name loaded from Preferences: %s\n", name);
    } else {
        Serial.println("Device name buffer too small");
        // Use default
        String defaultName = "env_logger";
        if (defaultName.length() < nameSize) {
            defaultName.toCharArray(name, nameSize);
            success = true;
        }
    }

    preferences.end();
    return success;
}

bool saveWiFiCredentials(const char* ssid, const char* password) {
    preferences.begin("env_logger", false);

    // Validate inputs
    if (strlen(ssid) == 0 || strlen(ssid) > 32) {
        Serial.println("Invalid WiFi SSID length");
        preferences.end();
        return false;
    }

    if (strlen(password) > 63) {
        Serial.println("Invalid WiFi password length");
        preferences.end();
        return false;
    }

    // Save credentials
    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_pass", password);

    Serial.println("WiFi credentials saved to Preferences");
    preferences.end();
    return true;
}

bool saveMQTTCredentials(const char* broker, uint16_t port,
                        const char* user, const char* password) {
    preferences.begin("env_logger", false);

    // Validate inputs
    if (strlen(broker) == 0) {
        Serial.println("Invalid MQTT broker");
        preferences.end();
        return false;
    }

    // Save credentials
    preferences.putString("mqtt_broker", broker);
    preferences.putUShort("mqtt_port", port);
    preferences.putString("mqtt_user", user);
    preferences.putString("mqtt_pass", password);

    Serial.println("MQTT credentials saved to Preferences");
    preferences.end();
    return true;
}

bool saveDeviceName(const char* name) {
    preferences.begin("env_logger", false);

    // Validate input
    if (strlen(name) == 0) {
        name = "env_logger";
    }

    // Sanitize for MQTT topic validity (remove invalid characters)
    String cleanName = "";
    for (uint8_t i = 0; i < strlen(name); i++) {
        char c = name[i];
        // Allow alphanumeric, hyphen, underscore, dot (valid for MQTT topics)
        if (isalnum(c) || c == '-' || c == '_' || c == '.') {
            cleanName += c;
        } else {
            // Replace invalid chars with underscore
            cleanName += '_';
        }
    }

    // Ensure not empty after sanitization
    if (cleanName.length() == 0) {
        cleanName = "env_logger";
    }

    preferences.putString("device_name", cleanName.c_str());
    Serial.printf("Device name saved to Preferences: %s\n", cleanName.c_str());
    preferences.end();
    return true;
}

bool clearCredentials() {
    preferences.begin("env_logger", false);
    preferences.clear();
    preferences.end();
    Serial.println("All credentials cleared from Preferences");
    return true;
}

// WiFiManager object and callbacks
WiFiManager wifiManager;

// Flag to track if we're in configuration mode
bool configModeActive = false;

// WiFiManager configuration mode callback
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered configuration mode");
    Serial.print("Config SSID: ");
    Serial.println(myWiFiManager->getConfigAPSSID());
    Serial.print("Config IP: ");
    Serial.println(WiFi.softAPIP());

    // Set custom AP SSID to include device name for easier identification
    char apName[32];
    char deviceName[32] = "env_logger"; // Default
    loadDeviceName(deviceName, sizeof(deviceName));
    snprintf(apName, sizeof(apName), "ESP_%s_%06X", deviceName, ESP.getChipId() & 0xFFFFFF);
    WiFi.softAP(apName);

    configModeActive = true;

    // Optional: Disable SSID broadcast for privacy (uncomment if desired)
    // WiFi.softAPsetHidden(true);
}

// WiFiManager save callback
void saveConfigCallback() {
    Serial.println("Configuration saved");

    // Save custom parameters
    // Note: WiFiManager automatically saves built-in parameters (SSID, password)
    // We need to save our custom parameters manually

    // Get custom parameters (if any were added)
    // For now, we rely on WiFiManager's built-in parameter handling
    // Custom parameters would be retrieved here if we added them

    // Signal that we should restart to apply new settings
    ESP.restart();
}

// WiFiManager connection callback
void connectCallback() {
    Serial.println("Connected to WiFi via WiFiManager");
    configModeActive = false;
}

// Function to start configuration mode
void startConfigurationMode() {
    Serial.println("Starting configuration mode");

    // Set callbacks
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    // wifiManager.setConnectCallback(connectCallback); // Optional

    // Set custom parameters for MQTT and device name
    // WiFiManagerParameter custom_mqtt_broker("server", "MQTT Broker", mqtt_broker, 40);
    // WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
    // WiFiManagerParameter custom_mqtt_user("user", "MQTT User", mqtt_user, 32);
    // WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", mqtt_pass, 32);
    // WiFiManagerParameter custom_device_name("name", "Device Name", device_name, 32);

    // Add parameters to WiFiManager
    // wifiManager.addParameter(&custom_mqtt_broker);
    // wifiManager.addParameter(&custom_mqtt_port);
    // wifiManager.addParameter(&custom_mqtt_user);
    // wifiManager.addParameter(&custom_mqtt_pass);
    // wifiManager.addParameter(&custom_device_name);

    // Set timeout for configuration mode (3 minutes)
    wifiManager.setConfigPortalTimeout(180);

    // Start the configuration portal
    if (!wifiManager.startConfigPortal()) {
        Serial.println("Failed to start config portal. Rebooting...");
        delay(3000);
        ESP.restart();
    }
}

// Device ID generation functions
String getDeviceId() {
    char deviceName[32] = "env_logger";
    loadDeviceName(deviceName, sizeof(deviceName));
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(deviceName) + "-" + String(macStr);
}

// Helper function for MAC address formatting (used by getDeviceId)
String formatMacAddress(uint8_t* mac) {
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

// MQTT topic generation functions
String getTemperatureTopic() {
    return getDeviceId() + "/temperature";
}

String getHumidityTopic() {
    return getDeviceId() + "/humidity";
}

String getPressureTopic() {
    return getDeviceId() + "/pressure";
}

String getTvocTopic() {
    return getDeviceId() + "/tvoc";
}

String getEco2Topic() {
    return getDeviceId() + "/eco2";
}

String getStatusTopic() {
    return getDeviceId() + "/status";
}