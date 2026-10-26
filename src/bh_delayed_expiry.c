#include "bh_delayed_expiry.h"
#include "bh_round.h"
#include <string.h>
#include <float.h>
int __cdecl bh_delayed_expiry_bind(BHEntityPool *p,BHEntity *e,float lifetime,float delay){if(!p||!e)return 0;memcpy((char*)e+0xe8,&lifetime,4);memcpy((char*)e+0x198,&delay,4);memcpy(e->payload+608,&p,4);e->step=bh_delayed_expiry_tick;return 1;}
void __cdecl bh_delayed_expiry_tick(BHEntity *e){BHEntityPool *p;float old,stored;double next;memcpy(&p,e->payload+608,4);memcpy(&old,(char*)e+0x198,4);
 if(!_isnan(old)&&old>0){bh_round_float(&stored,(double)old-1.0/30.0);memcpy((char*)e+0x198,&stored,4);return;}
 memcpy(&old,(char*)e+0xe8,4);next=(double)old-1.0/30.0;bh_round_float(&stored,next);memcpy((char*)e+0xe8,&stored,4);
 if(_isnan(next)||next<0)bh_entity_release(p,e);
}
