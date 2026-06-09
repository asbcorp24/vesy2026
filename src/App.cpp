#include "App.h"

#include "Config.h"

namespace {
float absoluteDiff(float a, float b) {
  return fabsf(a - b);
}

float clampMin(float value, float minimum) {
  return value < minimum ? minimum : value;
}

const char* menuItemLabel(App::MenuItem item) {
  switch (item) {
    case App::MenuItem::Status:
      return "Статус";
    case App::MenuItem::TareAll:
      return "Тара всех";
    case App::MenuItem::TareCorner:
      return "Тара угла";
    case App::MenuItem::CornerSelect:
      return "Выбор угла";
    case App::MenuItem::RefWeightUp:
      return "Вес +";
    case App::MenuItem::RefWeightDown:
      return "Вес -";
    case App::MenuItem::CalibrateCorner:
      return "Калибровать";
    case App::MenuItem::SaveCalibration:
      return "Сохранить";
    case App::MenuItem::ToggleMode:
      return "Режим";
    case App::MenuItem::Exit:
      return "Выход";
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

  climate_.begin();
  scales_.begin(settings_);
  display_.begin();

  connectivity_.begin(
      settings_,
      [this]() { return readings_; },
      [this](const AppSettings& settings) {
        settings_ = settings;
        storage_.save(settings_);
        scales_.updateSettings(settings_);
      },
      [this]() { scales_.tare(); });

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
      setStatusMessage("Экран статуса");
      break;
    case MenuItem::TareAll:
      scales_.tare();
      setStatusMessage("Тара всех датч.");
      break;
    case MenuItem::TareCorner:
      scales_.tare(calibrationCorner_);
      setStatusMessage("Тара угла ок");
      break;
    case MenuItem::CornerSelect:
      calibrationCorner_ = (calibrationCorner_ + 1) % 4;
      setStatusMessage("Выбран нов. угол");
      break;
    case MenuItem::RefWeightUp:
      settings_.calibrationReferenceWeight += settings_.calibrationReferenceWeight < 500.0F ? 50.0F : 100.0F;
      setStatusMessage("Опорный вес +");
      break;
    case MenuItem::RefWeightDown:
      settings_.calibrationReferenceWeight = clampMin(
          settings_.calibrationReferenceWeight - (settings_.calibrationReferenceWeight <= 500.0F ? 50.0F : 100.0F), 10.0F);
      setStatusMessage("Опорный вес -");
      break;
    case MenuItem::CalibrateCorner: {
      float factor = 0.0F;
      if (scales_.calibrate(calibrationCorner_, settings_.calibrationReferenceWeight, factor)) {
        settings_.calibration[calibrationCorner_] = factor;
        scales_.updateSettings(settings_);
        calibrationDirty_ = true;
        setStatusMessage("Калибровка ок");
      } else {
        setStatusMessage("Проверьте груз");
      }
      break;
    }
    case MenuItem::SaveCalibration:
      storage_.save(settings_);
      calibrationDirty_ = false;
      setStatusMessage("Сохранено в NVS");
      break;
    case MenuItem::ToggleMode:
      settings_.productMode = settings_.productMode == ProductMode::Mass ? ProductMode::Pieces : ProductMode::Mass;
      storage_.save(settings_);
      setStatusMessage(settings_.productMode == ProductMode::Mass ? "Режим: масса" : "Режим: штуки");
      break;
    case MenuItem::Exit:
      menuActive_ = false;
      setStatusMessage("Выход из меню");
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
      snprintf(line2, sizeof(line2), "Вес %.1fг Кол %lu", readings_.totalWeight, static_cast<unsigned long>(readings_.quantity));
      break;
    case MenuItem::TareAll:
      snprintf(line2, sizeof(line2), "Обнулить все углы");
      break;
    case MenuItem::TareCorner:
      snprintf(line2, sizeof(line2), "Угол %u без груза", calibrationCorner_ + 1);
      break;
    case MenuItem::CornerSelect:
      snprintf(line2, sizeof(line2), "Текущий угол: %u", calibrationCorner_ + 1);
      break;
    case MenuItem::RefWeightUp:
    case MenuItem::RefWeightDown:
      snprintf(line2, sizeof(line2), "Опора %.0f г", settings_.calibrationReferenceWeight);
      break;
    case MenuItem::CalibrateCorner:
      snprintf(line2, sizeof(line2), "Угол %u груз %.0fг", calibrationCorner_ + 1, settings_.calibrationReferenceWeight);
      break;
    case MenuItem::SaveCalibration:
      snprintf(line2, sizeof(line2), calibrationDirty_ ? "Есть несохр. изм." : "Все сохранено");
      break;
    case MenuItem::ToggleMode:
      snprintf(line2, sizeof(line2), "Сейчас: %s", settings_.productMode == ProductMode::Mass ? "масса" : "штуки");
      break;
    case MenuItem::Exit:
      snprintf(line2, sizeof(line2), "Закрыть меню");
      break;
    case MenuItem::Count:
      line2[0] = '\0';
      break;
  }

  if (millis() < statusMessageUntilMs_ && statusMessage_[0] != '\0') {
    snprintf(footer, sizeof(footer), "%s", statusMessage_);
  } else {
    snprintf(footer, sizeof(footer), "B1 след. B2 ок");
  }

  display_.renderMenu("Меню", line1, line2, footer);
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
