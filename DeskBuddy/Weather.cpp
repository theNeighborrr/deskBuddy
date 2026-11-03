#include "Weather.h"
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <cJSON.h>
#include <Preferences.h>
#include <atomic>
#include <math.h>
#include <time.h>
namespace BuddyWeather {
namespace {
struct Job { bool geocode; char query[65]; float lat,lon; };
QueueHandle_t jobs=nullptr;TaskHandle_t worker=nullptr;
portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
WeatherReading latest{};char geoResult[4096]="{\"state\":\"idle\",\"results\":[]}";
std::atomic<bool> working{false},fresh{false};std::atomic<int> fetchResult{0};
uint32_t due=0,seenRevision=0,lastAttempt=0;Preferences cache;
float requestedLatitude=0,requestedLongitude=0;
struct Cached {uint32_t magic;WeatherReading value;};
struct Body {String text;bool overflow=false;};
esp_err_t receive(esp_http_client_event_t* event){
 auto* body=(Body*)event->user_data;
 if(event->event_id==HTTP_EVENT_ON_DATA){if(body->text.length()+event->data_len>8192){body->overflow=true;return ESP_FAIL;}body->text.concat((const char*)event->data,event->data_len);}
 return ESP_OK;
}
bool get(const String& url,String& response){
 Body body;body.text.reserve(8192);
 esp_http_client_config_t options{};options.url=url.c_str();options.crt_bundle_attach=esp_crt_bundle_attach;
 options.timeout_ms=8000;options.event_handler=receive;options.user_data=&body;options.disable_auto_redirect=true;options.buffer_size=1024;
 auto client=esp_http_client_init(&options);if(!client)return false;
 esp_http_client_set_header(client,"Accept","application/json");
 auto error=esp_http_client_perform(client);int status=esp_http_client_get_status_code(client);esp_http_client_cleanup(client);
 if(error!=ESP_OK||status!=200||body.overflow){Serial.printf("[weather] request failed code=%d http=%d\n",int(error),status);return false;}
 response=body.text;return true;
}
String encode(const char* raw){String result;while(*raw){uint8_t c=*raw++;if(isalnum(c)||c=='-'||c=='_'||c=='.')result+=(char)c;else{char s[4];snprintf(s,sizeof(s),"%%%02X",c);result+=s;}}return result;}
bool number(cJSON* object,const char* key,double& value){auto n=cJSON_GetObjectItemCaseSensitive(object,key);if(!cJSON_IsNumber(n)||!isfinite(n->valuedouble))return false;value=n->valuedouble;return true;}
bool firstNumber(cJSON* object,const char* key,double& value){auto n=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(object,key),0);if(!cJSON_IsNumber(n)||!isfinite(n->valuedouble))return false;value=n->valuedouble;return true;}
bool arrayNumber(cJSON* object,const char* key,int i,double& value){auto n=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(object,key),i);if(!cJSON_IsNumber(n)||!isfinite(n->valuedouble))return false;value=n->valuedouble;return true;}
bool parseForecast(cJSON* doc,cJSON* daily,WeatherReading& out){
 double offset;if(!number(doc,"utc_offset_seconds",offset)||offset< -50400||offset>50400||floor(offset)!=offset)return false;
 ForecastDay days[7]{};
 for(int i=0;i<7;++i){double date,high,low,code,rain=-1;
  if(!arrayNumber(daily,"time",i,date)||!arrayNumber(daily,"temperature_2m_max",i,high)||!arrayNumber(daily,"temperature_2m_min",i,low)||!arrayNumber(daily,"weather_code",i,code))return false;
  if(date<1577836800||date>4102444800||floor(date)!=date||high<low||low< -100||high>70||code<0||code>99||floor(code)!=code)return false;
  if(i&&date-days[i-1].date!=86400)return false;
  auto precipitation=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(daily,"precipitation_probability_max"),i);
  if(precipitation&&!cJSON_IsNull(precipitation)){
   if(!cJSON_IsNumber(precipitation)||!isfinite(precipitation->valuedouble))return false;
   rain=precipitation->valuedouble;if(rain<0||rain>100)return false;
  }
  days[i]={(int64_t)date,(float)high,(float)low,(int16_t)code,(int16_t)(rain<0?-1:lround(rain))};
 }
 out.utcOffset=(int32_t)offset;out.days=7;memcpy(out.forecast,days,sizeof(days));return true;
}
bool parseWeather(const String& body,const Job& job,WeatherReading& out){
 auto doc=cJSON_Parse(body.c_str());if(!doc)return false;auto current=cJSON_GetObjectItemCaseSensitive(doc,"current");auto daily=cJSON_GetObjectItemCaseSensitive(doc,"daily");
 double temp,feels,code,day,wind,stamp,high,low;
 bool ok=number(current,"temperature_2m",temp)&&number(current,"apparent_temperature",feels)&&number(current,"weather_code",code)&&number(current,"is_day",day)&&number(current,"wind_speed_10m",wind)&&number(current,"time",stamp)&&firstNumber(daily,"temperature_2m_max",high)&&firstNumber(daily,"temperature_2m_min",low);
 ok=ok&&temp>=-100&&temp<=70&&feels>=-130&&feels<=100&&code>=0&&code<=99&&floor(code)==code&&(day==0||day==1)&&wind>=0&&wind<=500&&stamp>=1577836800&&stamp<=4102444800&&high>=low&&low>=-100&&high<=70;
 if(ok){out={};out.available=true;out.temperature=temp;out.feelsLike=feels;out.code=code;out.day=day;out.wind=wind;out.observedAt=stamp;out.fetchedAt=time(nullptr);out.high=high;out.low=low;out.latitude=job.lat;out.longitude=job.lon;parseForecast(doc,daily,out);}
 cJSON_Delete(doc);return ok;
}
void publishGeo(const char* json){portENTER_CRITICAL(&lock);strlcpy(geoResult,json,sizeof(geoResult));portEXIT_CRITICAL(&lock);}
void geocode(const Job& job){
 String body;bool ok=get("https://geocoding-api.open-meteo.com/v1/search?name="+encode(job.query)+"&count=5&language=en&format=json",body);
 cJSON* doc=ok?cJSON_Parse(body.c_str()):nullptr;
 auto output=cJSON_CreateObject();cJSON_AddStringToObject(output,"state",doc?"done":"error");auto results=cJSON_AddArrayToObject(output,"results");
 auto values=cJSON_GetObjectItemCaseSensitive(doc,"results");
 for(int i=0;i<cJSON_GetArraySize(values)&&i<5;++i){
  auto entry=cJSON_GetArrayItem(values,i);double lat,lon;auto name=cJSON_GetObjectItemCaseSensitive(entry,"name");
  if(!cJSON_IsString(name)||!number(entry,"latitude",lat)||!number(entry,"longitude",lon)||lat< -90||lat>90||lon< -180||lon>180)continue;
  auto item=cJSON_CreateObject();cJSON_AddStringToObject(item,"name",name->valuestring);cJSON_AddNumberToObject(item,"latitude",lat);cJSON_AddNumberToObject(item,"longitude",lon);
  for(const char* key:{"admin1","country"}){auto field=cJSON_GetObjectItemCaseSensitive(entry,key);cJSON_AddStringToObject(item,key,cJSON_IsString(field)?field->valuestring:"");}
  cJSON_AddItemToArray(results,item);
 }
 char* serialized=cJSON_PrintUnformatted(output);if(serialized&&strlen(serialized)<sizeof(geoResult))publishGeo(serialized);else publishGeo("{\"state\":\"error\",\"results\":[]}");
 cJSON_free(serialized);cJSON_Delete(output);cJSON_Delete(doc);
}
void run(void*){
 Job job;
 for(;;){if(xQueueReceive(jobs,&job,portMAX_DELAY)!=pdTRUE)continue;
  if(job.geocode)geocode(job);
  else{
   char url[512];snprintf(url,sizeof(url),"https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,apparent_temperature,weather_code,is_day,wind_speed_10m&daily=temperature_2m_max,temperature_2m_min,weather_code,precipitation_probability_max&forecast_days=7&timezone=auto&timeformat=unixtime",job.lat,job.lon);
   String body;WeatherReading next{};bool ok=get(url,body)&&parseWeather(body,job,next);
   if(ok){portENTER_CRITICAL(&lock);latest=next;portEXIT_CRITICAL(&lock);fresh.store(true);Serial.println("[weather] current conditions refreshed");}
   fetchResult.store(ok?1:-1);
  }
  working.store(false);
 }
}
}
bool begin(){
 cache.begin("deskbuddy",false);Cached cached{};
 if(cache.getBytesLength("weather6")==sizeof(cached)){cache.getBytes("weather6",&cached,sizeof(cached));if(cached.magic==0x57454106&&cached.value.available&&isfinite(cached.value.temperature)&&cached.value.fetchedAt>=1577836800)latest=cached.value;}
 jobs=xQueueCreate(1,sizeof(Job));if(!jobs)return false;
 return xTaskCreatePinnedToCore(run,"weather6",12288,nullptr,1,&worker,0)==pdPASS;
}
void tick(bool online,bool synchronized,bool updateBusy){
 uint32_t now=millis();const auto& cfg=AppSettings::get();
 if(seenRevision!=AppSettings::revision()){seenRevision=AppSettings::revision();due=0;}
 int result=fetchResult.exchange(0);
 if(result)due=(fabsf(cfg.latitude-requestedLatitude)>.001f||fabsf(cfg.longitude-requestedLongitude)>.001f)?0:now+(result>0?900000:120000);
 if(fresh.exchange(false)){WeatherReading value=reading();if(value.available){Cached entry{0x57454106,value};cache.putBytes("weather6",&entry,sizeof(entry));}}
 if(!online||!synchronized||updateBusy||!cfg.hasLocation||!jobs||working.load()||(due&&int32_t(now-due)<0))return;
 if(lastAttempt&&uint32_t(now-lastAttempt)<60000)return;
 Job job{};job.lat=cfg.latitude;job.lon=cfg.longitude;requestedLatitude=job.lat;requestedLongitude=job.lon;
 working.store(true);if(xQueueSend(jobs,&job,0)!=pdTRUE){working.store(false);return;}lastAttempt=now;
}
WeatherReading reading(){WeatherReading value;portENTER_CRITICAL(&lock);value=latest;portEXIT_CRITICAL(&lock);const auto& cfg=AppSettings::get();if(!cfg.hasLocation||fabsf(cfg.latitude-value.latitude)>.001f||fabsf(cfg.longitude-value.longitude)>.001f)value.available=false;return value;}
bool busy(){return working.load();}
bool search(const String& query){
 if(!jobs||query.length()<2||query.length()>64||working.exchange(true))return false;
 Job job{};job.geocode=true;query.toCharArray(job.query,sizeof(job.query));publishGeo("{\"state\":\"searching\",\"results\":[]}");
 if(xQueueSend(jobs,&job,0)!=pdTRUE){working.store(false);return false;}return true;
}
String searchResult(){char result[4096];portENTER_CRITICAL(&lock);memcpy(result,geoResult,sizeof(result));portEXIT_CRITICAL(&lock);return String(result);}
void refresh(){due=0;}
const char* description(int code){
 if(code==0)return "Clear skies";if(code==1)return "Mostly clear";if(code==2)return "Partly cloudy";if(code==3)return "Cloudy";
 if(code==45||code==48)return "Foggy";if(code>=51&&code<=57)return "Drizzle";if(code>=61&&code<=67)return "Rainy";
 if(code>=71&&code<=77)return "Snowy";if(code>=80&&code<=82)return "Rain showers";if(code==85||code==86)return "Snow showers";
 if(code>=95&&code<=99)return "Thunderstorms";return "Weather";
}
unsigned selfTest(){
 unsigned failures=0;Job job{};WeatherReading result{};
 const char* good="{\"current\":{\"temperature_2m\":20,\"apparent_temperature\":19,\"weather_code\":2,\"is_day\":1,\"wind_speed_10m\":8,\"time\":1791691200},\"daily\":{\"temperature_2m_max\":[24],\"temperature_2m_min\":[12]}}";
 if(!parseWeather(good,job,result)||result.temperature!=20||result.code!=2)++failures;
 if(parseWeather("{}",job,result))++failures;
 String broken(good);broken.replace("\"temperature_2m\":20","\"temperature_2m\":null");if(parseWeather(broken,job,result))++failures;
 broken=good;broken.replace("\"weather_code\":2","\"weather_code\":999");if(parseWeather(broken,job,result))++failures;
 auto doc=cJSON_Parse(good);auto daily=cJSON_GetObjectItemCaseSensitive(doc,"daily");
 cJSON_AddNumberToObject(doc,"utc_offset_seconds",50400);
 for(const char* key:{"time","temperature_2m_max","temperature_2m_min","weather_code","precipitation_probability_max"}){
  cJSON_DeleteItemFromObject(daily,key);auto values=cJSON_AddArrayToObject(daily,key);
  for(int i=0;i<7;++i)cJSON_AddItemToArray(values,cJSON_CreateNumber(!strcmp(key,"time")?1791691200+i*86400:!strcmp(key,"temperature_2m_max")?25:!strcmp(key,"temperature_2m_min")?12:!strcmp(key,"weather_code")?2:30));
 }
 if(!parseForecast(doc,daily,result)||result.days!=7||result.forecast[6].rain!=30||result.utcOffset!=50400)++failures;
 auto rain=cJSON_GetObjectItemCaseSensitive(daily,"precipitation_probability_max");cJSON_ReplaceItemInArray(rain,0,cJSON_CreateNull());
 if(!parseForecast(doc,daily,result)||result.forecast[0].rain!=-1)++failures;
 cJSON_ReplaceItemInArray(rain,0,cJSON_CreateNumber(101));if(parseForecast(doc,daily,result))++failures;
 cJSON_ReplaceItemInArray(rain,0,cJSON_CreateNumber(0));cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(daily,"weather_code"),6);if(parseForecast(doc,daily,result))++failures;
 cJSON_Delete(doc);
 return failures;
}
}
