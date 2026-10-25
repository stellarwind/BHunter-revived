#include "bh_player_input.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char binding_size[sizeof(BHInputBinding)==0x20 ? 1 : -1];
typedef char binding_name[offsetof(BHInputBinding,name)==0xc ? 1 : -1];
typedef char list_size[sizeof(BHPlayerInputList)==0x2130 ? 1 : -1];
typedef char list_bindings[offsetof(BHPlayerInputList,bindings)==0x10 ? 1 : -1];
typedef char list_state[offsetof(BHPlayerInputList,state)==0x2010 ? 1 : -1];
typedef char profile_size[sizeof(BHInputProfile)==0x4c ? 1 : -1];

static void *__cdecl allocate(unsigned long bytes,void *user) {return malloc(bytes);}
static void __cdecl release(void *memory,void *user) {free(memory);}

int __cdecl bh_player_input_owner_init(BHPlayerInputOwner *owner,
                                      const BHSceneAllocator *allocator) {
    if(!owner || (allocator && (!allocator->allocate || !allocator->release)))return 0;
    owner->first=NULL;
    if(allocator)owner->allocator=*allocator;
    else {owner->allocator.allocate=allocate;owner->allocator.release=release;owner->allocator.user=NULL;}
    return 1;
}

void __cdecl bh_player_input_owner_dispose(BHPlayerInputOwner *owner) {
    BHPlayerInputList *list,*next;
    if(!owner)return;
    list=owner->first;
    while(list) {
        next=list->next;
        owner->allocator.release(list,owner->allocator.user);
        list=next;
    }
    owner->first=NULL;
}

BHPlayerInputList *__cdecl bh_player_input_create(BHPlayerInputOwner *owner,int mode) {
    BHPlayerInputList *list;
    unsigned long i;
    list=(BHPlayerInputList*)owner->allocator.allocate(sizeof(*list),owner->allocator.user);
    if(!list)return NULL;
    list->flags=mode ? 2 : 0;
    list->previous=NULL;list->next=owner->first;owner->first=list;
    if(list->next)list->next->previous=list;
    for(i=0;i<256;++i)list->bindings[i].kind=0;
    list->count=0;list->state[1]=0;list->state[0]=0;
    return list;
}

static BHInputBinding *binding(BHPlayerInputList *list,long index,const char *name) {
    BHInputBinding *entry=&list->bindings[index];
    unsigned long i=0;
    while(name[i] && i<19) {entry->name[i]=name[i];++i;}
    entry->name[i]=0;
    if(list->count<index+1)list->count=index+1;
    return entry;
}

void __cdecl bh_player_input_bind_axis(BHPlayerInputList *list,long index,
    const char *name,unsigned char flags,unsigned short modifiers,
    unsigned short first,unsigned short second,unsigned short third,unsigned short fourth) {
    BHInputBinding *entry=binding(list,index,name);
    entry->flags=flags;entry->modifiers=modifiers;entry->key[1]=first;
    entry->kind=2;entry->key[0]=second;entry->key[2]=third;entry->key[3]=fourth;
}

void __cdecl bh_player_input_bind_key(BHPlayerInputList *list,long index,
    const char *name,unsigned char flags,unsigned short modifiers,unsigned short key) {
    BHInputBinding *entry=binding(list,index,name);
    entry->kind=1;entry->flags=flags;entry->modifiers=modifiers;entry->key[0]=key;
}

void __cdecl bh_player_input_bind_string(BHPlayerInputList *list,long index,const char *name) {
    binding(list,index,name)->kind=5;
}

void __cdecl bh_player_input_setup(BHPlayerInputList *list,const BHInputProfile *profile) {
    static const unsigned char axes[5][5]={
        {1,0x1c,0x1d,0x22,0x25},{0,0x1e,0x1f,0x23,0x26},
        {0x19,0x40,0x41,0x45,0x47},{0x1a,0x42,0x43,0x44,0x46},
        {2,0x21,0x20,0x24,0x27}
    };
    static const unsigned char keys[24][2]={
        {3,0x31},{5,0x32},{4,0x33},{0x16,0x28},{0x18,0x29},{0x17,0x2a},
        {0xc,0x2c},{0xd,0x2d},{0xe,0x2e},{0xf,0x2f},{0x10,0x30},
        {0x11,0x35},{0x12,0x36},{0x13,0x37},{0x14,0x38},{0x15,0x39},
        {6,0x3a},{7,0x3b},{8,0x3c},{9,0x3d},{10,0x3e},{11,0x3f},
        {0x1b,0x2b},{0x1c,0x34}
    };
    const unsigned char *bytes=(const unsigned char*)profile;
    unsigned long i;
    for(i=0;i<5;++i)bh_player_input_bind_axis(list,axes[i][0],"",0,0,
        bytes[axes[i][1]],bytes[axes[i][2]],bytes[axes[i][3]],bytes[axes[i][4]]);
    for(i=0;i<24;++i)bh_player_input_bind_key(list,keys[i][0],"",0,0,bytes[keys[i][1]]);
    bh_player_input_bind_key(list,0x21,"",0,0,0x1b);
    bh_player_input_bind_key(list,0x20,"",0,0,9);
    bh_player_input_bind_key(list,0x22,"",0,0,0x59);
    bh_player_input_bind_key(list,0x23,"",0,0,0x4e);
}

void __cdecl bh_player_input_apply_packet(BHPlayerInputState *state,const void *packet) {
    const unsigned char *p=(const unsigned char*)packet+2;
    unsigned long records=((const unsigned char*)packet)[1],n,i;
    while(records--) {
        switch(*p) {
        case 0:case 1:
            n=p[1];
            for(i=0;i<n;++i)state->values[p[2+i]]=(*p==0 ? 1 : 0);
            p+=((n+5)&~3u);
            break;
        case 2:case 3:case 4:
            memcpy(&state->values[p[1]],p+2,4);p+=6;break;
        default:++p;break;
        }
    }
}

int __cdecl bh_player_input_apply_packet_checked(BHPlayerInputState *state,
                                                const void *packet,unsigned long bytes) {
    const unsigned char *p=(const unsigned char*)packet;
    unsigned long records,offset=2,record_bytes;
    if(!state || !packet || bytes<2)return 0;
    records=p[1];
    while(records--) {
        if(offset>=bytes)return 0;
        switch(p[offset]) {
        case 0:case 1:
            if(bytes-offset<2)return 0;
            record_bytes=(p[offset+1]+5u)&~3u;break;
        case 2:case 3:case 4:record_bytes=6;break;
        default:record_bytes=1;break;
        }
        if(record_bytes>bytes-offset)return 0;
        offset+=record_bytes;
    }
    bh_player_input_apply_packet(state,packet);return 1;
}

unsigned char __cdecl bh_player_input_transition(BHPlayerInputState *state,unsigned long index) {
    unsigned long value=state->values[index];
    if(value && !(value&0x80000000u)) {
        state->values[index]=value+1;
        return (unsigned char)(value==1 ? 0 : 2);
    }
    state->values[index]=value-1;
    return (unsigned char)(value==0 ? 1 : 3);
}
