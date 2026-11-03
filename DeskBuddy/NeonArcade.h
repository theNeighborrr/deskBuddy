#pragma once
#include "Visuals.h"
#include "NeonArt.h"
namespace NeonArcade {
inline void draw(TFT_eSprite& f,const char* name,uint32_t now){
 using namespace Visuals;
 // Expand directly into the existing framebuffer, coalescing equal-color runs.
 for(int y=0;y<120;++y){
  int start=0,previous=-1;
  for(int x=0;x<=120;++x){
   int color=-1;
   if(x<120){unsigned p=y*120+x;uint8_t b=pgm_read_byte(NEON_BACKGROUND+p/2);color=p%2?b&15:b>>4;}
   if(color!=previous){if(x>start)f.fillRect(start*2,y*2,(x-start)*2,2,previous);start=x;previous=color;}
  }
 }
 uint32_t t=now/100;const uint16_t reflections[]={CORAL,DARK_CYAN,VIOLET,CYAN};
 for(int i=0;i<28;++i){int x=(i*29+11)%120,y=88+(i*11)%31,dx=int((t/3+i)%5)-2;f.fillRect((x+dx)*2,y*2,(3+i%8)*2,2,reflections[i%4]);}
 for(int i=0;i<30;++i){
  int x=(i*37+13+(t/2)%120)%120,y=(i*23+t*(2+i%2))%120;
  if(y>=35&&y<=53&&x<61)continue;
  // Stair-stepped rain at the same 2x pixel scale as the architecture.
  f.fillRect(x*2,y*2,2,4,i%3?DARK_CYAN:BLUE);f.fillRect((x-1)*2,(y+2)*2,2,4,i%3?DARK_CYAN:BLUE);
 }
 for(int i=0;i<5;++i){int x=16+i*21,y=95+i%3*8,phase=(t+i*5)%18;if(phase<5)f.fillRect((x-phase/2)*2,y*2,(2*(phase/2)+1)*2,2,DARK_CYAN);}
 for(int i=0;i<10;++i)f.fillRect((4+i*5)*2,100,4,2,(t/8+i)%10<3?CORAL:VIOLET);
 // Bounded ASCII name, matching the settings limit, scaled to fit the sign.
 size_t length=strnlen(name,18);int scale=length<=13?2:1,width=int(length)*4*scale-scale;
 int left=60-width/2,top=scale==2?81:83;
 for(size_t i=0;i<length;++i){unsigned c=(uint8_t)name[i];if(c<32||c>126)c='?';
  for(int y=0;y<5;++y){uint8_t row=pgm_read_byte(&NEON_FONT[c-32][y]);for(int x=0;x<3;++x)if(row&(4>>x))f.fillRect(left+int(i)*4*scale+x*scale,top+y*scale,scale,scale,YELLOW);}
 }
}
}
