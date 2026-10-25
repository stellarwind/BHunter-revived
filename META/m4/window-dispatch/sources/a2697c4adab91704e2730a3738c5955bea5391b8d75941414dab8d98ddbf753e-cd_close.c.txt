#include "../src/bh_cd_close.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL cd-close: %s\n",s);}}while(0)
int bh_cd_close_checkpoint(unsigned long *count) {
 BHCdClose c,before;MCI_OPEN_PARMSA open;MCI_STATUS_PARMS status;MCIERROR error;FILE *f;
 static const unsigned char wave[]={82,73,70,70,38,0,0,0,87,65,86,69,102,109,116,32,16,0,0,0,1,0,1,0,64,31,0,0,64,31,0,0,1,0,8,0,100,97,116,97,2,0,0,0,128,128};
 checks=failures=0;memset(&c,0,sizeof(c));c.playing=9;c.last_track=7;before=c;
 CHECK(!bh_cd_stop_close(NULL),"absent source binding rejected");
 CHECK(bh_cd_stop_close(&c)&&!memcmp(&c,&before,sizeof(c)),"absent MCI device preserves playback/track state and performs no calls");
 f=fopen("cd-close-fixture.wav","wb");CHECK(f!=NULL,"private source-generated PCM fixture");if(!f)goto done;
 CHECK(fwrite(wave,1,sizeof(wave),f)==sizeof(wave)&&!fclose(f),"write complete private WAV without playback");
 memset(&open,0,sizeof(open));open.lpstrDeviceType="waveaudio";open.lpstrElementName="cd-close-fixture.wav";
 error=mciSendCommandA(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT,(DWORD)&open);
 CHECK(!error&&open.wDeviceID,"actual MCI device surrogate opens private WAV; no CD-drive or Play claim");if(error)goto remove_file;
 memset(&status,0,sizeof(status));status.dwItem=MCI_STATUS_MODE;
 CHECK(!mciSendCommandA(open.wDeviceID,MCI_STATUS,MCI_STATUS_ITEM,(DWORD)&status)&&status.dwReturn==MCI_MODE_STOP,"native opened device remains stopped");
 c.device=open.wDeviceID;c.playing=1;c.last_track=4;
 CHECK(bh_cd_stop_close(&c)&&c.native_calls==2&&!c.stop_result&&!c.close_result&&!c.device&&!c.playing&&c.last_track==-1,"complete STOP then CLOSE against a real native device and exact final state");
 CHECK(mciSendCommandA(open.wDeviceID,MCI_STATUS,MCI_STATUS_ITEM,(DWORD)&status)!=0,"closed actual device is no longer addressable");
 before=c;CHECK(bh_cd_stop_close(&c)&&!memcmp(&c,&before,sizeof(c)),"repeated close remains no-op after native release");
 c.device=open.wDeviceID;c.playing=1;c.last_track=6;
 CHECK(!bh_cd_stop_close(&c)&&c.stop_result&&c.close_result&&c.native_calls==4&&!c.device&&!c.playing&&c.last_track==-1,"both actual API failures still preserve original cleanup order/effects");
remove_file:
 CHECK(!remove("cd-close-fixture.wav"),"private MCI fixture removed");
done:
 if(count)*count=checks;printf("{\"scenario\":\"cd-close\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0,\"cd_drive_tested\":false}\n",checks,failures);return failures!=0;
}
