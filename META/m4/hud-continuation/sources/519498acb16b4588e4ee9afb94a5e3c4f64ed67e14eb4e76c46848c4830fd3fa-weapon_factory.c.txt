#include "../src/bh_weapon_factory.h"
#include "../src/bh_weapon_timer.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-factory: %s\n",s);}}while(0)
int bh_weapon_factory_checkpoint(unsigned long *count){
    BHEntityPool pool;BHSoundQueue sound;BHWeaponFactory factory;
    BHEntity *entity,*resolved,*target,*reused;BHWeaponObjectState *state,*view;
    BHEntity timer,before;BHWeaponCommonState *timed;
    BHMathEnvironment fp;
    BHWeaponRemovedPacket packet;unsigned long identity;unsigned int i;
    static const long limits[10]={750,400,150,50,300,15,10,4,1,8};
    memset(&pool,0,sizeof(pool));memset(&sound,0,sizeof(sound));
    CHECK(bh_math_environment_enter(&fp),"PC53 nearest masked timer test environment");
    memset(&timer,0xa5,sizeof(timer));timed=bh_weapon_common_state(&timer);
    timed->word_f0=0x3f800000UL;before=timer;
    CHECK(bh_weapon_single_timer_tick(&timer) && timed->word_f0==0x3f777777UL,
        "index7/8 shared timer subtracts original double 1/30 from positive 1.0");
    timed->word_f0=0x3c23d70aUL; /* exact binary32 0.01 */
    CHECK(bh_weapon_single_timer_tick(&timer) && timed->word_f0==0xbcbf258cUL,
        "small positive timer crosses below zero without clamping");
    before=timer;CHECK(bh_weapon_single_timer_tick(&timer) && !memcmp(&timer,&before,sizeof(timer)),
        "negative timer remains byte-identical on subsequent tick");
    timed->word_f0=0x80000000UL;before=timer;
    CHECK(bh_weapon_single_timer_tick(&timer) && !memcmp(&timer,&before,sizeof(timer)),
        "negative zero and all unrelated weapon bytes survive");
    timed->word_f0=0x7fc12345UL;before=timer;
    CHECK(bh_weapon_single_timer_tick(&timer) && !memcmp(&timer,&before,sizeof(timer)),
        "unordered NaN timer retains exact payload");
    CHECK(!bh_weapon_single_timer_tick(NULL),"timer rejects absent weapon owner");
    CHECK(bh_math_environment_supported(),"timer preserves masked PC53 caller profile");
    bh_math_environment_leave(&fp);
    CHECK(bh_entities_init(&pool,12,NULL),"real entity pool initialized");
    CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL),"real sound owner initialized without playback");
    CHECK(bh_weapon_factory_open(&factory,&pool,&sound),"explicit query context bound");
    CHECK(!bh_weapon_factory_open(&factory,&pool,&sound),"second global query binding rejected");
    for(i=0;i<10;i++){
        entity=bh_weapon_factory_create(&factory,i);state=bh_weapon_object_state(entity);
        CHECK(entity && state && state->common.ammo_limit==limits[i],"factory allocates actual pool record and verified ammo limit");
        CHECK(state && (unsigned char*)state==(unsigned char*)entity+0xe8 &&
            (unsigned char*)&state->common.hud==(unsigned char*)entity+0xf8 &&
            (unsigned char*)&state->common.ammo==(unsigned char*)entity+0x10c &&
            (unsigned char*)&state->method_130==(unsigned char*)entity+0x130,
            "one authoritative entity allocation carries HUD, ammo and selected method");
        CHECK(state && !state->common.word_e8 && !state->common.available_ec &&
            !state->common.word_f0 && !state->common.word_f4 && !state->common.hud.target &&
            !state->common.hud.charge && state->common.word_100==-1 &&
            !state->common.word_104 && !state->common.word_108 && !state->common.ammo &&
            !state->common.delay,"complete original initialization subset");
        CHECK(bh_weapon_registry_hud(&factory.registry,i,&resolved,&view,&identity) &&
            resolved==entity && view==state &&
            identity==(i==4?0x00432ff0UL:i==5?0x004314f0UL:0x0047d680UL),
            "registry resolves actual live allocation and verified selected HUD identity");
        CHECK(entity && entity->query && factory.registry.slots[i].generation==entity->generation,
            "real query owner installed with pool generation");
    }
    CHECK(bh_entities_valid(&pool) && pool.used==10,"pool links and query registrations valid");
    for(i=7;i<=8;i++){
        entity=factory.registry.slots[i].entity;state=bh_weapon_object_state(entity);
        state->common.word_f0=0x3f800000UL;
        CHECK(bh_weapon_factory_tick(&factory,i) && state->common.word_f0==0x3f777777UL,
            "each distinct index7/8 method identity routes to shared real timer owner");
    }
    entity=factory.registry.slots[2].entity;before=*entity;
    CHECK(!bh_weapon_factory_tick(&factory,2) && factory.error==BH_WEAPON_FACTORY_METHOD &&
        !memcmp(entity,&before,sizeof(*entity)),
        "unrecovered gameplay method reports explicit error without altering weapon");
    CHECK(!bh_weapon_factory_create(&factory,10) && factory.error==BH_WEAPON_FACTORY_ARGUMENT,
        "invalid table index rejected before any table access");
    CHECK(!bh_weapon_factory_create(&factory,4) && factory.error==BH_WEAPON_FACTORY_BINDING,
        "factory cannot silently replace a live index allocation");
    CHECK(!bh_weapon_factory_close(&factory),"factory cannot unbind while weapon queries remain live");
    target=bh_entity_acquire(&pool);CHECK(target!=NULL,"target has real pool allocation");
    state=bh_weapon_object_state(factory.registry.slots[4].entity);
    state->common.hud.target=target;state->common.hud.charge=17;state->common.word_100=-1;
    packet.event=17;packet.entity=target;
    factory.error=BH_WEAPON_FACTORY_OK;
    CHECK(bh_entities_query(&pool,0,0,NULL,0,&packet),"event17 travels through actual pool query list");
    CHECK(!state->common.hud.target && state->common.hud.charge==17 && state->common.word_100==-1 &&
        factory.error==BH_WEAPON_FACTORY_OK,
        "query clears matching target through real removal owner and keeps charge");
    /* The target itself has no weapon query; the other nine entries have null
       targets, so no sound binding error should be raised for this packet. */
    factory.error=BH_WEAPON_FACTORY_OK;
    state->common.hud.target=target;state->common.word_100=-1;
    CHECK(bh_entities_query(&pool,0,0,NULL,0,&packet) && factory.error==BH_WEAPON_FACTORY_OK &&
        !state->common.hud.target,"query completes with no error when no sound handle needs stopping");
    entity=factory.registry.slots[4].entity;
    CHECK(bh_entity_release(&pool,factory.registry.slots[5].entity),
        "another index released first to test cross-index address reuse");
    CHECK(bh_entity_release(&pool,entity),"owned weapon allocation released through real pool");
    CHECK(!bh_weapon_registry_resolve(&factory.registry,4,&resolved,&view) && !resolved && !view,
        "released generation explicitly rejected");
    reused=bh_entity_acquire(&pool);CHECK(reused==entity,"pool reuses same address");
    CHECK(!bh_weapon_registry_resolve(&factory.registry,4,&resolved,&view),
        "reused address with new generation is a distinct weapon");
    CHECK(bh_entity_release(&pool,reused),"temporary allocation returned");
    entity=bh_weapon_factory_create(&factory,5);
    CHECK(entity==factory.registry.slots[4].entity &&
        bh_weapon_registry_hud(&factory.registry,5,&resolved,&view,&identity) &&
        identity==0x004314f0UL,"different index reuses stale slot address with new generation");
    view->common.hud.target=target;view->common.word_100=-1;
    CHECK(bh_entities_query(&pool,0,0,NULL,0,&packet) &&
        !view->common.hud.target && factory.error==BH_WEAPON_FACTORY_OK,
        "query dispatch selects current generation when stale index shares address");
    entity=bh_weapon_factory_create(&factory,4);
    CHECK(entity && bh_weapon_registry_hud(&factory.registry,4,&resolved,&view,&identity) &&
        identity==0x00432ff0UL,"real factory can create replacement index after release");
    CHECK(!bh_weapon_method_available(0x0047d680UL) &&
        !bh_weapon_method_available(0x004327a0UL) &&
        bh_weapon_method_available(0x00432ff0UL) &&
        bh_weapon_method_available(0x004314f0UL) &&
        bh_weapon_method_available(0x00433820UL) &&
        bh_weapon_method_available(0x00433b00UL),
        "unrecovered methods and verified RET have no callable fake implementation");
    for(i=0;i<10;i++)CHECK(bh_entity_release(&pool,factory.registry.slots[i].entity),
        "player owner releases each borrowed weapon allocation");
    CHECK(bh_entity_release(&pool,target),"target allocation released");
    CHECK(bh_weapon_factory_close(&factory),"query binding closes after all owned weapons release");
    CHECK(bh_entities_valid(&pool) && !pool.used,"real pool fully valid after lifecycle");
    CHECK(bh_sound_queue_dispose(&sound) && bh_entities_dispose(&pool),"real source owners disposed");
    if(count)*count=checks;
    printf("{\"scenario\":\"weapon-factory\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_WEAPON_FACTORY_STANDALONE
int main(void){unsigned long count;return bh_weapon_factory_checkpoint(&count);}
#endif
