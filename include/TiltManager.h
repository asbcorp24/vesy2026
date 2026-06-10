#pragma once

#include <Arduino.h>

class TiltManager {
 public:
  void begin();
  bool poll(float& rawTiltXDegrees, float& rawTiltYDegrees);
  bool isReady() const;

 private:
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readAccelerometer(int16_t& ax, int16_t& ay, int16_t& az);

  bool ready_ = false;
  bool hasSample_ = false;
  float filteredTiltX_ = 0.0F;
  float filteredTiltY_ = 0.0F;
};
