#include "StorageManager.h"

namespace {
constexpr char kNamespace[] = "smartbasket";
}

void StorageManager::begin() {
  preferences_.begin(kNamespace, false);
}

void StorageManager::load(AppSettings& settings) {
  preferences_.getBytes("settings", &settings, sizeof(settings));
}

void StorageManager::save(const AppSettings& settings) {
  preferences_.putBytes("settings", &settings, sizeof(settings));
}
