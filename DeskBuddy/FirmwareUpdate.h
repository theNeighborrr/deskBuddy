#pragma once
#include <Arduino.h>
class WebServer;
namespace FirmwareUpdate {
void begin();
void upload(WebServer& server,bool authorized);
bool accepted();
bool busy();
const char* message();
unsigned percent();
void tick(bool healthy);
bool rollback();
bool canRollback();
void failBoot();
}
