#include "bh_actor_vehicle_lifetime.h"
#include <string.h>
static BHEntity *link_at(BHEntity *e,unsigned long offset){BHEntity *p;memcpy(&p,(char*)e+offset,4);return p;}
static void set_link(BHEntity *e,unsigned long offset,BHEntity *p){memcpy((char*)e+offset,&p,4);}
int __cdecl bh_actor_vehicle_release(BHActorVehicleLifetimeContext *c,BHEntity *e){
 BHActorRouteState s;BHPickupRoom *room=0;BHEntity *at,*previous,*next;unsigned long i,n=0;struct {long event;BHEntity *actor;} packet;
 if(!c||!c->pool||!c->route||!c->route->sound||!e||!c->count)return 0;
 for(at=c->pool->active;at&&at!=e&&n++<c->pool->capacity;at=at->next){}if(at!=e)return 0;
 bh_actor_route_read(e,&s);for(i=0;i<c->route->room_count;++i)if(c->route->rooms[i].world==s.current){room=c->route->rooms+i;break;}if(!room)return 0;
 previous=link_at(e,0xf8);next=link_at(e,0xf4);if((previous?link_at(previous,0xf4):room->actors)!=e||(next&&link_at(next,0xf8)!=e))return 0;
 previous=link_at(e,0xec);next=link_at(e,0xf0);if((previous?link_at(previous,0xf0):c->head)!=e||(next&&link_at(next,0xec)!=e))return 0;
 if(s.sound!=BH_SOUND_INVALID)bh_sound_stop(c->route->sound,s.sound);
 packet.event=17;packet.actor=e;bh_entities_query(c->pool,0,0,0,0,&packet);
 previous=link_at(e,0xf8);next=link_at(e,0xf4);if(previous)set_link(previous,0xf4,next);else room->actors=next;if(next)set_link(next,0xf8,previous);
 previous=link_at(e,0xec);next=link_at(e,0xf0);if(previous)set_link(previous,0xf0,next);else c->head=next;if(next)set_link(next,0xec,previous);
 --c->count;return bh_entity_release(c->pool,e);
}
