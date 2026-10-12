#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_timer.h>
#include <esp_system.h>
#include "TouchInput.h"
#include "TouchSelfTest.h"
#include "SetupRules.h"
#include "NetworkClock.h"
#include "AppSettings.h"
#include "Weather.h"
#include "FirmwareUpdate.h"
#include "Visuals.h"
#include "AvatarLogic.h"
#include "PageControl.h"
using namespace Visuals;
#include <esp_task_wdt.h>

SET_LOOP_TASK_STACK_SIZE(16384);

constexpr uint8_t LCD_POWER = 21, LCD_BACKLIGHT = 19, TOUCH_PIN = 32;
constexpr const char* VERSION = FIRMWARE_VERSION;
TFT_eSPI display;
TFT_eSprite frame(&display);
TouchInput input;
uint8_t rotation = 0;
bool inverted = true;
bool setupHoldFired = false;
uint32_t tapCount = 0, holdCount = 0, lastFrame = 0, lastLog = 0;
uint16_t rawTouch = 0;
uint32_t greetingUntil = 0;
uint32_t settingsRevision=UINT32_MAX;
bool startupHealthy=false;
// Sixteen bright colors in a compact 28.8 KB indexed framebuffer.

void centered(const char* text, int y, uint8_t font, uint16_t color, uint16_t bg) {
  frame.setTextColor(color, bg);
  frame.drawCentreString(text, 120, y, font);
}

uint16_t sampleTouch() {
  uint16_t a = touchRead(TOUCH_PIN), b = touchRead(TOUCH_PIN), c = touchRead(TOUCH_PIN);
  if (a > b) { uint16_t v = a; a = b; b = v; }
  if (b > c) { uint16_t v = b; b = c; c = v; }
  if (a > b) { uint16_t v = a; a = b; b = v; }
  return b;
}

void drawPage(uint32_t now) {
  frame.fillSprite(BG);
  if(FirmwareUpdate::busy()){
    frame.fillRect(0,0,240,3,LIME);
    centered("> SYSTEM UPDATE",18,1,LIME,BG);
    centered("Updating",55,4,INK,BG);
    centered(FirmwareUpdate::message(),100,1,INK,BG);
    frame.drawRoundRect(20,145,200,18,7,MUTED);
    frame.fillRoundRect(22,147,FirmwareUpdate::percent()*196/100,14,5,MINT);
    centered("Keep your buddy plugged in",190,1,MUTED,BG);
    frame.pushSprite(0,0);return;
  }
  if (BuddyNetwork::showingSetup()) {
    frame.fillRect(0,0,240,3,CYAN);
    if(BuddyNetwork::connected()){
      centered("Phone dashboard",12,4,MINT,BG);
      centered("On your home Wi-Fi, open",49,1,MUTED,BG);
      String address="http://"+BuddyNetwork::dashboardAddress();centered(address.c_str(),65,2,INK,BG);
      centered("Screen code",93,1,MUTED,BG);centered(BuddyNetwork::dashboardPin(),108,4,MINT,BG);
      centered("Or join this setup network:",145,1,MUTED,BG);
      centered(BuddyNetwork::setupName(),159,1,INK,BG);
      String pass="Password: "+String(BuddyNetwork::setupPassword());centered(pass.c_str(),174,1,MINT,BG);
      centered("Then open http://192.168.4.1",192,1,MUTED,BG);
      centered("Tap top: back to companion",219,1,MUTED,BG);
      frame.pushSprite(0,0);return;
    }
    centered("Wi-Fi setup", 10, 4, MINT, BG);
    centered(BuddyNetwork::setupStatus(), 44, 2, INK, BG);
    centered("1. Join this Wi-Fi on your phone", 76, 1, MUTED, BG);
    centered(BuddyNetwork::setupName(), 93, 2, INK, BG);
    centered("Password", 122, 1, MUTED, BG);
    centered(BuddyNetwork::setupPassword(), 137, 2, MINT, BG);
    centered("2. Open your browser", 168, 1, MUTED, BG);
    centered("http://192.168.4.1", 182, 2, INK, BG);
    centered("Tap top: use display / setup stays open", 217, 1, MUTED, BG);
    frame.pushSprite(0, 0);
    return;
  }
  const auto& cfg=AppSettings::get();
  uint8_t page=PageControl::position(),app=PageControl::app();
  shell(frame,cfg.name,app,page,cfg.count);
  if(app==PAL){
    avatar(frame,PageControl::avatar(),now);
    String caption=String("> ")+avatarCaption(PageControl::avatar());centered(caption.c_str(),205,1,CYAN,BG);
  }else if(app==CLOCK){
    frame.setTextColor(VIOLET,BG);frame.drawString(BuddyNetwork::zoneLabel(),16,56,2);
    struct tm local{};
    if(BuddyNetwork::localTime(local)){
      char timeText[12],dateText[40];
      snprintf(timeText,sizeof(timeText),"%02d:%02d",BuddyNetwork::hour24()?local.tm_hour:(local.tm_hour%12?local.tm_hour%12:12),local.tm_min);
      bigText(frame,timeText,16,91,5,INK);
      frame.setTextColor(VIOLET,BG);frame.drawString(BuddyNetwork::hour24()?"24H":local.tm_hour>=12?"PM":"AM",190,109,2);
      strftime(dateText,sizeof(dateText),"%a, %b %d",&local);frame.setTextColor(INK,BG);frame.drawString(dateText,16,143,2);
      for(int i=0;i<60;++i)frame.fillRect(16+i*3,177,2,8,i<=local.tm_sec?VIOLET:CARD);
      char seconds[8];snprintf(seconds,sizeof(seconds),"%02d",local.tm_sec);frame.setTextColor(VIOLET,BG);frame.drawRightString(seconds,225,175,1);
      centered(BuddyNetwork::connected()?"> time synchronized":"> offline / clock running",202,1,MUTED,BG);
    }else{
      centered(BuddyNetwork::connected()?"Syncing time...":"Time needs Wi-Fi",106,2,VIOLET,BG);
      centered("Waiting for internet time",145,1,MUTED,BG);
    }
  }else if(app==WEATHER){
    weatherPage(frame,now);
  }else if(app==WIFI_SIGNAL){signalPage(frame);}
  else if(app==WEEKLY){weeklyPage(frame,now);}
  footer(frame);
  if (input.down()) {
    uint32_t elapsed = now - input.pressedAt();
    uint16_t width = (uint16_t)(min(elapsed, uint32_t(5000)) * 212 / 5000);
    frame.fillRect(14, 237, width, 3, MINT);
  }
  frame.pushSprite(0, 0);
}

void applyGesture(Gesture event, const char* source) {
  if (event == Gesture::None) return;
  if(FirmwareUpdate::busy())return;
  if (BuddyNetwork::showingSetup()) { BuddyNetwork::hideSetup(); }
  else if (event == Gesture::Tap) { PageControl::command("next"); ++tapCount; }
  else { PageControl::command("home"); ++holdCount; }
  Serial.printf("[gesture] source=%s event=%s page=%u taps=%lu holds=%lu raw=%u baseline=%u\n",
      source, event == Gesture::Tap ? "tap" : "hold", PageControl::position(),
      (unsigned long)tapCount, (unsigned long)holdCount, rawTouch, input.baseline());
  drawPage(millis());
}

void calibrate() {
  uint16_t samples[80];
  for (int i = 0; i < 80; ++i) { samples[i] = sampleTouch(); delay(10); }
  for (int i = 1; i < 80; ++i) {
    uint16_t v = samples[i]; int j = i - 1;
    while (j >= 0 && samples[j] > v) { samples[j+1] = samples[j]; --j; }
    samples[j+1] = v;
  }
  input.begin(samples[60]); // upper quartile tolerates brief accidental touches
  Serial.printf("[touch] calibrated baseline=%u pin=%u\n", input.baseline(), TOUCH_PIN);
}

void serialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'n') applyGesture(Gesture::Tap, "serial");
    if (c == 'h') applyGesture(Gesture::Hold, "serial");
    if (c == 'r') { rotation = (rotation + 1) % 4; display.setRotation(rotation); }
    if (c == 'i') { inverted = !inverted; display.invertDisplay(inverted); }
    if (c == 'c') calibrate();
    if (c == 'w') BuddyNetwork::openSetup();
    if (c == 'd') BuddyNetwork::usbSession();
    if (c == 'o') BuddyNetwork::testOffline();
    if (c == 's') {
      if (BuddyNetwork::showingSetup()) { Serial.println("[screenshot] unavailable during private setup"); continue; }
      // Sixteen RGB565 palette entries, then packed high-nibble-first pixels.
      Serial.printf("\nSCREENSHOT 240 240 INDEX4 28832\n");
      for (int i=0;i<16;++i) { uint16_t color=frame.getPaletteColor(i);Serial.write(color >> 8);Serial.write(color & 255); }
      Serial.write((uint8_t*)frame.getPointer(), 240 * 240 / 2);
      Serial.println("\nEND_SCREENSHOT");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n[boot] DeskBuddy %s reset_reason=%d\n", VERSION, (int)esp_reset_reason());
  FirmwareUpdate::begin();
  unsigned failures = touchSelfTest();
  Serial.printf("[test] gestures=%s failures=%u\n", failures ? "FAIL" : "PASS", failures);
  unsigned setupFailures = setupSelfTest();
  Serial.printf("[test] setup_and_dst=%s failures=%u\n", setupFailures ? "FAIL" : "PASS", setupFailures);
  unsigned navFailures=PageControl::selfTest();
  Serial.printf("[test] navigation=%s failures=%u\n",navFailures?"FAIL":"PASS",navFailures);
  unsigned appFailures=AppSettings::selfTest(),weatherFailures=BuddyWeather::selfTest(),signalFailures=SignalMeter::selfTest(),avatarFailures=avatarLogicSelfTest();
  Serial.printf("[test] avatar_weather=%s failures=%u\n",avatarFailures?"FAIL":"PASS",avatarFailures);
  Serial.printf("[test] signal=%s failures=%u\n",signalFailures?"FAIL":"PASS",signalFailures);
  Serial.printf("[test] settings=%s weather=%s failures=%u\n",appFailures?"FAIL":"PASS",weatherFailures?"FAIL":"PASS",appFailures+weatherFailures);
  startupHealthy=!failures&&!setupFailures&&!appFailures&&!weatherFailures&&!signalFailures&&!avatarFailures&&!navFailures;
  digitalWrite(LCD_BACKLIGHT, HIGH); pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_POWER, LOW); pinMode(LCD_POWER, OUTPUT);
  delay(50);
  display.init();
  display.setRotation(rotation);
  display.invertDisplay(inverted);
  display.fillScreen(PALETTE[BG]);
  digitalWrite(LCD_BACKLIGHT, LOW);
  Serial.printf("[memory] before_frame heap=%u largest=%u\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  frame.setColorDepth(4);
  if (!frame.createSprite(240, 240)) {
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.drawString("Display memory error", 8, 100, 2);
    Serial.println("[fatal] framebuffer allocation failed");
    FirmwareUpdate::failBoot();
  }
  frame.createPalette(PALETTE, 16);
  frame.fillSprite(BG);
  frame.fillRect(0,0,240,3,CYAN);
  centered("> BOOTING COMPANION_",35,1,CYAN,BG);
  centered("Hello, human.", 80, 4, INK, BG);
  centered("Small screen. Bright company.",135,1,VIOLET,BG);
  centered("Keep the top clear for a moment", 180, 1, MUTED, BG);
  frame.pushSprite(0, 0);
  calibrate();
  BuddyNetwork::begin();
  PageControl::sync();
  startupHealthy=startupHealthy&&BuddyNetwork::healthy();
  if(!startupHealthy)FirmwareUpdate::failBoot();
  esp_task_wdt_config_t watchdog{.timeout_ms=20000,.idle_core_mask=0,.trigger_panic=true};
  if(esp_task_wdt_status(nullptr)==ESP_ERR_INVALID_STATE)esp_task_wdt_init(&watchdog);else esp_task_wdt_reconfigure(&watchdog);
  esp_task_wdt_add(nullptr);
  delay(500);
  drawPage(millis());
  Serial.printf("[ready] version=%s heap=%u min_heap=%u largest=%u psram=%u framebuffer=28800 pages=5\n",
      VERSION, ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap(), ESP.getPsramSize());
  Serial.println("[commands] n=next h=home w=dashboard s=screenshot d=private_USB_session o=test_outage");

}

void loop() {
  esp_task_wdt_reset();
  uint32_t now = millis();
  rawTouch = sampleTouch();
  applyGesture(input.update(rawTouch, now), "touch");
  if (!input.down()) setupHoldFired=false;
  if (input.activelyDown() && !setupHoldFired && uint32_t(now-input.pressedAt())>=5000) {
    setupHoldFired=true;
    BuddyNetwork::openSetup();
  }
  serialCommands();
  BuddyNetwork::tick();
  FirmwareUpdate::tick(startupHealthy&&ESP.getFreeHeap()>45000);
  if(AppSettings::revision()!=settingsRevision){
    settingsRevision=AppSettings::revision();PageControl::sync();
    uint16_t palette[16];for(int i=0;i<16;++i){uint16_t c=PALETTE[i];palette[i]=AppSettings::get().colorSwap?((c&31)<<11)|(c&0x7E0)|((c>>11)&31):c;}frame.createPalette(palette,16);
  }
  BuddyNetwork::takeFinished();
  now=millis();const auto& cfg=AppSettings::get();
  PageControl::tick(now,BuddyNetwork::showingSetup()||FirmwareUpdate::busy()||input.down());
  if (uint32_t(now - lastFrame) >= 100) { lastFrame = now; drawPage(now); }
  if (uint32_t(now - lastLog) >= 1000) {
    lastLog = now;
    Serial.printf("[status] ms=%lu page=%u raw=%u baseline=%u down=%u heap=%u min_heap=%u wifi=%u synced=%u setup=%u weather=%u app=%u\n",
        (unsigned long)now, PageControl::position(), rawTouch, input.baseline(), input.down(),
        ESP.getFreeHeap(), ESP.getMinFreeHeap(), BuddyNetwork::connected(), BuddyNetwork::clockReady(), BuddyNetwork::showingSetup(),BuddyWeather::reading().available,PageControl::app());
  }
  delay(8);
}
