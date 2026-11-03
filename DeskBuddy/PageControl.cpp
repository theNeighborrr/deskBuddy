#include "PageControl.h"
namespace PageControl {
namespace {
struct Navigation {
 uint8_t page=0,character=0;bool pause=false;
 uint32_t lastSwitch=0;
 void land(const AppConfig& cfg,uint8_t target,uint32_t now,bool visit=true){
  page=target%cfg.count;lastSwitch=now;
  if(visit&&cfg.order[page]==PAL&&cfg.avatarMode)character=(character+1)%AVATAR_COUNT;
 }
};
Navigation nav;uint32_t revision=UINT32_MAX;
}
void sync(){if(revision==AppSettings::revision())return;revision=AppSettings::revision();nav.page=0;nav.character=AppSettings::get().avatar;nav.lastSwitch=millis();}
bool command(const String& action,int id){
 sync();const auto& cfg=AppSettings::get();uint8_t target=nav.page;
 if(action=="pause"){nav.pause=true;return true;}
 if(action=="resume"){nav.pause=false;nav.lastSwitch=millis();return true;}
 if(action=="next")target=(nav.page+1)%cfg.count;
 else if(action=="previous")target=(nav.page+cfg.count-1)%cfg.count;
 else if(action=="home")target=0;
 else if(action=="page"){
  bool found=false;for(int i=0;i<cfg.count;++i)if(cfg.order[i]==id){target=i;found=true;break;}
  if(!found)return false;
 }else return false;
 nav.land(cfg,target,millis(),target!=nav.page||action!="page");return true;
}
void tick(uint32_t now,bool blocked){sync();const auto& cfg=AppSettings::get();if(!blocked&&!nav.pause&&cfg.count>1&&cfg.rotationSeconds&&uint32_t(now-nav.lastSwitch)>=uint32_t(cfg.rotationSeconds)*1000)nav.land(cfg,(nav.page+1)%cfg.count,now);}
uint8_t position(){return nav.page;}
uint8_t app(){return AppSettings::get().order[nav.page%AppSettings::get().count];}
uint8_t avatar(){return AppSettings::get().avatarMode?nav.character:AppSettings::get().avatar;}
bool paused(){return nav.pause;}
unsigned selfTest(){
 unsigned fails=0;AppConfig c{};c.count=2;c.order[0]=PAL;c.order[1]=CLOCK;c.avatarMode=1;Navigation n;
 n.land(c,1,10);if(n.character!=0||n.page!=1)++fails;
 n.land(c,0,20);if(n.character!=1)++fails;
 n.land(c,0,30,false);if(n.character!=1)++fails;
 for(int i=0;i<6;++i)n.land(c,0,40+i);if(n.character!=0)++fails;
 c.avatarMode=0;n.land(c,0,60);if(n.character!=0)++fails;
 return fails;
}
}
