#pragma once
#include "AppSettings.h"
namespace PageControl {
void sync();
bool command(const String& action,int app=-1);
void tick(uint32_t now,bool blocked);
uint8_t position();
uint8_t app();
uint8_t avatar();
bool paused();
unsigned selfTest();
}
