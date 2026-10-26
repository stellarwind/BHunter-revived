#include "bh_script_flash.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
typedef char flash_layout[sizeof(void*)==4&&offsetof(BHEntity,payload)==0xe4?1:-1];
static BHEntityPool *pool(BHEntity*e){BHEntityPool*p;memcpy(&p,e->payload+456,4);return p;}
float __cdecl bh_script_flash_countdown(const BHEntity*e){float f=0;if(e)memcpy(&f,e->payload+4,4);return f;}
int __cdecl bh_script_flash_bind(BHEntityPool*p,BHEntity*e,float countdown){BHEntity*at;unsigned long n=0;int found=0;
 if(!p||!e||!bh_entities_valid(p))return 0;for(at=p->active;at&&n++<p->capacity;at=at->next)if(at==e)found=1;if(!found)return 0;
 memcpy(e->payload+4,&countdown,4);memcpy(e->payload+456,&p,4);e->step=bh_script_flash_tick;return 1;
}
void __cdecl bh_script_flash_tick(BHEntity*e){float f;double value;
 if(!bh_math_environment_supported())return;
 value=(double)bh_script_flash_countdown(e)-(1.0/30.0);bh_round_float(&f,value);memcpy(e->payload+4,&f,4);
 if(!(value>0))bh_entity_release(pool(e),e);
}
