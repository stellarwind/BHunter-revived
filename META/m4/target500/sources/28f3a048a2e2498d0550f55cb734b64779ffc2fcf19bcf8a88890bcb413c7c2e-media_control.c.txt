#include "../src/bh_media_control.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
static unsigned long checks;static int failures;
static void check(int ok,const char *name){++checks;if(!ok){++failures;printf("FAIL media %s\n",name);}}
int bh_media_control_checkpoint(unsigned long *count){BHCdClose cd;BHCdNotify n;BHCdAudio audio;BHMediaControl c;int i;unsigned long bits=0x7fc00000;float nan;float values[7]={-2,0,.25f,.5f,.75f,1,3};DWORD expected[7]={0,0,0x3fff3fff,0x7fff7fff,0xbfffbfff,0xffffffff,0xffffffff};
 checks=0;failures=0;memset(&cd,0,sizeof(cd));memset(&n,0,sizeof(n));memset(&audio,0,sizeof(audio));memset(&c,0,sizeof(c));n.cd=&cd;audio.notify=&n;c.audio=&audio;cd.last_track=-1;audio.wave_device=(HWAVEOUT)-1;
 n.from_track=88;cd.playing=9;check(bh_cd_play_range(&c,1,2,3,4,1)&&n.from_track==88&&cd.playing==9&&!n.plays,"absent CD preserves range");
 bh_cd_stop(&c);check(cd.playing==0&&!c.stops,"stop clears repeat without device");bh_cd_set_volume(&c,.5f);bh_wave_set_volume(&c,.5f);check(!c.aux_writes&&!c.wave_writes,"absent endpoints no write");
 /* Deliberately invalid IDs cannot change an installed device's volume or
    start physical playback; they still execute the actual native API path. */
 cd.device=0x7fffffff;n.callback=0x1234;check(!bh_cd_play_range(&c,0x123,0x456,0x12345678,0xabcd,7),"native invalid-device play error");
 check(n.from_track==0x123&&n.end_track==0x456&&n.minute_second==0x12345678&&n.frame==0xabcd&&cd.playing==7,"full repeat fields retained on error");
 check(n.last.dwFrom==0x23&&n.last.dwTo==0xdf7856&&n.last.dwCallback==0x1234&&n.plays==1&&n.failures==1,"native TMSF argument packing with overlapping OR fields");
 bh_cd_stop(&c);check(cd.playing==0&&c.stops==1&&c.stop_result!=0&&cd.device==0x7fffffff,"native stop error retains ownership");
 cd.last_track=0x7fffffff;audio.wave_device=(HWAVEOUT)0x7fffffff;
 for(i=0;i<7;++i){bh_cd_set_volume(&c,values[i]);bh_wave_set_volume(&c,values[i]);check(c.aux_packed==expected[i]&&c.wave_packed==expected[i],"clamp/truncate/stereo duplication");check(c.aux_result!=MMSYSERR_NOERROR&&c.wave_result!=MMSYSERR_NOERROR,"native invalid endpoint errors");}
 memcpy(&nan,&bits,4);bh_cd_set_volume(&c,nan);bh_wave_set_volume(&c,nan);check(!c.aux_packed&&!c.wave_packed,"unordered maps to lower bound");check(c.aux_writes==8&&c.wave_writes==8,"actual write invocation counts");
 *count+=checks;printf("{\"scenario\":\"media-control\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_MEDIA_CONTROL_STANDALONE
int main(void){unsigned long n=0;return bh_media_control_checkpoint(&n);}
#endif
