#pragma once

#include <U8g2lib.h>

#include "Config.h"
#include "Types.h"

class DisplayManager {
 public:
  DisplayManager();
  void begin();
  void render(const SensorReadings& readings, const AppSettings& settings, bool wifiConnected, bool mqttConnected);
  void renderMenu(const char* title, const char* line1, const char* line2, const char* footer);

 private:
  using OledDriver = U8G2_SSD1306_128X64_NONAME_F_HW_I2C;
  OledDriver display_;
};
