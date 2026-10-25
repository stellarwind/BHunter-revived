#include "../src/bh_player_reset.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL player-reset: %s\n",s);}}while(0)
static void store32(BHEntity *p,unsigned int offset,unsigned long value){
    memcpy((unsigned char*)p+offset,&value,4);
}
static unsigned long load32(BHEntity *p,unsigned int offset){
    unsigned long value;memcpy(&value,(unsigned char*)p+offset,4);return value;
}
int bh_player_reset_checkpoint(unsigned long *count){
    BHEntityPool pool;BHSoundQueue sound;BHWeaponFactory factory;
    BHPlayerReset reset;BHEntity *player,*current,*weapons[10],before;
    BHWeaponObjectState *states[10];
    BHSoundResource *resource;BHSoundVoice *voices[3];
    long frame_count;unsigned int i;
    memset(&pool,0,sizeof(pool));memset(&sound,0,sizeof(sound));
    memset(&factory,0,sizeof(factory));memset(&reset,0,sizeof(reset));
    CHECK(bh_entities_init(&pool,12,NULL),"real entity pool initialized");
    CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL),"real sound owner initialized");
    CHECK(bh_weapon_factory_open(&factory,&pool,&sound),"real weapon factory bound");
    player=bh_entity_acquire(&pool);CHECK(player!=NULL,"live player entity acquired");
    for(i=0;i<10;i++){
        weapons[i]=bh_weapon_factory_create(&factory,i);
        states[i]=bh_weapon_object_state(weapons[i]);
        CHECK(weapons[i] && states[i],"verified factory weapon allocated");
        memcpy((unsigned char*)player+0x258+i*4,&weapons[i],4);
        states[i]->common.available_ec=7;
        states[i]->common.ammo=999;
    }
    resource=bh_sound_resource_allocate(&sound);
    CHECK(resource!=NULL,"real sound resource allocated");
    resource->streaming=1;
    for(i=0;i<3;i++){
        voices[i]=bh_sound_voice_allocate(&sound);
        CHECK(voices[i] && voices[i]->index==i,"real source voice allocated");
        voices[i]->resource=resource;
        voices[i]->previous=i?voices[i-1]:NULL;
        voices[i]->next=NULL;
        if(i)voices[i-1]->next=voices[i];
    }
    resource->stream_head=voices[0];
    memset((unsigned char*)player+0x224,0x5a,0x98);
    for(i=0;i<10;i++)memcpy((unsigned char*)player+0x258+i*4,&weapons[i],4);
    store32(player,0x228,1);store32(player,0x22c,2);store32(player,0x224,0);
    frame_count=77;current=player;
    reset.current_player=&current;reset.weapons=&factory.registry;
    reset.sound=&sound;reset.frame_count=&frame_count;
    CHECK(bh_player_reset(&reset) && reset.error==BH_PLAYER_RESET_OK &&
        reset.attempted_stops==3 && !reset.failed_stops,
        "complete reset stops three live handles in original order");
    CHECK(sound.free_voices==voices[0] && voices[0]->next==voices[2] &&
        voices[2]->next==voices[1] && !resource->stream_head,
        "source sound free list proves +228,+22c,+224 stop order");
    CHECK(load32(player,0x228)==BH_SOUND_INVALID &&
        load32(player,0x22c)==BH_SOUND_INVALID &&
        load32(player,0x224)==BH_SOUND_INVALID &&
        !bh_sound_voice(&sound,0) && !bh_sound_voice(&sound,1) &&
        !bh_sound_voice(&sound,2),"all three stored handles invalidated after stop");
    CHECK(!load32(player,0x23c) && !load32(player,0x254) &&
        load32(player,0x250)==0x3f800000UL && !load32(player,0x24c) &&
        load32(player,0x230)==10000 && !load32(player,0x234) &&
        !load32(player,0x238) && !load32(player,0x248) &&
        !load32(player,0x244) && !load32(player,0x240) &&
        !frame_count,"player status, timing and source global reset");
    CHECK(((unsigned char*)player)[0x2b4]==0 &&
        ((unsigned char*)player)[0x2b5]==0 &&
        ((unsigned char*)player)[0x2b6]==0 &&
        ((unsigned char*)player)[0x2b7]==9 &&
        ((unsigned char*)player)[0x2b8]==0 &&
        ((unsigned char*)player)[0x2b9]==0 &&
        ((unsigned char*)player)[0x2ba]==0 &&
        ((unsigned char*)player)[0x2bb]==0x5a,
        "player selectors and flags change without neighboring byte clobber");
    for(i=0;i<10;i++)CHECK(states[i]->common.available_ec==((i==0 || i==9)?1UL:0UL) &&
        states[i]->common.ammo==(i==0?300L:i==9?1L:0L),
        "all ten weapons clear, then indices zero and nine receive grants");
    CHECK(!bh_player_reset(NULL),"missing reset context rejected");
    frame_count=99;
    memcpy((unsigned char*)player+0x258+4*4,&weapons[5],4);
    before=*player;
    CHECK(!bh_player_reset(&reset) && reset.error==BH_PLAYER_RESET_BINDING &&
        frame_count==99 && !memcmp(player,&before,sizeof(before)) &&
        states[0]->common.ammo==300,"wrong table identity rejected before mutation");
    memcpy((unsigned char*)player+0x258+4*4,&weapons[4],4);
    states[0]->common.ammo_limit=-7;states[9]->common.ammo_limit=-3;
    store32(player,0x228,0x00000077UL); /* stale source handle */
    CHECK(!bh_player_reset(&reset) && reset.error==BH_PLAYER_RESET_SOUND &&
        reset.attempted_stops==1 && reset.failed_stops==1 &&
        load32(player,0x228)==BH_SOUND_INVALID && !frame_count &&
        states[0]->common.ammo==-7 && states[9]->common.ammo==-3 &&
        ((unsigned char*)player)[0x2b7]==9,
        "failed sound stop still completes reset; signed negative limits survive");
    current=NULL;frame_count=42;
    CHECK(!bh_player_reset(&reset) && reset.error==BH_PLAYER_RESET_BINDING &&
        frame_count==42,"missing live current player rejected");
    for(i=0;i<10;i++)CHECK(bh_entity_release(&pool,weapons[i]),
        "owned weapon allocation released");
    CHECK(bh_entity_release(&pool,player),"player allocation released");
    CHECK(bh_weapon_factory_close(&factory),"factory query binding closed");
    CHECK(bh_sound_queue_dispose(&sound) && bh_entities_dispose(&pool),
        "source sound and entity owners disposed");
    if(count)*count=checks;
    printf("{\"scenario\":\"player-reset\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_PLAYER_RESET_STANDALONE
int main(void){unsigned long count;return bh_player_reset_checkpoint(&count);}
#endif
