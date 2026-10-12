#pragma once
#include <Arduino.h>
#include "AppSettings.h"
struct WeatherReading {
 bool available;
 float temperature,feelsLike,high,low,wind;
 int code;
 bool day;
 int64_t observedAt,fetchedAt;
 float latitude,longitude;
};
namespace BuddyWeather {
bool begin();
void tick(bool online,bool synchronized,bool updateBusy);
WeatherReading reading();
bool busy();
bool search(const String& query);
String searchResult();
void refresh();
const char* description(int code);
unsigned selfTest();
}
