#include "bh_route_choice.h"
#include <stddef.h>
#include <string.h>
typedef char route_actor_payload[offsetof(BHEntity,payload)==0xe4 ? 1:-1];
typedef char route_word_size[sizeof(unsigned long)==4 && sizeof(long)==4 ? 1:-1];

static int suitable(const BHWorldGeometry *neighbor,long kind) {
    unsigned long flags;
    if(!neighbor)return 0;
    flags=neighbor->query_flags;
    if((flags&0x10) && kind>0)return 0;
    if((flags&0x20) && (kind==0 || kind==6))return 0;
    return 1;
}
int __cdecl bh_route_choose_exit(BHEntity *actor,const BHWorldGeometry *room,
    const BHWorldGeometry *preferred,long excluded,long *selected)
{
    unsigned long count,i,eligible=0,state,rank;
    long kind;unsigned char allowed[6];const BHWorldGeometry *neighbor;
    if(!actor || !room || !room->model || !selected)return 0;
    count=room->model->portal_count;
    if(count>6 || room->navigation.count!=count)return 0;
    memcpy(&kind,actor->payload+24,4);
    if(preferred)for(i=0;i<count;++i) {
        neighbor=(const BHWorldGeometry *)room->navigation.portals[i].neighbor;
        if(neighbor==preferred && suitable(neighbor,kind)){*selected=(long)i;return 1;}
    }
    for(i=0;i<count;++i) {
        neighbor=(const BHWorldGeometry *)room->navigation.portals[i].neighbor;
        allowed[i]=(unsigned char)(suitable(neighbor,kind) && (long)i!=excluded);
        eligible+=allowed[i];
    }
    if(!eligible){*selected=excluded;return 1;}
    memcpy(&state,actor->payload+0x104,4);
    state=((state*0xe3570000UL)|((state*0xe357UL)>>16))^state;
    memcpy(actor->payload+0x104,&state,4);
    rank=((state&0x7fffUL)*eligible)>>15;
    for(i=0;i<count;++i)if(allowed[i]) {
        if(!rank){*selected=(long)i;return 1;}
        --rank;
    }
    /* rank is strictly below eligible for all32-bit states/counts0..6. */
    return 0;
}
