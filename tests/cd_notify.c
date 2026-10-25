#include "../src/bh_cd_notify.h"
#include <stdio.h>
#include <string.h>
int __cdecl bh_cd_notify_checkpoint(unsigned long *count) {
 BHCdClose cd;BHCdNotify c;MCI_OPEN_PARMS open;MCI_GENERIC_PARMS close;
 unsigned long checks=0,failures=0,i;MCIERROR error;FILE *file;
 static const unsigned char wav[46]={82,73,70,70,38,0,0,0,87,65,86,69,102,109,116,32,16,0,0,0,1,0,1,0,64,31,0,0,128,62,0,0,2,0,16,0,100,97,116,97,2,0,0,0,0,0};
#define CHECK(x) do{++checks;if(!(x))++failures;}while(0)
 memset(&cd,0,sizeof(cd));memset(&c,0,sizeof(c));CHECK(!bh_cd_notify(NULL,1,0));CHECK(!bh_cd_notify(&c,1,0));c.cd=&cd;
 cd.playing=1;CHECK(bh_cd_notify(&c,1,0)&&!c.plays);
 cd.device=42;CHECK(bh_cd_notify(&c,1,43)&&!c.plays);
 for(i=0;i<5;++i)if(i!=1)CHECK(bh_cd_notify(&c,(short)i,42)&&!c.plays);
 cd.playing=0;CHECK(bh_cd_notify(&c,1,42)&&!c.plays);
 file=fopen("cd-notify-private.wav","wb");CHECK(file!=NULL);if(!file)goto done;CHECK(fwrite(wav,sizeof(wav),1,file)==1);CHECK(fclose(file)==0);
 memset(&open,0,sizeof(open));open.lpstrDeviceType="waveaudio";open.lpstrElementName="cd-notify-private.wav";
 error=mciSendCommand(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT|MCI_WAIT,(DWORD)&open);CHECK(error==0);if(error)goto done;
 CHECK(open.wDeviceID!=0&&open.wDeviceID!=MCI_ALL_DEVICE_ID);if(!open.wDeviceID||open.wDeviceID==MCI_ALL_DEVICE_ID)goto done;
 memset(&close,0,sizeof(close));error=mciSendCommand(open.wDeviceID,MCI_CLOSE,MCI_WAIT,(DWORD)&close);CHECK(error==0);if(error)goto done;
 cd.device=open.wDeviceID;cd.playing=1;c.from_track=0xabcdef02;c.end_track=0x12345603;c.minute_second=0x98764321;c.frame=0x123456ab;c.callback=0;
 CHECK(!bh_cd_notify(&c,1,cd.device)&&c.plays==1&&c.failures==1&&c.last_result!=0);
 CHECK(c.last.dwFrom==2&&c.last.dwTo==0xeb2103UL&&c.last.dwCallback==0);
 CHECK(cd.device==open.wDeviceID&&cd.playing==1);
done:
 remove("cd-notify-private.wav");if(count)*count=checks;
 printf("{\"scenario\":\"cd-notify\",\"checks\":%lu,\"failures\":%lu,\"plays\":%lu,\"mci_result\":%lu,\"audible_playback\":false,\"original_code_required\":false}\n",checks,failures,c.plays,(unsigned long)c.last_result);return failures!=0;
#undef CHECK
}
