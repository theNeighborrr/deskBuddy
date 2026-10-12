#pragma once
#include <Arduino.h>
namespace SignalMeter {
constexpr uint8_t HISTORY=45;
struct Reading {bool online=false,available=false;int rssi=0;uint8_t bars=0;};
constexpr uint8_t barsFor(int dbm){return dbm>=-55?4:dbm>=-67?3:dbm>=-75?2:1;}
const char* quality(const Reading& value);
void tick(bool online);
Reading reading();
int history(uint8_t index); // Oldest to newest; zero means unavailable.
unsigned selfTest();
}
