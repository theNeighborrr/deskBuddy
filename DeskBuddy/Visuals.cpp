#include "Visuals.h"
#include "AvatarArt.h"
#include "AvatarLogic.h"
#include "Weather.h"
#include "NetworkClock.h"
namespace Visuals {
uint16_t accent(uint8_t app){return app==0?CYAN:app==1?VIOLET:app==2?YELLOW:LIME;}
void bigText(TFT_eSprite& f,const char* text,int x,int y,uint8_t size,uint16_t color,uint16_t bg){f.setTextFont(1);f.setTextSize(size);f.setTextColor(color,bg);f.drawString(text,x,y,1);f.setTextSize(1);}
void shell(TFT_eSprite& f,const char* name,uint8_t app,uint8_t page,uint8_t count){
 const char* titles[]={"> COMPANION","> LOCAL TIME","> WEATHER","> WI-FI SIGNAL"};auto color=accent(app);
 f.fillRect(0,0,240,3,color);f.setTextColor(color,BG);f.drawString(titles[app],12,13,1);
 auto s=SignalMeter::reading();for(int i=0;i<4;++i){int h=3+i*3;f.fillRect(207+i*5,23-h,3,h,s.available&&i<s.bars?color:GRID);}
 f.setTextColor(MUTED,BG);String title=name;while(f.textWidth(title,1)>158&&title.length()>3)title=title.substring(0,title.length()-4)+"...";f.drawString(title,12,31,1);
 char position[16];snprintf(position,sizeof(position),"%02u/%02u",page+1,count);f.drawRightString(position,228,31,1);f.drawFastHLine(12,44,216,GRID);
}
void footer(TFT_eSprite& f){f.drawFastHLine(12,217,216,GRID);f.setTextColor(MUTED,BG);f.drawCentreString("tap: next   hold 5s: settings",120,226,1);}
const char* avatarCaption(uint8_t id){
 if(id==0){auto w=BuddyWeather::reading();if(!w.available)return "waiting for weather";if(!BuddyNetwork::connected())return "saved weather / offline";
  if(BuddyNetwork::clockReady()&&time(nullptr)-w.observedAt>=1800)return "saved weather / older";
  return BuddyWeather::description(w.code);
 }
 return AVATAR_CAPTIONS[id%ART_AVATAR_COUNT];
}
void avatar(TFT_eSprite& f,uint8_t id,uint32_t now){
 static_assert(ART_AVATAR_COUNT==AVATAR_COUNT,"Avatar settings and artwork must match");
 id%=ART_AVATAR_COUNT;const uint16_t colors[]={CYAN,MUTED,LIME,YELLOW,CYAN,ORANGE,VIOLET};uint16_t color=colors[id];
 f.setTextColor(color,BG);f.drawString(AVATAR_NAMES[id],16,55,2);
 char index[12];snprintf(index,sizeof(index),"[%u/%u]",id+1,ART_AVATAR_COUNT);f.setTextColor(MUTED,BG);f.drawRightString(index,224,60,1);
 for(int x=24;x<228;x+=24)for(int y=86;y<191;y+=24)f.drawPixel(x,y,GRID);
 uint8_t phase=(now/(id==5?350:id==4?550:700))%2;auto w=BuddyWeather::reading();
 const AvatarFrame& art=id==0?CLOUD_FRAMES[cloudState(w.available,w.code,w.day)][phase]:AVATAR_FRAMES[id][phase];
 int left=(240-art.width*3)/2,top=78;
 for(int y=0;y<art.height;++y)for(int x=0;x<art.width;++x){unsigned pos=y*art.width+x;uint8_t packed=pgm_read_byte(art.pixels+pos/2),c=pos%2?packed&15:packed>>4;if(c)f.fillRect(left+x*3,top+y*3,3,3,c);}
}
void weatherIcon(TFT_eSprite& f,int code,bool day,int x,int y){
 bool clear=code<=1,part=code==2,rain=(code>=51&&code<=67)||(code>=80&&code<=82),snow=(code>=71&&code<=77)||code==85||code==86,storm=code>=95;
 if(clear||part){if(day){f.fillCircle(x,y,19,YELLOW);for(int d=-1;d<=1;d+=2){f.fillRect(x-2,y+d*29-4,4,8,ORANGE);f.fillRect(x+d*29-4,y-2,8,4,ORANGE);f.fillRect(x+d*23-2,y-25,4,5,ORANGE);f.fillRect(x+d*23-2,y+20,4,5,ORANGE);}}else{f.fillCircle(x,y,23,VIOLET);f.fillCircle(x+11,y-10,20,BG);f.fillRect(x+25,y-24,3,3,YELLOW);}}
 if(!clear){int cy=part?y+13:y;f.fillCircle(x-14,cy,15,INK);f.fillCircle(x+3,cy-8,20,INK);f.fillCircle(x+21,cy+2,12,INK);f.fillRoundRect(x-28,cy,62,19,7,INK);
  if(rain)for(int i=-1;i<=1;++i){f.drawLine(x+i*18+3,cy+27,x+i*18-2,cy+36,CYAN);f.drawLine(x+i*18+4,cy+27,x+i*18-1,cy+36,CYAN);}
  if(snow)for(int i=-1;i<=1;++i){f.drawFastHLine(x+i*19-3,cy+29,7,CYAN);f.drawFastVLine(x+i*19,cy+26,7,CYAN);}
  if(storm){f.fillTriangle(x+2,cy+21,x-10,cy+38,x+3,cy+35,YELLOW);f.fillTriangle(x+3,cy+29,x-2,cy+48,x+15,cy+28,YELLOW);}
  if(code==45||code==48){f.drawFastHLine(x-29,cy+27,59,MUTED);f.drawFastHLine(x-22,cy+33,45,MUTED);}
 }
}
void signalPage(TFT_eSprite& f){
 auto s=SignalMeter::reading();uint16_t color=!s.available?MUTED:s.bars>=3?LIME:s.bars==2?YELLOW:CORAL;
 f.fillRoundRect(16,55,122,23,4,s.available?DARK_LIME:CARD);f.setTextColor(color,s.available?DARK_LIME:CARD);f.drawString(SignalMeter::quality(s),25,58,2);
 char value[12];if(s.available)snprintf(value,sizeof(value),"%d",s.rssi);else strcpy(value,"--");bigText(f,value,16,91,4,INK);f.setTextColor(MUTED,BG);f.drawString(s.available?"dBm / live":"no reading",18,129,1);
 for(int i=0;i<4;++i){int height=(i+1)*16;f.fillRoundRect(150+i*19,140-height,13,height,2,s.available&&i<s.bars?color:CARD);}
 f.setTextColor(MUTED,BG);f.drawString("LAST 45 SECONDS",16,153,1);f.drawRect(16,168,208,29,GRID);
 int previous=0,py=0;for(int i=0;i<SignalMeter::HISTORY;++i){int v=SignalMeter::history(i),x=30+i*4;if(v){int y=194-(constrain(v,-100,-40)+100)*23/60;if(previous)f.drawLine(x-4,py,x,y,CYAN);else f.drawPixel(x,y,CYAN);py=y;}previous=v;}
 f.setTextColor(MUTED,BG);f.drawCentreString(s.online?"Signal, not download speed":"Reconnecting to saved Wi-Fi",120,205,1);
}
}
