#include "App.h"

#include <Wire.h>

#include "Config.h"

namespace {
float absoluteDiff(float a, float b) {
  return fabsf(a - b);
}

float clampRange(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  return value > maximum ? maximum : value;
}

float clampMin(float value, float minimum) {
  return value < minimum ? minimum : value;
}

const char* menuItemLabel(App::MenuItem item) {
  switch (item) {
    case App::MenuItem::Status:
      return "Status";
    case App::MenuItem::TareAll:
      return "Tare all";
    case App::MenuItem::TareCorner:
      return "Tare corner";
    case App::MenuItem::CornerSelect:
      return "Pick corner";
    case App::MenuItem::RefWeightUp:
      return "Ref +";
    case App::MenuItem::RefWeightDown:
      return "Ref -";
    case App::MenuItem::CalibrateCorner:
      return "Calibrate";
    case App::MenuItem::SaveCalibration:
      return "Save";
    case App::MenuItem::ZeroTilt:
      return "Zero IMU";
    case App::MenuItem::ToggleMode:
      return "Mode";
    case App::MenuItem::Exit:
      return "Exit";
    case App::MenuItem::Count:
      return "";
  }

  return "";
}
}  // namespace

void App::begin() {
  Serial.begin(115200);
  pinMode(Pins::BUTTON_MENU, INPUT_PULLUP);
  pinMode(Pins::BUTTON_SELECT, INPUT_PULLUP);

  storage_.begin();
  storage_.load(settings_);

  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);
  climate_.begin();
  scales_.begin(settings_);
  tilt_.begin();
  display_.begin();

  connectivity_.begin(
      settings_,
      [this]() { return readings_; },
      [this](const AppSettings& settings) {
        settings_ = settings;
        storage_.save(settings_);
        scales_.updateSettings(settings_);
      },
      [this]() { scales_.tare(); },
      [this]() { zeroTilt(); });

  updateSensors();
}

void App::loop() {
  connectivity_.loop(settings_);
  handleButtons();

  const unsigned long now = millis();
  if (now - lastSensorPollMs_ >= Defaults::SENSOR_POLL_MS) {
    updateSensors();
  }

  if (now - lastDisplayMs_ >= Defaults::DISPLAY_REFRESH_MS) {
    if (menuActive_) {
      renderMenu();
    } else {
      display_.render(readings_, settings_, connectivity_.isWifiConnected(), connectivity_.isMqttConnected());
    }
    lastDisplayMs_ = now;
  }

  if (sensorUpdated_ && shouldPublish()) {
    connectivity_.publishState(readings_, settings_, false);
    publishedReadings_ = readings_;
    sensorUpdated_ = false;
  }

  if (menuActive_ && now - lastMenuActionMs_ > Defaults::MENU_TIMEOUT_MS) {
    menuActive_ = false;
  }
}

void App::updateSensors() {
  const SensorReadings previous = readings_;
  readings_ = scales_.read(settings_);
  climate_.poll(readings_.temperature, readings_.humidity);

  float rawTiltXDegrees = 0.0F;
  float rawTiltYDegrees = 0.0F;
  readings_.imuReady = tilt_.poll(rawTiltXDegrees, rawTiltYDegrees);
  readings_.compensationFactor = 1.0F;

  if (readings_.imuReady) {
    imuSampleReady_ = true;
    lastRawTiltX_ = rawTiltXDegrees;
    lastRawTiltY_ = rawTiltYDegrees;
    readings_.tiltX = rawTiltXDegrees - settings_.tiltZeroX;
    readings_.tiltY = rawTiltYDegrees - settings_.tiltZeroY;

    const float rollRadians = readings_.tiltX * DEG_TO_RAD;
    const float pitchRadians = readings_.tiltY * DEG_TO_RAD;
    const float projection = clampRange(cosf(rollRadians) * cosf(pitchRadians), Defaults::MIN_TILT_PROJECTION, 1.0F);
    readings_.compensationFactor = 1.0F / projection;
    readings_.totalWeight = readings_.rawWeight * readings_.compensationFactor;
  }

  scales_.finalizeReadings(readings_, settings_);
  readings_.changed = absoluteDiff(readings_.totalWeight, previous.totalWeight) >= settings_.changeThreshold;
  lastSensorPollMs_ = millis();
  sensorUpdated_ = true;
}

void App::handleButtons() {
  static bool lastMenuPressed = false;
  static bool lastSelectPressed = false;

  const bool menuPressed = digitalRead(Pins::BUTTON_MENU) == LOW;
  const bool selectPressed = digitalRead(Pins::BUTTON_SELECT) == LOW;

  if (menuPressed && !lastMenuPressed) {
    lastMenuActionMs_ = millis();
    menuActive_ = true;
    menuIndex_ = (menuIndex_ + 1) % static_cast<uint8_t>(MenuItem::Count);
  }

  if (selectPressed && !lastSelectPressed) {
    lastMenuActionMs_ = millis();
    menuActive_ = true;
    executeMenuAction();
  }

  lastMenuPressed = menuPressed;
  lastSelectPressed = selectPressed;
}

void App::executeMenuAction() {
  const MenuItem item = static_cast<MenuItem>(menuIndex_);

  switch (item) {
    case MenuItem::Status:
      setStatusMessage("Status screen");
      break;
    case MenuItem::TareAll:
      scales_.tare();
      setStatusMessage("All cells tared");
      break;
    case MenuItem::TareCorner:
      scales_.tare(calibrationCorner_);
      setStatusMessage("Corner tared");
      break;
    case MenuItem::CornerSelect:
      calibrationCorner_ = (calibrationCorner_ + 1) % 4;
      setStatusMessage("Corner selected");
      break;
    case MenuItem::RefWeightUp:
      settings_.calibrationReferenceWeight += settings_.calibrationReferenceWeight < 500.0F ? 50.0F : 100.0F;
      setStatusMessage("Reference up");
      break;
    case MenuItem::RefWeightDown:
      settings_.calibrationReferenceWeight = clampMin(
          settings_.calibrationReferenceWeight - (settings_.calibrationReferenceWeight <= 500.0F ? 50.0F : 100.0F), 10.0F);
      setStatusMessage("Reference down");
      break;
    case MenuItem::CalibrateCorner: {
      float factor = 0.0F;
      if (scales_.calibrate(calibrationCorner_, settings_.calibrationReferenceWeight, factor)) {
        settings_.calibration[calibrationCorner_] = factor;
        scales_.updateSettings(settings_);
        calibrationDirty_ = true;
        setStatusMessage("Calibration ok");
      } else {
        setStatusMessage("Check reference");
      }
      break;
    }
    case MenuItem::SaveCalibration:
      storage_.save(settings_);
      calibrationDirty_ = false;
      setStatusMessage("Saved to NVS");
      break;
    case MenuItem::ZeroTilt:
      zeroTilt();
      break;
    case MenuItem::ToggleMode:
      settings_.productMode = settings_.productMode == ProductMode::Mass ? ProductMode::Pieces : ProductMode::Mass;
      storage_.save(settings_);
      setStatusMessage(settings_.productMode == ProductMode::Mass ? "Mode: mass" : "Mode: pieces");
      break;
    case MenuItem::Exit:
      menuActive_ = false;
      setStatusMessage("Menu closed");
      break;
    case MenuItem::Count:
      break;
  }
}

void App::renderMenu() {
  char line1[32];
  char line2[32];
  char footer[32];

  const MenuItem item = static_cast<MenuItem>(menuIndex_);
  snprintf(line1, sizeof(line1), "> %s", menuItemLabel(item));

  switch (item) {
    case MenuItem::Status:
      snprintf(line2, sizeof(line2), "Wt %.1fg Qty %lu", readings_.totalWeight, static_cast<unsigned long>(readings_.quantity));
      break;
    case MenuItem::TareAll:
      snprintf(line2, sizeof(line2), "Zero all corners");
      break;
    case MenuItem::TareCorner:
      snprintf(line2, sizeof(line2), "Corner %u empty", calibrationCorner_ + 1);
      break;
    case MenuItem::CornerSelect:
      snprintf(line2, sizeof(line2), "Current corner: %u", calibrationCorner_ + 1);
      break;
    case MenuItem::RefWeightUp:
    case MenuItem::RefWeightDown:
      snprintf(line2, sizeof(line2), "Reference %.0f g", settings_.calibrationReferenceWeight);
      break;
    case MenuItem::CalibrateCorner:
      snprintf(line2, sizeof(line2), "Corner %u load %.0fg", calibrationCorner_ + 1, settings_.calibrationReferenceWeight);
      break;
    case MenuItem::SaveCalibration:
      snprintf(line2, sizeof(line2), calibrationDirty_ ? "Unsaved changes" : "Saved");
      break;
    case MenuItem::ZeroTilt:
      if (imuSampleReady_) {
        snprintf(line2, sizeof(line2), "X %.1f Y %.1f", lastRawTiltX_, lastRawTiltY_);
      } else {
        snprintf(line2, sizeof(line2), "MPU-6050 missing");
      }
      break;
    case MenuItem::ToggleMode:
      snprintf(line2, sizeof(line2), "Current: %s", settings_.productMode == ProductMode::Mass ? "mass" : "pieces");
      break;
    case MenuItem::Exit:
      snprintf(line2, sizeof(line2), "Close menu");
      break;
    case MenuItem::Count:
      line2[0] = '\0';
      break;
  }

  if (millis() < statusMessageUntilMs_ && statusMessage_[0] != '\0') {
    snprintf(footer, sizeof(footer), "%s", statusMessage_);
  } else {
    snprintf(footer, sizeof(footer), "B1 next B2 ok");
  }

  display_.renderMenu("Menu", line1, line2, footer);
}

void App::setStatusMessage(const char* message) {
  strlcpy(statusMessage_, message, sizeof(statusMessage_));
  statusMessageUntilMs_ = millis() + 2000;
}

bool App::shouldPublish() const {
  if (!settings_.sendOnChangeOnly) {
    return true;
  }

  return absoluteDiff(readings_.totalWeight, publishedReadings_.totalWeight) >= settings_.changeThreshold;
}

void App::zeroTilt() {
  if (!imuSampleReady_) {
    setStatusMessage("MPU-6050 missing");
    return;
  }

  settings_.tiltZeroX = lastRawTiltX_;
  settings_.tiltZeroY = lastRawTiltY_;
  storage_.save(settings_);
  setStatusMessage("IMU zero saved");
}
