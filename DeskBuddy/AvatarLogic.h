#pragma once
#include <stdint.h>
// Cloud states: neutral, sun, rain, storm, snow, night, fog.
constexpr uint8_t cloudState(bool available,int code,bool day){
 if(!available)return 0;
 if(code>=95&&code<=99)return 3;
 if((code>=51&&code<=67)||(code>=80&&code<=82))return 2;
 if((code>=71&&code<=77)||code==85||code==86)return 4;
 if(code==45||code==48)return 6;
 if(!day)return 5;
 return code>=0&&code<=2?1:0;
}
inline unsigned avatarLogicSelfTest(){
 unsigned fails=0;
 struct Case{bool available;int code;bool day;uint8_t state;};
 const Case cases[]={{false,95,true,0},{true,0,true,1},{true,2,true,1},{true,3,true,0},{true,0,false,5},{true,3,false,5},{true,45,true,6},{true,48,false,6},{true,51,true,2},{true,67,false,2},{true,82,true,2},{true,71,true,4},{true,77,false,4},{true,86,true,4},{true,95,true,3},{true,99,false,3},{true,100,true,0}};
 for(auto c:cases)if(cloudState(c.available,c.code,c.day)!=c.state)++fails;
 return fails;
}
