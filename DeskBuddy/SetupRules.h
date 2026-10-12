#pragma once
#include <Arduino.h>
#include <time.h>

struct ClockZone { const char* id; const char* label; const char* rule; };
static const ClockZone CLOCK_ZONES[] = {
  {"America/Chicago", "Central", "CST6CDT,M3.2.0,M11.1.0"},
  {"America/New_York", "Eastern", "EST5EDT,M3.2.0,M11.1.0"},
  {"America/Denver", "Mountain", "MST7MDT,M3.2.0,M11.1.0"},
  {"America/Los_Angeles", "Pacific", "PST8PDT,M3.2.0,M11.1.0"},
  {"America/Phoenix", "Arizona (no DST)", "MST7"},
  {"Pacific/Honolulu", "Hawaii", "HST10"},
  {"Etc/UTC", "UTC", "UTC0"}
};
constexpr uint8_t ZONE_COUNT = sizeof(CLOCK_ZONES) / sizeof(CLOCK_ZONES[0]);
inline int zoneIndex(const String& id) {
  for (int i = 0; i < ZONE_COUNT; ++i) if (id == CLOCK_ZONES[i].id) return i;
  return -1;
}
inline bool validNetwork(const String& ssid, const String& password, bool openNetwork) {
  if (ssid.length() < 1 || ssid.length() > 32 || memchr(ssid.c_str(), 0, ssid.length())) return false;
  if (openNetwork) return password.isEmpty();
  if (memchr(password.c_str(), 0, password.length())) return false;
  if (password.length() >= 8 && password.length() <= 63) return true;
  if (password.length() != 64) return false;
  for (unsigned i = 0; i < 64; ++i) if (!isxdigit((unsigned char)password[i])) return false;
  return true;
}

inline unsigned setupSelfTest() {
  unsigned failures = 0;
  auto check = [&](bool value) { if (!value) ++failures; };
  check(validNetwork("Home", "example-pass", false));
  check(!validNetwork("", "example-pass", false));
  check(!validNetwork("123456789012345678901234567890123", "example-pass", false));
  check(!validNetwork("Home", "short", false));
  check(validNetwork("Open", "", true));
  check(!validNetwork("Home", "", false));
  check(!validNetwork("Open", "example-pass", true));
  check(validNetwork("Home", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", false));
  check(!validNetwork("Home", "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", false));
  check(zoneIndex("America/Chicago") == 0 && zoneIndex("invalid") == -1);
  setenv("TZ", CLOCK_ZONES[0].rule, 1); tzset();
  const time_t epochs[] = {1772956799, 1772956800, 1793516399, 1793516400};
  const int hours[] = {1, 3, 1, 1}, minutes[] = {59, 0, 59, 0}, dst[] = {0, 1, 1, 0};
  for (int i = 0; i < 4; ++i) {
    struct tm local{}; localtime_r(&epochs[i], &local);
    check(local.tm_hour == hours[i] && local.tm_min == minutes[i] && local.tm_isdst == dst[i]);
  }
  return failures;
}
