#include "bh_projectile_lifetime.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
typedef char projectile_layout[sizeof(void*)==4&&offsetof(BHEntity,payload)==0xe4&&offsetof(BHProjectileSyncPacket,position)==16?1:-1];
static void get(const BHEntity *e,unsigned long offset,void *out){memcpy(out,(const unsigned char*)e+offset,4);}
static void put(BHEntity *e,unsigned long offset,const void *value){memcpy((unsigned char*)e+offset,value,4);}
static BHProjectileLifetime *context(BHEntity *e){BHProjectileLifetime *c;memcpy(&c,e->payload+468,4);return c;}
static int live(BHProjectileLifetime *c,const BHEntity *e,int bound){BHEntity *p;unsigned long n=0,g;
 if(!c||!c->pool||!c->sound||!e)return 0;
 if(bound)memcpy(&g,e->payload+472,4);
 for(p=c->pool->active;p&&n++<c->pool->capacity;p=p->next)if(p==e)return !bound||p->generation==g;
 return 0;
}
int __cdecl bh_projectile_query_state(const BHEntity *e,BHProjectileQueryState *s){if(!e||!s)return 0;
 get(e,0xe8,&s->countdown);get(e,0x118,&s->sound);get(e,0x11c,&s->identity);get(e,0x12c,&s->target);get(e,0x130,&s->secondary);return 1;
}
int __cdecl bh_projectile_lifetime_bind(BHProjectileLifetime *c,BHEntity *e,const BHProjectileQueryState *s){
 if(!s||!live(c,e,0))return 0;
 put(e,0xe8,&s->countdown);put(e,0x118,&s->sound);put(e,0x11c,&s->identity);put(e,0x12c,&s->target);put(e,0x130,&s->secondary);
 memcpy(e->payload+468,&c,4);memcpy(e->payload+472,&e->generation,4);c->error=0;
 return bh_entity_set_query(c->pool,e,bh_projectile_query);
}
void __cdecl bh_projectile_cleanup(BHEntity *e){BHProjectileLifetime *c=context(e);unsigned long handle;
 if(!live(c,e,1)){if(c)c->error=1;return;}
 get(e,0x118,&handle);bh_sound_stop(c->sound,handle);bh_entity_release(c->pool,e);
}
void __cdecl bh_projectile_query(BHEntity *e,void *packet){BHProjectileLifetime *c=context(e);BHProjectileQueryState s;long event;unsigned long identity,kind;BHEntity *removed;
 if(!packet||!live(c,e,1)){if(c)c->error=1;return;}
 memcpy(&event,packet,4);bh_projectile_query_state(e,&s);
 if(event==20){e->step=bh_projectile_cleanup;bh_entity_set_query(c->pool,e,NULL);e->refresh=NULL;return;}
 if(event==17){memcpy(&removed,(unsigned char*)packet+4,4);
  if(removed==s.target){s.target=NULL;put(e,0x12c,&s.target);}
  if(removed==s.secondary){s.secondary=NULL;put(e,0x130,&s.secondary);}return;
 }
 if(event!=18)return;
 memcpy(&identity,(unsigned char*)packet+4,4);if(identity!=s.identity)return;
 identity=0xffffffffUL;memcpy((unsigned char*)packet+4,&identity,4);memcpy(&kind,(unsigned char*)packet+8,4);
 if(kind!=3)return;
 if(!_isnan(s.countdown)&&(double)s.countdown>.2){s.countdown=.2f;put(e,0xe8,&s.countdown);}
 memcpy(&e->position,(unsigned char*)packet+16,12);identity=1;put(e,0x11c,&identity);
}
