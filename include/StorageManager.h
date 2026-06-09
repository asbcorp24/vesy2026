#pragma once

#include <Preferences.h>

#include "Types.h"

class StorageManager {
 public:
  void begin();
  void load(AppSettings& settings);
  void save(const AppSettings& settings);

 private:
  Preferences preferences_;
};
