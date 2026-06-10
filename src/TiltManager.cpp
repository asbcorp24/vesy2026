#include "TiltManager.h"

#include <Wire.h>

#include "Config.h"

namespace {
constexpr float kRadiansToDegrees = 57.2957795F;
}

void TiltManager::begin() {
  ready_ = writeRegister(ImuConfig::PWR_MGMT_1_REGISTER, 0x00);
}

bool TiltManager::poll(float& rawTiltXDegrees, float& rawTiltYDegrees) {
  int16_t ax = 0;
  int16_t ay = 0;
  int16_t az = 0;
  if (!ready_ || !readAccelerometer(ax, ay, az)) {
    return false;
  }

  const float accelX = static_cast<float>(ax) / ImuConfig::ACCEL_SCALE;
  const float accelY = static_cast<float>(ay) / ImuConfig::ACCEL_SCALE;
  const float accelZ = static_cast<float>(az) / ImuConfig::ACCEL_SCALE;

  const float rollDegrees = atan2f(accelY, accelZ) * kRadiansToDegrees;
  const float pitchDegrees = atan2f(-accelX, sqrtf(accelY * accelY + accelZ * accelZ)) * kRadiansToDegrees;

  if (!hasSample_) {
    filteredTiltX_ = rollDegrees;
    filteredTiltY_ = pitchDegrees;
    hasSample_ = true;
  } else {
    filteredTiltX_ += (rollDegrees - filteredTiltX_) * ImuConfig::TILT_SMOOTHING;
    filteredTiltY_ += (pitchDegrees - filteredTiltY_) * ImuConfig::TILT_SMOOTHING;
  }

  rawTiltXDegrees = filteredTiltX_;
  rawTiltYDegrees = filteredTiltY_;
  return true;
}

bool TiltManager::isReady() const {
  return ready_;
}

bool TiltManager::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(ImuConfig::MPU6050_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool TiltManager::readAccelerometer(int16_t& ax, int16_t& ay, int16_t& az) {
  Wire.beginTransmission(ImuConfig::MPU6050_ADDRESS);
  Wire.write(ImuConfig::ACCEL_XOUT_H_REGISTER);
  if (Wire.endTransmission(false) != 0) {
    ready_ = false;
    return false;
  }

  const uint8_t bytesRequested = 6;
  const uint8_t bytesRead = Wire.requestFrom(static_cast<int>(ImuConfig::MPU6050_ADDRESS), static_cast<int>(bytesRequested));
  if (bytesRead != bytesRequested) {
    ready_ = false;
    return false;
  }

  ax = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  ay = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  az = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  return true;
}
