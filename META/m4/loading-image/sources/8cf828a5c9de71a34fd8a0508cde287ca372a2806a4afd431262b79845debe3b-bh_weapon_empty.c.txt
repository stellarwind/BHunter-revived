#include "bh_weapon_empty.h"
#include <string.h>

const char bh_weapon_empty_warning[]=
    "5Hello Mister,\n\nseems like you're\nfresh out of ammo\non that one!\n\n"
    "Come visit one of my\nshops for a refill.\n";

static int empty_notice(BHWeaponEmptyContext *context,const BHEntity *player){
    unsigned long handle;int message_ok=1,sound_ok=1;
    if(!context || !context->sound || !player){
        if(context)context->error=BH_WEAPON_EMPTY_ARGUMENT;return 0;
    }
    handle=bh_sound_request(context->sound,context->shift_sound,.8f,1.5f,
                            &player->position,&player->origin);
    ++context->requests;sound_ok=handle!=BH_SOUND_INVALID;
    if(!context->warning_seen){context->error=BH_WEAPON_EMPTY_ARGUMENT;return 0;}
    if(!*context->warning_seen){
        if(!context->messages)message_ok=0;
        else message_ok=bh_message_enqueue(context->messages,bh_weapon_empty_warning);
        *context->warning_seen=1;++context->warnings;
    }
    context->error=!sound_ok?BH_WEAPON_EMPTY_SOUND:
                   !message_ok?BH_WEAPON_EMPTY_MESSAGE:BH_WEAPON_EMPTY_OK;
    return context->error==BH_WEAPON_EMPTY_OK;
}
int __cdecl bh_weapon_empty_trigger(BHWeaponEmptyContext *context,
    unsigned long identity,BHEntity *weapon,const BHEntity *player){
    BHWeaponCommonState *state;
    if(!context || !weapon){if(context)context->error=BH_WEAPON_EMPTY_ARGUMENT;return 0;}
    state=bh_weapon_common_state(weapon);
    switch(identity){
    case 0x0042f730UL:state->word_e8&=~1UL;break;
    case 0x004309d0UL:state->word_e8=0;break;
    case 0x00432560UL:state->word_e8=0;state->word_108=1;break;
    default:context->error=BH_WEAPON_EMPTY_METHOD;return 0;
    }
    if(state->ammo!=0){context->error=BH_WEAPON_EMPTY_OK;return 1;}
    return empty_notice(context,player);
}
int __cdecl bh_weapon_index9_trigger(BHWeaponEmptyContext *context,
    BHEntity *weapon,BHEntity *player){
    BHWeaponCommonState *state;float timer;
    if(!context || !weapon || !player){
        if(context)context->error=BH_WEAPON_EMPTY_ARGUMENT;return 0;
    }
    memcpy(&timer,(unsigned char*)player+0x240,4);
    if(timer>0){context->error=BH_WEAPON_EMPTY_OK;return 1;}
    state=bh_weapon_common_state(weapon);
    if(state->ammo==0)return empty_notice(context,player);
    state->ammo=(long)((unsigned long)state->ammo-1UL);
    timer=15.0f;memcpy((unsigned char*)player+0x240,&timer,4);
    memcpy((unsigned char*)player+0x248,&timer,4);
    timer=1.0f;memcpy((unsigned char*)player+0x244,&timer,4);
    context->error=BH_WEAPON_EMPTY_OK;return 1;
}
int __cdecl bh_weapon_empty_dispatch(BHWeaponEmptyContext *context,
    BHWeaponRegistry *registry,unsigned int selection,BHEntity *player){
    BHEntity *entity;BHWeaponObjectState *state;
    if(!context || !registry){if(context)context->error=BH_WEAPON_EMPTY_ARGUMENT;return 0;}
    if(!bh_weapon_registry_resolve(registry,selection,&entity,&state)){
        context->error=BH_WEAPON_EMPTY_STALE;return 0;
    }
    if(state->method_124==0x00433dc0UL)
        return bh_weapon_index9_trigger(context,entity,player);
    return bh_weapon_empty_trigger(context,state->method_124,entity,player);
}
