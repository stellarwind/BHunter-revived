#include "bh_hit_marker.h"
#include "bh_callback_helpers.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
typedef char marker_layout[sizeof(void*)==4&&offsetof(BHEntity,payload)==0xe4?1:-1];
static BHEntity *link(const BHEntity *e,unsigned long offset){BHEntity*p;memcpy(&p,(const unsigned char*)e+offset,4);return p;}
static void set_link(BHEntity *e,unsigned long offset,BHEntity*p){memcpy((unsigned char*)e+offset,&p,4);}
static void scalar(BHEntity *e,unsigned long offset,float f){memcpy((unsigned char*)e+offset,&f,4);}
static float number(const BHEntity *e,unsigned long offset){float f;memcpy(&f,(const unsigned char*)e+offset,4);return f;}
static int unlink_owner(BHEntity *e){BHEntity *owner=link(e,0xf4),*older=link(e,0xec),*newer=link(e,0xf0);
 if(!owner)return 0;
 if(older)set_link(older,0xf0,newer);
 if(newer)set_link(newer,0xec,older);else set_link(owner,0x12c,older);
 return 1;
}
BHEntity *__cdecl bh_hit_marker_create(BHHitMarkerContext*c,const BHVec3*p,const BHVec3*n,float size,unsigned long flags,BHEntity*owner){
 BHEntity *e,*prior,*old;BHVec3 normal,position,up;BHHitMarkerDraw draw=bh_hit_marker_draw;double dot;
 if(!c||!c->pool||!c->queue||!p||!n||!owner||c->count>200||!bh_math_environment_supported()){
  if(c)c->error=BH_HIT_MARKER_ARGUMENT;return NULL;}
 if((c->count&&(!c->oldest||!c->newest))||(!c->count&&(c->oldest||c->newest))){c->error=BH_HIT_MARKER_TOPOLOGY;return NULL;}
 normal=*n;position=*p;e=bh_entity_acquire(c->pool);if(!e){c->error=BH_HIT_MARKER_POOL;return NULL;}
 scalar(e,0x100,0);scalar(e,0x104,30);scalar(e,0x108,size);scalar(e,0x10c,(float)(flags&1)*.5f);scalar(e,0x110,(float)(flags&2)*.25f);
 memcpy((unsigned char*)e+0x114,&normal,12);
 dot=((double)normal.x*0.0+(double)normal.z*0.0)+(double)normal.y;
 if(dot<.5&&dot>-.5){up.x=-0.0f;up.y=-1;up.z=-0.0f;}else{up.x=0;up.y=0;up.z=1;}
 memcpy((unsigned char*)e+0x120,&up,12);e->position=position;
 memcpy((unsigned char*)e+0xe8,&draw,4);memcpy(e->payload+624,&c,4);
 set_link(e,0xf4,owner);prior=link(owner,0x12c);set_link(e,0xf0,NULL);set_link(e,0xec,prior);set_link(owner,0x12c,e);if(prior)set_link(prior,0xf0,e);
 e->step=NULL;e->refresh=NULL;bh_entity_set_query(c->pool,e,bh_marker_query);
 if(c->count==200){old=c->oldest;c->oldest=link(old,0xfc);set_link(c->oldest,0xf8,NULL);unlink_owner(old);bh_entity_release(c->pool,old);}
 else ++c->count;
 if(c->count==1)c->oldest=e;
 set_link(e,0xf8,c->newest);if(c->newest)set_link(c->newest,0xfc,e);c->newest=e;
 c->error=BH_HIT_MARKER_OK;return e;
}
void __cdecl bh_hit_marker_draw(BHEntity*e){BHHitMarkerContext*c;BHVec3 up,normal;float size;int r;
 if(!e)return;memcpy(&c,e->payload+624,4);if(!c)return;
 size=number(e,0x108);if(!(size>0))return;
 memcpy(&up,(unsigned char*)e+0x120,12);memcpy(&normal,(unsigned char*)e+0x114,12);
 r=bh_quad_queue(c->queue,0,c->image,&e->position,&up,&normal,size,size,0,0,1,number(e,0x10c),.5f,number(e,0x110),.5f);
 c->error=r==1?BH_HIT_MARKER_OK:BH_HIT_MARKER_QUEUE;
}
int __cdecl bh_hit_marker_dispose(BHHitMarkerContext*c){BHEntity*e,*next;
 if(!c||!c->pool)return 0;
 while(c->count){e=c->oldest;if(!e||!unlink_owner(e)){c->error=BH_HIT_MARKER_TOPOLOGY;return 0;}
  next=c->count>1?link(e,0xfc):NULL;if(!bh_entity_release(c->pool,e)){c->error=BH_HIT_MARKER_POOL;return 0;}
  c->oldest=next;--c->count;
 }
 c->newest=NULL;c->error=BH_HIT_MARKER_OK;return 1;
}
