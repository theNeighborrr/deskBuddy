#pragma once
#include <TFT_eSPI.h>
#include "SignalMeter.h"
namespace Visuals {
enum Color:uint16_t{BG,CARD,INK,MUTED,CYAN,FACE_INK,LIME,CORAL,VIOLET,YELLOW,ORANGE,BLUE,DARK_CYAN,DARK_VIOLET,DARK_LIME,GRID};
constexpr uint16_t MINT=CYAN;
constexpr uint16_t rgb(unsigned r,unsigned g,unsigned b){return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);}
constexpr uint16_t PALETTE[16]={rgb(9,14,30),rgb(20,29,53),rgb(247,250,255),rgb(160,174,205),rgb(33,230,255),rgb(9,14,30),rgb(188,255,64),rgb(255,107,138),rgb(182,140,255),rgb(255,227,109),rgb(255,157,77),rgb(86,120,255),rgb(21,65,84),rgb(53,37,71),rgb(50,68,38),rgb(40,54,90)};
uint16_t accent(uint8_t app);
void shell(TFT_eSprite& f,const char* name,uint8_t app,uint8_t page,uint8_t count);
void footer(TFT_eSprite& f);
void avatar(TFT_eSprite& f,uint8_t id,uint32_t now);
const char* avatarCaption(uint8_t id);
void weatherIcon(TFT_eSprite& f,int code,bool day,int x,int y);
void weatherPage(TFT_eSprite& f,uint32_t now);
void weeklyPage(TFT_eSprite& f,uint32_t now);
void signalPage(TFT_eSprite& f);
void bigText(TFT_eSprite& f,const char* text,int x,int y,uint8_t size,uint16_t color,uint16_t bg=BG);
}
