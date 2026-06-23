/**
 * Example configuration file for MCU Environmental Data Logger
 * Copy this file to config.h and modify the values for your setup
 */

#ifndef CONFIG_EXAMPLE_H
#define CONFIG_EXAMPLE_H

// ===== WiFi Settings =====
#define WIFI_SSID "MyHomeNetwork"
#define WIFI_PASSWORD "supersecretpassword123"
#define WIFI_TIMEOUT_MS 15000 // 15 seconds timeout

// ===== MQTT Settings =====
#define MQTT_BROKER "192.168.1.100" // IP of your MQTT broker (Home Assistant, etc.)
#define MQTT_PORT 1883
#define MQTT_USER "mqtt_user"                  // Leave empty string "" if not used
#define MQTT_PASSWORD "mqtt_password"          // Leave empty string "" if not used
#define DEVICE_HOSTNAME "envlogger_livingroom" // Must be unique on your network

// MQTT Topics (change if needed)
#define MQTT_TOPIC_TEMPERATURE "home/livingroom/temperature"
#define MQTT_TOPIC_HUMIDITY "home/livingroom/humidity"
#define MQTT_TOPIC_PRESSURE "home/livingroom/pressure"
#define MQTT_TOPIC_TVOC "home/livingroom/tvoc"
#define MQTT_TOPIC_ECO2 "home/livingroom/eco2"
#define MQTT_TOPIC_STATUS "home/livingroom/status"

// Enable/disable MQTT (set to 0 to disable for SD-only logging)
#define ENABLE_MQTT 1

// ===== Sensor Settings =====
// BME280 I2C address (0x76 or 0x77)
#define BME280_I2C_ADDR 0x76

// CCS811 wake pin (optional - connect to GPIO for manual control)
// Set to -1 if not connected
#define CCS811_WAKE_PIN -1

// ===== Storage Settings =====
#define SD_CS_PIN 5 // GPIO5 for ESP32 SD card CS

// ===== Power Management =====
// Measurement interval in milliseconds
// 300000 = 5 minutes, 600000 = 10 minutes, etc.
#define MEASUREMENT_INTERVAL_MS 300000 // 5 minutes

// ===== Optional Features =====
// Uncomment to enable status LED (requires WS2812B on GPIO)
// #define USE_STATUS_LED        1
// #define STATUS_LED_PIN        27
// #define STATUS_LED_COUNT      1

// Uncomment to enable firmware version reporting via MQTT
#define FIRMWARE_VERSION "1.0.0"

#endif // CONFIG_EXAMPLE_H
