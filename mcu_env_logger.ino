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
#include "config.h"

// Optional: Status LED (uncomment if using)
// #include <Adafruit_NeoPixel.h>
// #define STATUS_LED_PIN    27
// #define STATUS_LED_COUNT  1
// Adafruit_NeoPixel statusLED(STATUS_LED_COUNT, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);

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

    // Initialize components
    Serial.println("Initializing sensors...");
    sensorsInitialized = initializeSensors();

    Serial.println("Initializing SD card...");
    sdInitialized = initializeSD();

    Serial.println("Connecting to WiFi...");
    wifiConnected = connectWiFi();

    if (wifiConnected)
    {
        Serial.println("Connecting to MQTT...");
        mqttConnected = connectMQTT();

        // Setup OTA updates
        ArduinoOTA.setHostname(DEVICE_HOSTNAME);
        ArduinoOTA.onStart([]()
                           { Serial.println("OTA Start"); });
        ArduinoOTA.onEnd([]()
                         { Serial.println("\nOTA End"); });
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
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi");
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
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
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

    Serial.print("Connecting to MQTT broker ");
    Serial.print(MQTT_BROKER);
    Serial.print(":");
    Serial.println(MQTT_PORT);

    if (mqttClient.connect(DEVICE_HOSTNAME, MQTT_USER, MQTT_PASSWORD))
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
    mqttClient.publish(MQTT_TOPIC_TEMPERATURE, String(temp).c_str(), true);
    mqttClient.publish(MQTT_TOPIC_HUMIDITY, String(hum).c_str(), true);
    mqttClient.publish(MQTT_TOPIC_PRESSURE, String(pres).c_str(), true);
    mqttClient.publish(MQTT_TOPIC_TVOC, String(tvoc).c_str(), true);
    mqttClient.publish(MQTT_TOPIC_ECO2, String(eco2).c_str(), true);

    // Publish status
    mqttClient.publish(MQTT_TOPIC_STATUS, "ok", true);

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
