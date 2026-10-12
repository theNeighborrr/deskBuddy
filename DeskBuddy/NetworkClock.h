#pragma once
#include <Arduino.h>
#include <time.h>

namespace BuddyNetwork {
void begin();
void tick();
void openSetup();
void closeSetup();
bool inSetup();
bool connected();
bool configured();
bool clockReady();
bool hour24();
bool takeFinished();
const char* zoneLabel();
const char* setupName();
const char* setupPassword();
const char* setupStatus();
bool localTime(struct tm& value);
const char* dashboardPin();
String dashboardAddress();
bool healthy();
void usbSession();
void testOffline();
}
