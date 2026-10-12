#include "NetworkClock.h"
#include "SetupRules.h"
#include "DashboardPage.h"
#include "AppSettings.h"
#include "Weather.h"
#include "FirmwareUpdate.h"
#include "SignalMeter.h"
#include "PageControl.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_sntp.h>
#include <esp_system.h>
#include <atomic>

namespace BuddyNetwork {
namespace {
constexpr uint32_t MAGIC = 0x44423102;
struct Settings {
  uint32_t magic;
  char ssid[33], password[65];
  uint8_t zone, h24;
};
Settings saved{}, candidate{};
Preferences prefs;
const IPAddress AP_IP(192, 168, 4, 1);
WebServer server(80);
DNSServer dns;
std::atomic<bool> syncEvent{false};
bool setupVisible=false;
bool hasSaved=false, portal=false, synced=false, wasConnected=false;
bool trying=false, pendingStart=false, finished=false, storeReady=false;
bool scanStarted=false, scanCached=false;
uint32_t attemptAt=0, retryAt=0, startAt=0, closeAt=0;
String networkList="[]", token;
char apName[32]{}, apPassword[17]{};
char pin[7]{};
uint32_t lastAction=0,authWindow=0,offlineUntil=0;
unsigned authFailures=0;
bool appsReady=false,weatherReady=false;
enum class State { Ready, Connecting, Saved, Error };
State state=State::Ready;
const char* errorText="";

String quote(const String& value) {
  String result="\"";
  for (size_t i=0;i<value.length();++i) {
    uint8_t c=value[i];
    if(c=='"'||c=='\\'){result+='\\';result+=(char)c;}
    else if(c<32){char escape[7];snprintf(escape,sizeof(escape),"\\u%04x",c);result+=escape;}
    else result+=(char)c;
  }
  return result+'"';
}
bool valid(const Settings& s) {
  return s.magic==MAGIC && memchr(s.ssid,0,sizeof(s.ssid)) &&
    memchr(s.password,0,sizeof(s.password)) && s.zone<ZONE_COUNT && s.h24<=1 &&
    validNetwork(s.ssid,s.password,s.password[0]==0);
}
void headers() {
  server.sendHeader("Cache-Control","no-store");
  server.sendHeader("X-Content-Type-Options","nosniff");
  server.sendHeader("Referrer-Policy","no-referrer");
  server.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
}
void json(int status, const String& body){headers();server.send(status,"application/json",body);}
bool localRequest() {
  IPAddress address=server.client().localIP();
  if(!portal || (address!=AP_IP&&address!=WiFi.localIP())) {
    json(403,"{\"message\":\"Join the DeskBuddy setup network first.\"}");return false;
  }
  String host=server.hostHeader();
  String expected=address.toString();
  if(host!=expected&&host!=expected+":80") {
    headers();server.sendHeader("Location","http://"+expected+"/");
    server.send(server.method()==HTTP_GET?302:403,"text/plain","Open the address on your buddy");return false;
  }
  return true;
}
bool authorized() {
  if(!localRequest())return false;
  if(token.isEmpty()||server.header("X-Buddy-Token")!=token){json(401,"{\"message\":\"Unlock the dashboard using the code on your buddy.\"}");return false;}
  if(server.method()!=HTTP_GET)lastAction=millis();
  return true;
}
void setupPage() {
  if(!localRequest())return;
  headers();server.send_P(200,"text/html; charset=utf-8",DASHBOARD_PAGE);
}
void startSaved() {
  if(!hasSaved)return;
  WiFi.begin(saved.ssid,saved.password);
  retryAt=millis();
  Serial.println("[wifi] trying saved network");
}
void setClock() {
  configTzTime(CLOCK_ZONES[AppSettings::get().zone].rule,"pool.ntp.org","time.nist.gov");
}
bool integer(const String& s,int lo,int hi,int& value){
 if(s.isEmpty()||s.length()>6)return false;for(size_t i=0;i<s.length();++i)if(!isdigit((unsigned char)s[i]))return false;
 value=s.toInt();return value>=lo&&value<=hi;
}
bool coordinate(const String& s,float& value){char* end;value=strtof(s.c_str(),&end);return !s.isEmpty()&&end==s.c_str()+s.length()&&isfinite(value);}
String configJson(){
 const auto& c=AppSettings::get();String order="[";for(int i=0;i<c.count;++i){if(i)order+=',';order+=c.order[i];}order+=']';
 return String("{\"name\":")+quote(c.name)+",\"avatar\":"+c.avatar+",\"avatarMode\":"+c.avatarMode+",\"colorSwap\":"+c.colorSwap+",\"order\":"+order+",\"rotation\":"+c.rotationSeconds+
 ",\"zone\":"+quote(CLOCK_ZONES[c.zone].id)+",\"h24\":"+c.h24+",\"fahrenheit\":"+c.fahrenheit+
 ",\"hasLocation\":"+c.hasLocation+",\"city\":"+quote(c.city)+",\"latitude\":"+String(c.latitude,5)+",\"longitude\":"+String(c.longitude,5)+
 ",\"version\":"+quote(FIRMWARE_VERSION)+",\"configured\":"+(hasSaved?"true":"false")+",\"canRollback\":"+(FirmwareUpdate::canRollback()?"true":"false")+"}";
}
void routes() {
  const char* keys[]={"X-Buddy-Token"};server.collectHeaders(keys,1);
  server.on("/",HTTP_GET,setupPage);
  server.on("/api/auth",HTTP_POST,[]{
    if(!localRequest())return;
    if(uint32_t(millis()-authWindow)>=60000){authWindow=millis();authFailures=0;}
    if(authFailures>=5){json(429,"{\"message\":\"Too many attempts. Wait one minute and try again.\"}");return;}
    if(server.client().localIP()!=AP_IP&&server.arg("pin")!=pin){++authFailures;json(401,"{\"message\":\"Enter the six-digit code on your buddy.\"}");return;}
    lastAction=millis();if(hasSaved&&connected())setupVisible=false;json(200,String("{\"token\":")+quote(token)+"}");
  });
  server.on("/api/config",HTTP_GET,[]{if(authorized())json(200,configJson());});
  server.on("/api/settings",HTTP_POST,[]{
    if(!authorized())return;AppConfig next=AppSettings::get();int rotation,h24,units,location,avatar=next.avatar,avatarMode=next.avatarMode,colorSwap=next.colorSwap;
    String name=server.arg("name"),city=server.arg("city"),order=server.arg("order");int zone=zoneIndex(server.arg("zone"));
    bool ok=name.length()>0&&name.length()<sizeof(next.name)&&city.length()<sizeof(next.city)&&zone>=0&&
      integer(server.arg("rotation"),0,300,rotation)&&integer(server.arg("h24"),0,1,h24)&&integer(server.arg("fahrenheit"),0,1,units)&&integer(server.arg("hasLocation"),0,1,location)&&coordinate(server.arg("latitude"),next.latitude)&&coordinate(server.arg("longitude"),next.longitude);
    if(server.hasArg("avatar")&&!integer(server.arg("avatar"),0,AVATAR_COUNT-1,avatar))ok=false;
    if(server.hasArg("avatarMode")&&!integer(server.arg("avatarMode"),0,1,avatarMode))ok=false;
    if(server.hasArg("colorSwap")&&!integer(server.arg("colorSwap"),0,1,colorSwap))ok=false;
    next.avatar=avatar;next.avatarMode=avatarMode;next.colorSwap=colorSwap;next.count=0;
    if(order.isEmpty()||order.endsWith(","))ok=false;
    for(int start=0;start<(int)order.length();){int end=order.indexOf(',',start);if(end<0)end=order.length();int id;
      if(next.count>=APP_COUNT||!integer(order.substring(start,end),0,APP_COUNT-1,id)){ok=false;break;}next.order[next.count++]=id;start=end+1;}
    if(!ok){json(400,"{\"message\":\"Check the name, pages, clock settings, and city.\"}");return;}
    name.toCharArray(next.name,sizeof(next.name));city.toCharArray(next.city,sizeof(next.city));next.zone=zone;next.rotationSeconds=rotation;next.h24=h24;next.fahrenheit=units;next.hasLocation=location;
    if(!AppSettings::valid(next)){json(400,"{\"message\":\"Use basic letters for names, choose at least one unique page, and set rotation to 0 or 5–300 seconds.\"}");return;}
    if(!AppSettings::save(next)){json(500,"{\"message\":\"Settings could not be saved. Try again.\"}");return;}
    setenv("TZ",CLOCK_ZONES[next.zone].rule,1);tzset();BuddyWeather::refresh();Serial.println("[settings] saved");json(200,"{\"message\":\"Saved\"}");
  });
  server.on("/api/keepalive",HTTP_POST,[]{if(authorized())json(200,"{\"ok\":true}");});
  server.on("/api/control",HTTP_POST,[]{
    if(!authorized())return;
    if(FirmwareUpdate::busy()||trying||pendingStart){json(409,"{\"message\":\"Wait for the current operation to finish.\"}");return;}
    int app=-1;String action=server.arg("action");
    if(action=="page"&&!integer(server.arg("app"),0,APP_COUNT-1,app)){json(400,"{\"message\":\"Choose a valid page.\"}");return;}
    if(!PageControl::command(action,app)){json(400,"{\"message\":\"Choose an enabled page or a valid control.\"}");return;}
    setupVisible=false;json(200,"{\"ok\":true}");
  });
  server.on("/api/search",HTTP_POST,[]{
    if(!authorized())return;
    if(!connected()||!synced){json(409,"{\"message\":\"Connect Wi-Fi and wait for the clock to sync before searching.\"}");return;}
    if(!BuddyWeather::search(server.arg("query"))){json(409,"{\"message\":\"Weather is busy, or the city name is invalid. Try again shortly.\"}");return;}
    json(202,"{\"message\":\"Searching\"}");
  });
  server.on("/api/search",HTTP_GET,[]{if(authorized())json(200,BuddyWeather::searchResult());});
  server.on("/api/update",HTTP_POST,[]{
    if(!authorized())return;json(FirmwareUpdate::accepted()?200:400,String("{\"message\":")+quote(FirmwareUpdate::message())+"}");
  },[]{
    IPAddress local=server.client().localIP();String expected=local.toString(),host=server.hostHeader();
    bool valid=portal&&(local==AP_IP||local==WiFi.localIP())&&(host==expected||host==expected+":80")&&!token.isEmpty()&&server.header("X-Buddy-Token")==token;
    if(valid)lastAction=millis();FirmwareUpdate::upload(server,valid);
  });
  server.on("/api/rollback",HTTP_POST,[]{
    if(!authorized())return;
    if(!FirmwareUpdate::rollback()){json(409,"{\"message\":\"No previous firmware is available.\"}");return;}
    json(200,"{\"message\":\"Restoring previous firmware. Restarting...\"}");
  });
  server.on("/api/state",HTTP_GET,[]{
    if(!authorized())return;
    const char* value=state==State::Connecting?"connecting":state==State::Saved?"saved":state==State::Error?"error":"ready";
    auto weather=BuddyWeather::reading();
    auto signal=SignalMeter::reading();PageControl::sync();
    json(200,String("{\"state\":")+quote(value)+",\"message\":"+quote(errorText)+",\"online\":"+(connected()?"true":"false")+
      ",\"synced\":"+(synced?"true":"false")+",\"heap\":"+ESP.getFreeHeap()+",\"uptime\":"+millis()+
      ",\"app\":"+PageControl::app()+",\"avatar\":"+PageControl::avatar()+",\"paused\":"+(PageControl::paused()?"true":"false")+",\"forecastDays\":"+weather.days+",\"setupVisible\":"+(setupVisible?"true":"false")+",\"weatherAvailable\":"+(weather.available?"true":"false")+",\"weatherFetchedAt\":"+String((long long)weather.fetchedAt)+",\"temperatureC\":"+String(weather.temperature,1)+
      ",\"signal\":{\"available\":"+(signal.available?"true":"false")+",\"rssi\":"+(signal.available?String(signal.rssi):String("null"))+",\"bars\":"+signal.bars+",\"quality\":"+quote(SignalMeter::quality(signal))+"}}");
  });
  server.on("/api/networks",HTTP_GET,[]{
    if(!authorized())return;
    if(!trying&&!pendingStart&&!scanStarted&&!scanCached){
      int result=WiFi.scanNetworks(true,false,false,120);
      scanStarted=result==WIFI_SCAN_RUNNING;
      if(!scanStarted)scanCached=true;
    }
    json(200,String("{\"scanning\":")+(scanStarted?"true":"false")+",\"networks\":"+networkList+"}");
  });
  server.on("/api/connect",HTTP_POST,[]{
    if(!authorized())return;
    if(trying||pendingStart){json(409,"{\"message\":\"A connection is already being tested.\"}");return;}
    if(!storeReady){json(503,"{\"message\":\"Settings storage is unavailable. Restart your buddy and try again.\"}");return;}
    String ssid=server.arg("ssid"), password=server.arg("password");
    if(!validNetwork(ssid,password,server.arg("open")=="1")){
      json(400,"{\"message\":\"Check the network name, password, and timezone.\"}");return;
    }
    candidate={};candidate.magic=MAGIC;
    ssid.toCharArray(candidate.ssid,sizeof(candidate.ssid));
    password.toCharArray(candidate.password,sizeof(candidate.password));
    candidate.zone=AppSettings::get().zone;candidate.h24=AppSettings::get().h24;
    closeAt=0;state=State::Connecting;errorText="";pendingStart=true;startAt=millis();
    json(202,"{\"message\":\"Connecting\"}");
  });
  server.on("/api/done",HTTP_POST,[]{
    if(!authorized())return;
    if(FirmwareUpdate::busy()){json(409,"{\"message\":\"Wait for the update to finish.\"}");return;}
    closeAt=millis()+600;json(200,"{\"message\":\"Done\"}");
  });
  server.onNotFound([]{
    if(!localRequest())return;
    if(server.uri().startsWith("/api/")){json(404,"{\"message\":\"Not found\"}");return;}
    headers();server.sendHeader("Location","http://192.168.4.1/");server.send(302,"text/plain","");
  });
}
}
void begin() {
  storeReady=prefs.begin("deskbuddy",false);
  if(storeReady&&prefs.getBytesLength("netcfg")==sizeof(saved)){
    prefs.getBytes("netcfg",&saved,sizeof(saved));hasSaved=valid(saved);
  }
  if(!hasSaved)saved={};
  appsReady=AppSettings::begin(saved.zone,saved.h24);
  weatherReady=BuddyWeather::begin();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  esp_sntp_set_time_sync_notification_cb([](struct timeval*){syncEvent.store(true);});
  routes();
  Serial.printf("[network] settings=%s storage=%s\n",hasSaved?"saved":"empty",storeReady?"ready":"failed");
  setenv("TZ",CLOCK_ZONES[AppSettings::get().zone].rule,1);tzset();
  if(hasSaved){startSaved();}
  else openSetup();
}
void openSetup() {
  if(portal){setupVisible=true;lastAction=millis();return;}
  WiFi.mode(WIFI_AP_STA);
  uint64_t mac=ESP.getEfuseMac();
  snprintf(apName,sizeof(apName),"DeskBuddy-%04X",unsigned((mac>>32)&0xffff));
  // Wi-Fi is enabled before random generation so the hardware RNG has RF entropy.
  const char alphabet[]="abcdefghjkmnpqrstuvwxyz23456789";
  for(int i=0;i<12;++i)apPassword[i]=alphabet[esp_random()%(sizeof(alphabet)-1)];
  apPassword[12]=0;
  snprintf(pin,sizeof(pin),"%06lu",(unsigned long)(esp_random()%1000000));
  authFailures=0;authWindow=lastAction=millis();
  token="";for(int i=0;i<4;++i){char part[9];snprintf(part,sizeof(part),"%08lx",(unsigned long)esp_random());token+=part;}
  if(!WiFi.softAPConfig(AP_IP,AP_IP,IPAddress(255,255,255,0))||!WiFi.softAP(apName,apPassword)){
    Serial.println("[setup] access point failed");WiFi.mode(WIFI_STA);return;
  }
  portal=true;setupVisible=true;state=State::Ready;errorText="";closeAt=0;scanCached=false;networkList="[]";
  dns.start(53,"*",AP_IP);server.begin();
  Serial.println("[setup] open; credentials displayed on device only");
}
void closeSetup() {
  if(!portal||FirmwareUpdate::busy())return;
  bool cancel=trying||pendingStart;
  trying=pendingStart=false;candidate={};
  if(scanStarted){WiFi.scanDelete();scanStarted=false;}
  server.stop();dns.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);
  portal=false;setupVisible=false;token="";memset(apPassword,0,sizeof(apPassword));closeAt=0;
  if(cancel){WiFi.disconnect(false,false);wasConnected=false;startSaved();}
  Serial.println("[setup] closed");
}
void tick() {
  uint32_t now=millis();
  if(portal){dns.processNextRequest();server.handleClient();}
  // Request handlers update their activity timestamps; compare against time
  // sampled after the handler, otherwise unsigned subtraction can wrap.
  now=millis();
  if(scanStarted){
    int count=WiFi.scanComplete();
    if(count!=WIFI_SCAN_RUNNING){
      networkList="[";unsigned added=0;
      for(int i=0;i<count&&added<16;++i){String name=WiFi.SSID(i);if(name.isEmpty())continue;
        if(added++)networkList+=',';networkList+=quote(name);}
      networkList+=']';WiFi.scanDelete();scanStarted=false;scanCached=true;
    }
  }
  if(pendingStart&&!scanStarted&&uint32_t(now-startAt)>=350){
    pendingStart=false;trying=true;wasConnected=false;
    WiFi.disconnect(false,false);WiFi.begin(candidate.ssid,candidate.password);attemptAt=now;
    Serial.println("[wifi] testing submitted network");
  }
  bool online=WiFi.status()==WL_CONNECTED;
  SignalMeter::tick(online);
  if(trying&&online){
    trying=false;
    if(prefs.putBytes("netcfg",&candidate,sizeof(candidate))==sizeof(candidate)){
      saved=candidate;hasSaved=true;state=State::Saved;errorText="";closeAt=0;
      setClock();wasConnected=true;
      Serial.println("[wifi] connected; settings saved");
    }else{
      state=State::Error;errorText="Connected, but settings could not be saved. Please try again.";
      WiFi.disconnect(false,false);online=false;wasConnected=false;startSaved();
      Serial.println("[wifi] settings write failed");
    }
    candidate={};
  }else if(trying&&uint32_t(now-attemptAt)>=30000){
    trying=false;candidate={};state=State::Error;
    errorText="Could not connect. Check the password and use a 2.4 GHz network, then try again.";
    WiFi.disconnect(false,false);wasConnected=false;online=false;startSaved();
    Serial.println("[wifi] submitted network timed out; previous settings retained");
  }
  if(!trying&&!pendingStart){
    if(online&&!wasConnected&&hasSaved){setClock();Serial.println("[wifi] connected to saved network");}
    if(!online&&wasConnected)Serial.println("[wifi] offline; clock keeps running if synced");
    wasConnected=online;
    if(offlineUntil&&int32_t(now-offlineUntil)>=0){offlineUntil=0;WiFi.setAutoReconnect(true);startSaved();}
    if(!offlineUntil&&hasSaved&&!online&&uint32_t(now-retryAt)>=30000)startSaved();
  }
  if(syncEvent.exchange(false)){
    synced=true;struct tm local{};localTime(local);char stamp[48];strftime(stamp,sizeof(stamp),"%Y-%m-%d %H:%M:%S %Z",&local);
    Serial.printf("[clock] synchronized local=%s\n",stamp);
  }
  if(portal&&closeAt&&int32_t(now-closeAt)>=0){closeSetup();finished=true;}
  if(portal&&!FirmwareUpdate::busy()&&uint32_t(now-lastAction)>=600000)closeSetup();
  BuddyWeather::tick(online,synced,FirmwareUpdate::busy());
}
bool inSetup(){return portal;}
bool showingSetup(){return portal&&setupVisible;}
void hideSetup(){setupVisible=false;}
bool connected(){return WiFi.status()==WL_CONNECTED;}
bool configured(){return hasSaved;}
bool clockReady(){return synced;}
bool hour24(){return AppSettings::get().h24;}
bool takeFinished(){bool value=finished;finished=false;return value;}
const char* zoneLabel(){return CLOCK_ZONES[AppSettings::get().zone].label;}
const char* setupName(){return apName;}
const char* setupPassword(){return apPassword;}
const char* setupStatus(){return state==State::Connecting?"Connecting...":state==State::Saved?"Connected! Customize below.":state==State::Error?"Check your phone to retry.":"Set up from your phone";}
const char* dashboardPin(){return pin;}
String dashboardAddress(){return WiFi.localIP().toString();}
bool healthy(){return appsReady&&weatherReady&&storeReady;}
void usbSession(){if(portal)Serial.printf("[usb-session] {\"ip\":\"%s\",\"token\":\"%s\"}\n",WiFi.localIP().toString().c_str(),token.c_str());}
void testOffline(){WiFi.setAutoReconnect(false);WiFi.disconnect(false,false);offlineUntil=millis()+20000;Serial.println("[test] simulated Wi-Fi outage for 20 seconds");}
bool localTime(struct tm& value){if(!synced)return false;time_t now=time(nullptr);return localtime_r(&now,&value)!=nullptr;}
}
