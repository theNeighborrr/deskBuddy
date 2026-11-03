#pragma once
#include <Arduino.h>
#include "AppSettings.h"
struct ForecastDay { int64_t date;float high,low;int16_t code,rain; };
struct WeatherReading {
 bool available;
 float temperature,feelsLike,high,low,wind;
 int code;
 bool day;
 int64_t observedAt,fetchedAt;
 float latitude,longitude;
 uint8_t days;
 int32_t utcOffset;
 ForecastDay forecast[7];
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
