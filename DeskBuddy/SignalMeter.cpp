#include "SignalMeter.h"
#include <WiFi.h>
#include <utility>
namespace SignalMeter {
namespace {Reading current;int8_t samples[HISTORY]{};uint8_t head=0;uint32_t sampledAt=0;bool started=false;}
const char* quality(const Reading& v){if(!v.online)return "Offline";if(!v.available)return "Measuring";return v.bars==4?"Excellent":v.bars==3?"Good":v.bars==2?"Fair":"Weak";}
void tick(bool online){
 uint32_t now=millis();bool changed=online!=current.online;
 if(!online)current={};
 if(started&&!changed&&uint32_t(now-sampledAt)<1000)return;
 sampledAt=now;started=true;current.online=online;
 int value=online?WiFi.RSSI():0;current.available=online&&value<0&&value>=-127;
 current.rssi=current.available?value:0;current.bars=current.available?barsFor(value):0;
 samples[head]=current.rssi;head=(head+1)%HISTORY;
}
Reading reading(){return current;}
int history(uint8_t index){return index<HISTORY?samples[(head+index)%HISTORY]:0;}
unsigned selfTest(){unsigned fail=0;for(auto pair:{std::pair<int,int>{-40,4},{-55,4},{-56,3},{-67,3},{-68,2},{-75,2},{-76,1},{-100,1}})if(barsFor(pair.first)!=pair.second)++fail;Reading v{};if(strcmp(quality(v),"Offline"))++fail;v.online=true;if(strcmp(quality(v),"Measuring"))++fail;return fail;}
}
