#include "bh_cd_audio.h"
#include <string.h>
#include <stddef.h>
typedef char auxiliary_caps_layout[sizeof(AUXCAPSA)==0x30?1:-1];
typedef char wave_caps_layout[sizeof(WAVEOUTCAPSA)==0x34?1:-1];
int __cdecl bh_cd_audio_startup(BHCdAudio *audio,DWORD callback){
 MCI_OPEN_PARMSA open;MCI_SET_PARMS set;AUXCAPSA aux;WAVEOUTCAPSA wave;UINT count;
 BHCdClose *cd;
 if(!audio||!audio->notify||!(cd=audio->notify->cd)||cd->device)return 0;
 audio->notify->callback=callback;memset(&open,0,sizeof(open));open.lpstrDeviceType="cdaudio";
 ++audio->opens;audio->open_result=mciSendCommandA(0,MCI_OPEN,MCI_OPEN_TYPE,(DWORD)&open);
 if(audio->open_result)return 0;
 cd->device=open.wDeviceID;memset(&set,0,sizeof(set));set.dwTimeFormat=10;
 ++audio->sets;audio->set_result=mciSendCommandA(cd->device,MCI_SET,MCI_SET_TIME_FORMAT,(DWORD)&set);
 cd->last_track=-1;count=auxGetNumDevs();
 while(count&&cd->last_track==-1){--count;memset(&aux,0,sizeof(aux));++audio->auxiliary_caps;
  audio->auxiliary_result=auxGetDevCapsA(count,&aux,sizeof(aux));
  if(audio->auxiliary_result==MMSYSERR_NOERROR&&aux.wTechnology==AUXCAPS_CDAUDIO)cd->last_track=(long)count;
 }
 count=waveOutGetNumDevs();
 while(count&&audio->wave_device==(HWAVEOUT)-1){--count;memset(&wave,0,sizeof(wave));++audio->wave_caps;
  audio->wave_result=waveOutGetDevCapsA(count,&wave,sizeof(wave));
  if(audio->wave_result==MMSYSERR_NOERROR&&(wave.dwSupport&WAVECAPS_VOLUME))audio->wave_device=(HWAVEOUT)count;
 }
 return 1;
}
double __cdecl bh_cd_audio_volume(BHCdAudio *audio){DWORD value=0;BHCdClose *cd;
 if(!audio||!audio->notify||!(cd=audio->notify->cd)||cd->last_track==-1)return 1.0;
 ++audio->auxiliary_reads;audio->auxiliary_result=auxGetVolume((UINT)cd->last_track,&value);
 if(audio->auxiliary_result!=MMSYSERR_NOERROR)return 1.0;
 return (double)(value&65535UL)*(1.0/65535.0);
}
double __cdecl bh_wave_audio_volume(BHCdAudio *audio){DWORD value=0;
 if(!audio||audio->wave_device==(HWAVEOUT)-1)return 1.0;
 ++audio->wave_reads;audio->wave_result=waveOutGetVolume(audio->wave_device,&value);
 if(audio->wave_result!=MMSYSERR_NOERROR)return 1.0;
 return (double)(value&65535UL)*(1.0/65535.0);
}
