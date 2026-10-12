#include "FirmwareUpdate.h"
#include <WebServer.h>
#include "ReleasePublicKey.h"
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>

// Arduino must leave a new OTA image pending until our startup checks finish.
extern "C" bool verifyRollbackLater(){return true;}
namespace FirmwareUpdate {
namespace {
constexpr size_t HEADER_SIZE=160,SIGNED_SIZE=76;
uint8_t header[HEADER_SIZE]{};size_t headerBytes=0,imageBytes=0,imageSize=0;
mbedtls_sha256_context hash;
bool active=false,success=false,failed=false,pending=false,confirmed=false,hashLive=false;
const char* status="Ready for an update";uint32_t lastData=0,restartAt=0,bootAt=0;
void stop(const char* why){if(Update.isRunning())Update.abort();if(hashLive){mbedtls_sha256_free(&hash);hashLive=false;}active=false;failed=true;success=false;status=why;Serial.printf("[update] rejected: %s\n",why);}
bool checkHeader(){
 if(memcmp(header,"DBFW030",8)||memcmp(header+8,"NM-TV-154",10))return false;
 imageSize=uint32_t(header[24])|uint32_t(header[25])<<8|uint32_t(header[26])<<16|uint32_t(header[27])<<24;
 unsigned sigSize=header[76]|unsigned(header[77])<<8;
 const auto* partition=esp_ota_get_next_update_partition(nullptr);
 if(!partition||partition->size!=0x1e0000||imageSize<1024||imageSize>partition->size||sigSize<64||sigSize>72||!memchr(header+28,0,16))return false;
 uint8_t digest[32];mbedtls_sha256(header,SIGNED_SIZE,digest,0);
 mbedtls_pk_context key;mbedtls_pk_init(&key);
 int error=mbedtls_pk_parse_public_key(&key,(const unsigned char*)RELEASE_PUBLIC_KEY,strlen(RELEASE_PUBLIC_KEY)+1);
 if(!error)error=mbedtls_pk_verify(&key,MBEDTLS_MD_SHA256,digest,sizeof(digest),header+78,sigSize);
 mbedtls_pk_free(&key);return error==0;
}
}
void begin(){
 const auto* running=esp_ota_get_running_partition();esp_ota_img_states_t state;
 pending=esp_ota_get_state_partition(running,&state)==ESP_OK&&state==ESP_OTA_IMG_PENDING_VERIFY;bootAt=millis();
 Serial.printf("[update] running_slot=%s pending=%u rollback_supported=1\n",running->label,pending);
 mbedtls_sha256_init(&hash);
}
void upload(WebServer& server,bool authorized){
 auto& file=server.upload();esp_task_wdt_reset();
 if(file.status==UPLOAD_FILE_START){
  if(Update.isRunning())Update.abort();if(hashLive){mbedtls_sha256_free(&hash);hashLive=false;}
  headerBytes=imageBytes=imageSize=0;success=failed=false;active=false;restartAt=0;
  if(!authorized){stop("Open the dashboard and unlock it first");return;}
  active=true;status="Checking update";lastData=millis();
 }else if(file.status==UPLOAD_FILE_WRITE){
  if(!active||failed)return;lastData=millis();size_t pos=0;
  if(headerBytes<HEADER_SIZE){size_t count=min(size_t(file.currentSize),HEADER_SIZE-headerBytes);memcpy(header+headerBytes,file.buf,count);headerBytes+=count;pos+=count;
   if(headerBytes==HEADER_SIZE){
    if(!checkHeader()){stop("This is not a trusted update for this device");return;}
    if(!Update.begin(imageSize,U_FLASH)){stop("Could not prepare the spare firmware slot");return;}
    mbedtls_sha256_init(&hash);mbedtls_sha256_starts(&hash,0);hashLive=true;status="Installing update";Serial.println("[update] signature verified; writing spare slot");
   }
  }
  size_t count=file.currentSize-pos;
  if(count){if(imageBytes+count>imageSize){stop("Update file is too long");return;}
   if(Update.write(file.buf+pos,count)!=count){stop("Could not write update");return;}
   mbedtls_sha256_update(&hash,file.buf+pos,count);imageBytes+=count;
  }
 }else if(file.status==UPLOAD_FILE_END){
  if(!active||failed)return;
  if(headerBytes!=HEADER_SIZE||imageBytes!=imageSize||!hashLive){stop("Update file is incomplete");return;}
  uint8_t digest[32];mbedtls_sha256_finish(&hash,digest);mbedtls_sha256_free(&hash);hashLive=false;
  if(memcmp(digest,header+44,32)){stop("Update file did not pass its integrity check");return;}
  if(!Update.end(false)){stop("Firmware image validation failed");return;}
  active=false;success=true;status="Update installed. Restarting...";restartAt=millis()+1500;
  Serial.println("[update] verified and installed; restart scheduled");
 }else if(file.status==UPLOAD_FILE_ABORTED){if(active)stop("Upload interrupted; current version retained");}
}
bool busy(){return active||restartAt;}
bool accepted(){return success;}
const char* message(){return status;}
unsigned percent(){return imageSize?unsigned(imageBytes*100/imageSize):0;}
bool canRollback(){return !busy()&&Update.canRollBack();}
bool rollback(){if(!canRollback()||!Update.rollBack())return false;restartAt=millis()+1200;status="Restoring previous version...";Serial.println("[update] previous version selected");return true;}
void failBoot(){if(pending)esp_ota_mark_app_invalid_rollback_and_reboot();Serial.println("[fatal] startup checks failed");delay(1500);ESP.restart();}
void tick(bool healthy){
 if(active&&uint32_t(millis()-lastData)>15000)stop("Upload timed out; current version retained");
 if(restartAt&&int32_t(millis()-restartAt)>=0)ESP.restart();
 if(!confirmed&&uint32_t(millis()-bootAt)>=10000){
  if(!healthy){failBoot();return;}
  if(pending){if(esp_ota_mark_app_valid_cancel_rollback()!=ESP_OK){failBoot();return;}Serial.println("[update] startup checks passed; new version confirmed");}
  else Serial.println("[update] startup checks passed");
  confirmed=true;pending=false;
 }
}
}
