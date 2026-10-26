#include "bh_media_control.h"
#include <float.h>
static BHCdNotify *notify(BHMediaControl *c){return c&&c->audio?c->audio->notify:0;}
int __cdecl bh_cd_play_range(BHMediaControl *c,unsigned long from,unsigned long to,unsigned long ms,unsigned long frame,long repeat){BHCdNotify *n=notify(c);
 if(!n||!n->cd)return 0;if(!n->cd->device)return 1;
 n->from_track=from;n->end_track=to;n->minute_second=ms;n->frame=frame;n->cd->playing=repeat;
 n->last.dwCallback=n->callback;n->last.dwFrom=from&255UL;n->last.dwTo=(((frame&255UL)<<8)|(ms&65535UL))<<8|(to&255UL);
 ++n->plays;n->last_result=mciSendCommandA(n->cd->device,MCI_PLAY,0x0d,(DWORD)&n->last);
 if(n->last_result){++n->failures;return 0;}return 1;
}
void __cdecl bh_cd_stop(BHMediaControl *c){BHCdNotify *n=notify(c);if(!n||!n->cd)return;n->cd->playing=0;
 if(n->cd->device){++c->stops;c->stop_result=mciSendCommandA(n->cd->device,MCI_STOP,0,0);}
}
/* Original FNSTSW C0 first comparison sends unordered to zero. The product
   remains extended until truncating CRT __ftol; no intermediate float store. */
static DWORD packed(float volume){unsigned long word;double v=volume;if(_isnan(v)||v<0)v=0;else if(v>1)v=1;word=(unsigned long)(v*65535.0);return (word<<16)+word;}
void __cdecl bh_cd_set_volume(BHMediaControl *c,float v){BHCdNotify *n=notify(c);if(!n||!n->cd||n->cd->last_track==-1)return;
 c->aux_packed=packed(v);++c->aux_writes;c->aux_result=auxSetVolume((UINT)n->cd->last_track,c->aux_packed);
}
void __cdecl bh_wave_set_volume(BHMediaControl *c,float v){if(!c||!c->audio||c->audio->wave_device==(HWAVEOUT)-1)return;
 c->wave_packed=packed(v);++c->wave_writes;c->wave_result=waveOutSetVolume(c->audio->wave_device,c->wave_packed);
}
