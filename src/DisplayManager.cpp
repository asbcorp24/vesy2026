#include "DisplayManager.h"

DisplayManager::DisplayManager() : display_(U8G2_R0, U8X8_PIN_NONE) {}

void DisplayManager::begin() {
  display_.setI2CAddress(DisplayConfig::I2C_ADDRESS << 1);
  display_.begin();
  display_.enableUTF8Print();
}

void DisplayManager::render(const SensorReadings& readings, const AppSettings& settings, bool wifiConnected, bool mqttConnected) {
  display_.clearBuffer();
  display_.setFont(u8g2_font_6x12_t_cyrillic);
  display_.setCursor(0, 10);

  display_.print("SmartBasket");

  display_.setCursor(0, 24);
  display_.print("Вес: ");
  display_.print(readings.totalWeight, 1);
  display_.print(" г");

  display_.setCursor(0, 36);
  display_.print("Кол-во: ");
  if (settings.productMode == ProductMode::Pieces) {
    display_.print(readings.quantity);
  } else {
    display_.print("-");
  }

  display_.setCursor(0, 48);
  display_.print("T/H: ");
  display_.print(readings.temperature, 1);
  display_.print("/");
  display_.print(readings.humidity, 0);

  display_.setCursor(0, 60);
  display_.print(wifiConnected ? "WiFi" : "AP");
  display_.print(" ");
  display_.print(mqttConnected ? "MQTT" : "--");
  display_.print(" ");
  display_.print(readings.imuReady ? "IMU" : "NOIMU");

  display_.sendBuffer();
}

void DisplayManager::renderMenu(const char* title, const char* line1, const char* line2, const char* footer) {
  display_.clearBuffer();
  display_.setFont(u8g2_font_6x12_t_cyrillic);
  display_.setCursor(0, 10);

  display_.print(title);

  display_.setCursor(0, 28);
  display_.print(line1);

  display_.setCursor(0, 42);
  display_.print(line2);

  display_.setCursor(0, 60);
  display_.print(footer);

  display_.sendBuffer();
}
