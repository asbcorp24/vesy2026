#pragma once

#include <Arduino.h>

namespace Pins {
constexpr uint8_t HX711_DOUT[4] = {32, 33, 25, 26};
constexpr uint8_t HX711_SCK[4] = {4, 16, 17, 5};
constexpr uint8_t DHT_PIN = 27;
constexpr uint8_t BUTTON_MENU = 18;
constexpr uint8_t BUTTON_SELECT = 19;
constexpr uint8_t I2C_SDA = 21;
constexpr uint8_t I2C_SCL = 22;
}  // namespace Pins

namespace Defaults {
constexpr char AP_SSID[] = "SmartBasket_AP";
constexpr char AP_PASSWORD[] = "12345678";
constexpr char MQTT_TOPIC_STATE[] = "smartbasket/state";
constexpr char MQTT_TOPIC_COMMAND[] = "smartbasket/command";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t MENU_TIMEOUT_MS = 120000;
constexpr uint32_t SENSOR_POLL_MS = 500;
constexpr uint32_t DISPLAY_REFRESH_MS = 250;
constexpr uint32_t MQTT_RECONNECT_MS = 5000;
constexpr uint32_t CLIMATE_POLL_MS = 3000;
constexpr float MIN_TILT_PROJECTION = 0.2F;
}  // namespace Defaults

namespace DisplayConfig {
constexpr uint8_t I2C_ADDRESS = 0x3C;
constexpr bool USE_SH1106 = false;
}  // namespace DisplayConfig

namespace ImuConfig {
constexpr uint8_t MPU6050_ADDRESS = 0x68;
constexpr uint8_t PWR_MGMT_1_REGISTER = 0x6B;
constexpr uint8_t ACCEL_XOUT_H_REGISTER = 0x3B;
constexpr float ACCEL_SCALE = 16384.0F;
constexpr float TILT_SMOOTHING = 0.2F;
}  // namespace ImuConfig
