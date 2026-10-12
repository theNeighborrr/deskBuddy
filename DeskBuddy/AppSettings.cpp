#include "AppSettings.h"
#include <Preferences.h>
#include <math.h>
namespace AppSettings {
namespace { Preferences storage; AppConfig current{}; uint32_t change=0; bool ready=false;
constexpr uint32_t MAGIC=0x44420501;
bool migrateFour(const AppConfig& old,AppConfig& next){
 if(old.magic!=0x44420401||old.avatar>=4)return false;
 next=old;next.magic=MAGIC;next.avatar=0;
 return AppSettings::valid(next);
}
struct LegacyConfig {
 uint32_t magic;char name[19],city[33];float latitude,longitude;
 uint16_t rotationSeconds;uint8_t order[3],count,zone,h24,fahrenheit,hasLocation;
};
bool migrate(const LegacyConfig& old,AppConfig& next){
 if(old.magic!=0x44420301||old.count<1||old.count>3)return false;
 next={};next.magic=MAGIC;memcpy(next.name,old.name,sizeof(next.name));memcpy(next.city,old.city,sizeof(next.city));
 next.latitude=old.latitude;next.longitude=old.longitude;next.rotationSeconds=old.rotationSeconds;
 next.zone=old.zone;next.h24=old.h24;next.fahrenheit=old.fahrenheit;next.hasLocation=old.hasLocation;
 for(int i=0;i<old.count;++i){if(old.order[i]>=3)return false;next.order[next.count++]=old.order[i];}
 next.order[next.count++]=WIFI_SIGNAL;return AppSettings::valid(next);
}
bool textOK(const char* value,size_t limit,bool empty=false){
 const char* end=(const char*)memchr(value,0,limit);if(!end||(!empty&&end==value))return false;
 for(auto p=value;p!=end;++p)if((uint8_t)*p<32||(uint8_t)*p>126)return false;return true;
}
}
bool valid(const AppConfig& v){
 if(v.magic!=MAGIC||!textOK(v.name,sizeof(v.name))||!textOK(v.city,sizeof(v.city),!v.hasLocation)||
 v.count<1||v.count>APP_COUNT||v.zone>=ZONE_COUNT||v.h24>1||v.fahrenheit>1||v.hasLocation>1||v.avatar>=AVATAR_COUNT)return false;
 if(v.rotationSeconds!=0&&(v.rotationSeconds<5||v.rotationSeconds>300))return false;
 uint8_t seen=0;for(int i=0;i<v.count;++i){if(v.order[i]>=APP_COUNT||(seen&(1<<v.order[i])))return false;seen|=1<<v.order[i];}
 return isfinite(v.latitude)&&isfinite(v.longitude)&&v.latitude>=-90&&v.latitude<=90&&v.longitude>=-180&&v.longitude<=180;
}
bool begin(uint8_t oldZone,bool old24){
 ready=storage.begin("deskbuddy",false);
 if(ready&&storage.getBytesLength("appcfg5")==sizeof(current)){storage.getBytes("appcfg5",&current,sizeof(current));if(valid(current))return true;}
 if(ready&&storage.getBytesLength("appcfg4")==sizeof(AppConfig)){
  AppConfig old{};storage.getBytes("appcfg4",&old,sizeof(old));
  if(migrateFour(old,current)){Serial.println("[settings] migrated 0.4 preferences; weather cloud selected");return storage.putBytes("appcfg5",&current,sizeof(current))==sizeof(current);}
 }
 if(ready&&storage.getBytesLength("appcfg")==sizeof(LegacyConfig)){
  LegacyConfig old{};storage.getBytes("appcfg",&old,sizeof(old));
  if(migrate(old,current)){Serial.println("[settings] migrated 0.3 preferences; signal page added");return storage.putBytes("appcfg5",&current,sizeof(current))==sizeof(current);}
 }
 current={};current.magic=MAGIC;strcpy(current.name,"DeskBuddy");current.order[0]=PAL;current.order[1]=CLOCK;current.order[2]=WEATHER;
 current.order[3]=WIFI_SIGNAL;current.count=APP_COUNT;current.zone=oldZone<ZONE_COUNT?oldZone:0;current.h24=old24;current.fahrenheit=1;current.rotationSeconds=20;
 return ready;
}
const AppConfig& get(){return current;}
bool save(const AppConfig& value){if(!ready||!valid(value))return false;if(storage.putBytes("appcfg5",&value,sizeof(value))!=sizeof(value))return false;current=value;++change;return true;}
uint32_t revision(){return change;}
unsigned selfTest(){
 unsigned fails=0;auto check=[&](bool ok){if(!ok)++fails;};AppConfig v{};v.magic=MAGIC;strcpy(v.name,"Test");v.count=3;v.order[0]=0;v.order[1]=1;v.order[2]=2;
 check(valid(v));v.order[2]=1;check(!valid(v));v.order[2]=2;v.count=0;check(!valid(v));v.count=3;v.rotationSeconds=1;check(!valid(v));v.rotationSeconds=20;
 v.latitude=NAN;check(!valid(v));v.latitude=0;v.zone=ZONE_COUNT;check(!valid(v));v.zone=0;v.hasLocation=1;check(!valid(v));strcpy(v.city,"Memphis");check(valid(v));
 v.avatar=AVATAR_COUNT;check(!valid(v));v.avatar=AVATAR_COUNT-1;check(valid(v));
 LegacyConfig old{};old.magic=0x44420301;strcpy(old.name,"Keep my name");strcpy(old.city,"Memphis");old.latitude=35.14953;old.longitude=-90.04898;old.hasLocation=1;old.fahrenheit=1;old.rotationSeconds=20;old.count=2;old.order[0]=2;old.order[1]=0;
 check(migrate(old,v)&&v.count==3&&v.order[0]==2&&v.order[1]==0&&v.order[2]==WIFI_SIGNAL&&v.avatar==0&&!strcmp(v.name,old.name)&&v.latitude==old.latitude);
 old.order[1]=2;check(!migrate(old,v));
 AppConfig four=v;four.magic=0x44420401;four.order[0]=2;four.order[1]=0;four.order[2]=3;four.count=3;four.avatar=3;
 check(migrateFour(four,v)&&v.avatar==0&&v.count==3&&v.order[0]==2&&!strcmp(v.name,four.name));
 four.avatar=4;check(!migrateFour(four,v));return fails;
}
}
