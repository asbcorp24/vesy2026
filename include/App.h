#pragma once

#include <Arduino.h>

#include "ClimateManager.h"
#include "ConnectivityManager.h"
#include "DisplayManager.h"
#include "ScaleManager.h"
#include "StorageManager.h"
#include "TiltManager.h"
#include "Types.h"

class App {
 public:
  enum class MenuItem : uint8_t {
    Status = 0,
    TareAll,
    TareCorner,
    CornerSelect,
    RefWeightUp,
    RefWeightDown,
    CalibrateCorner,
    SaveCalibration,
    ZeroTilt,
    ToggleMode,
    Exit,
    Count
  };

  void begin();
  void loop();

 private:
  void updateSensors();
  void handleButtons();
  void executeMenuAction();
  void renderMenu();
  void setStatusMessage(const char* message);
  bool shouldPublish() const;
  void zeroTilt();

  StorageManager storage_;
  ScaleManager scales_;
  TiltManager tilt_;
  ClimateManager climate_;
  DisplayManager display_;
  ConnectivityManager connectivity_;
  AppSettings settings_;
  SensorReadings readings_;
  SensorReadings publishedReadings_;
  unsigned long lastSensorPollMs_ = 0;
  unsigned long lastDisplayMs_ = 0;
  unsigned long lastMenuActionMs_ = 0;
  unsigned long statusMessageUntilMs_ = 0;
  float lastRawTiltX_ = 0.0F;
  float lastRawTiltY_ = 0.0F;
  bool sensorUpdated_ = false;
  bool calibrationDirty_ = false;
  bool menuActive_ = false;
  bool imuSampleReady_ = false;
  uint8_t menuIndex_ = 0;
  uint8_t calibrationCorner_ = 0;
  char statusMessage_[32] = "";
};
