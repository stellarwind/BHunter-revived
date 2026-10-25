#include "bh_cd_close.h"
int __cdecl bh_cd_stop_close(BHCdClose *c) {
 if(!c)return 0;if(!c->device)return 1;
 c->playing=0;++c->native_calls;c->stop_result=mciSendCommandA(c->device,MCI_STOP,0,0);
 ++c->native_calls;c->close_result=mciSendCommandA(c->device,MCI_CLOSE,0,0);
 c->device=0;c->last_track=-1;return !c->stop_result&&!c->close_result;
}
