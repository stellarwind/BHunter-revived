#include "../src/bh_cd_audio.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL cd-audio %d: %s\n",__LINE__,s);}}while(0)
int bh_cd_audio_checkpoint(unsigned long *count){
 BHCdClose cd,saved;BHCdNotify notify;BHCdAudio audio,prior;WAVEFORMATEX format;HWAVEOUT handle=NULL;MMRESULT native;DWORD packed=0;double expected,value;BHMathEnvironment fp;unsigned long i;int started;
 checks=failures=0;CHECK(bh_math_environment_enter(&fp),"actual PC53 scalar environment");
 memset(&cd,0,sizeof(cd));memset(&notify,0,sizeof(notify));memset(&audio,0,sizeof(audio));notify.cd=&cd;audio.notify=&notify;audio.wave_device=(HWAVEOUT)-1;cd.last_track=-1;
 CHECK(bh_cd_audio_volume(NULL)==1&&bh_wave_audio_volume(NULL)==1,"source NULL volume guards");
 CHECK(bh_cd_audio_volume(&audio)==1&&bh_wave_audio_volume(&audio)==1&&!audio.auxiliary_reads&&!audio.wave_reads,"missing endpoints return full volume without native query");
 CHECK(!bh_cd_audio_startup(NULL,0),"source NULL startup guard");prior=audio;audio.notify=NULL;CHECK(!bh_cd_audio_startup(&audio,42)&&!audio.opens,"missing actual native lifetime owner guard");audio=prior;
 cd.device=99;CHECK(!bh_cd_audio_startup(&audio,42)&&!audio.opens&&!notify.callback,"occupied MCI owner source guard");cd.device=0;
 cd.last_track=1234;cd.playing=37;audio.wave_device=(HWAVEOUT)5678;saved=cd;
 started=bh_cd_audio_startup(&audio,0x12345678);
 CHECK(notify.callback==0x12345678&&audio.opens==1,"original callback published before real cdaudio open");
 if(!started){CHECK(audio.open_result!=0&&cd.device==0&&!audio.sets&&!audio.auxiliary_caps&&!audio.wave_caps,"actual native cdaudio-open failure returns before downstream state and API calls");CHECK(!memcmp(&cd,&saved,sizeof(cd))&&audio.wave_device==(HWAVEOUT)5678,"open failure retains other complete native state");}
 else {CHECK(!audio.open_result&&cd.device&&audio.sets==1&&cd.playing==37,"actual native positive MCI resource publishes device and sets format without changing playing");CHECK(audio.wave_device==(HWAVEOUT)5678&&!audio.wave_caps,"existing wave identity retained without caps query");CHECK(bh_cd_stop_close(&cd)&&!cd.device&&cd.last_track==-1&&!cd.playing,"real admitted STOP/CLOSE tears down owned CD resource without Play");}
 cd.last_track=-1;audio.wave_device=(HWAVEOUT)-1;CHECK(bh_cd_audio_volume(&audio)==1&&bh_wave_audio_volume(&audio)==1,"missing endpoint fallback after resource result");
 /* Real unknown AUX index forces the documented native-error output policy.
    No fabricated caps/device or write to system volume is introduced. */
 cd.last_track=0x7ffffffe;CHECK(bh_cd_audio_volume(&audio)==1&&audio.auxiliary_result!=MMSYSERR_NOERROR&&audio.auxiliary_reads==1,"actual aux failure returns defined fallback instead of original uninitialized stack output");cd.last_track=-1;
 memset(&format,0,sizeof(format));format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=1;format.nSamplesPerSec=22050;format.wBitsPerSample=16;format.nBlockAlign=2;format.nAvgBytesPerSec=44100;
 native=waveOutOpen(&handle,WAVE_MAPPER,&format,0,0,CALLBACK_NULL);CHECK(native==MMSYSERR_NOERROR&&handle,"actual wave output opened for read-only native volume query without samples");
 if(native==MMSYSERR_NOERROR&&handle){audio.wave_device=handle;
  for(i=0;i<4;++i){native=waveOutGetVolume(handle,&packed);value=bh_wave_audio_volume(&audio);CHECK(audio.wave_result==native,"source preserves actual wave volume HRESULT");if(native==MMSYSERR_NOERROR){expected=(double)(packed&65535UL)/65535.0;CHECK(fabs(value-expected)<1e-14,"native low-left-channel scalar matches retained1/65535 factor");}else CHECK(value==1,"defined fallback for real unsupported volume read");}
  CHECK(audio.wave_reads==4,"source invokes native read each time without cached substituted success");CHECK(waveOutClose(handle)==MMSYSERR_NOERROR,"actual nonplaying wave owner closes without prepared buffers");handle=NULL;audio.wave_device=(HWAVEOUT)-1;
 }
 CHECK(bh_cd_stop_close(&cd),"actual empty CD owner close remains harmless");bh_math_environment_leave(&fp);
 if(count)*count=checks;printf("{\"scenario\":\"cd-audio\",\"checks\":%lu,\"failures\":%lu,\"cd_open_result\":%lu,\"cd_positive\":%s,\"plays\":0,\"volume_writes\":0,\"original_code_required\":false}\n",checks,failures,(unsigned long)audio.open_result,started?"true":"false");return failures!=0;
}
