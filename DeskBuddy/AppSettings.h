#pragma once
#include <Arduino.h>
#include "SetupRules.h"
constexpr char FIRMWARE_VERSION[]="0.5.0";
constexpr uint8_t APP_COUNT=4,AVATAR_COUNT=7;
enum AppId : uint8_t { PAL=0, CLOCK=1, WEATHER=2, WIFI_SIGNAL=3 };
struct AppConfig {
  uint32_t magic;
  char name[19],city[33];
  float latitude,longitude;
  uint16_t rotationSeconds;
  uint8_t order[APP_COUNT],count,zone,h24,fahrenheit,hasLocation,avatar;
};
namespace AppSettings {
bool begin(uint8_t oldZone,bool old24);
const AppConfig& get();
bool valid(const AppConfig& value);
bool save(const AppConfig& value);
uint32_t revision();
unsigned selfTest();
}
