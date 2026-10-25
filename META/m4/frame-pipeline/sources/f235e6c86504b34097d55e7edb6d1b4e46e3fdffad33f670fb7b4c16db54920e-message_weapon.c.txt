#include "../src/bh_weapon_empty.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL message-weapon: %s\n",s);}}while(0)
static unsigned long raw_float(const void *p){unsigned long word;memcpy(&word,p,4);return word;}
int bh_message_weapon_checkpoint(unsigned long *count){
    BHMessageState messages,before;
    BHEntity panel,player,*weapon,*resolved;BHWeaponObjectState *state;
    BHEntityPool pool;BHSoundQueue sound;BHWeaponFactory factory;
    BHWeaponEmptyContext context;BHMathEnvironment fp;
    LPDIRECTSOUND device=NULL;BHWave wave;unsigned char samples[64];
    unsigned long panel_id=0x00442950UL,resource,handle;long warning_seen=0;
    unsigned int i;char label[4];
    checks=failures=0;
    CHECK(bh_math_environment_enter(&fp),"masked PC53 caller profile");
    memset(&messages,0,sizeof(messages));memset(&panel,0,sizeof(panel));
    memset(&player,0,sizeof(player));
    messages.panel_entity=&panel;messages.panel_step_identity=&panel_id;
    *(float*)(panel.payload+4)=5.0f;
    CHECK(bh_message_enqueue(&messages,"one") && messages.ring_count==1 &&
        messages.ring_start==0 && messages.last_index==0 &&
        messages.arena_used==4 && !strcmp(messages.ring[0],"one") &&
        !strcmp(messages.arena,"one") && messages.display_ticks==64 &&
        messages.display_phase==0 && raw_float(panel.payload+4)==0x40400000UL,
        "complete queue path appends ring and arena then clamps matching panel timer to3");
    panel_id=0;*(float*)(panel.payload+4)=5.0f;
    CHECK(bh_message_enqueue(&messages,"two") && raw_float(panel.payload+4)==0x40a00000UL,
        "different live panel step identity preserves timer");
    panel_id=0x00442950UL;
    for(i=2;i<9;i++){
        label[0]='0'+(char)i;label[1]=0;
        CHECK(bh_message_enqueue(&messages,label),"ring append with real source state");
    }
    CHECK(messages.ring_count==8 && messages.ring_start==1 &&
        messages.last_index==0 && !strcmp(messages.ring[0],"8") &&
        !strcmp(messages.ring[1],"two"),
        "ninth message overwrites oldest slot and retains following slot");
    before=messages;messages.arena_used=8192;
    CHECK(!bh_message_enqueue(&messages,"overflow") &&
        messages.error==BH_MESSAGE_ARENA_FULL && messages.ring_count==8 &&
        messages.last_index==1 && !strcmp(messages.ring[1],"overflow") &&
        messages.arena_used==8192 &&
        !memcmp(messages.arena,before.arena,sizeof(messages.arena)),
        "arena exhaustion returns0 after original ring/display side effects");
    messages.arena_used=before.arena_used;
    before=messages;
    {char long_text[514];memset(long_text,'x',513);long_text[513]=0;
        CHECK(!bh_message_enqueue(&messages,long_text) &&
            messages.error==BH_MESSAGE_RING_SIZE &&
            messages.ring_start==before.ring_start &&
            messages.ring_count==before.ring_count &&
            !memcmp(messages.ring,before.ring,sizeof(messages.ring)),
            "oversize source input explicitly rejected before unsafe original overflow");}
    memset(&messages,0,sizeof(messages));messages.panel_entity=&panel;
    messages.panel_step_identity=&panel_id;
    memset(&pool,0,sizeof(pool));memset(&sound,0,sizeof(sound));memset(&wave,0,sizeof(wave));
    CHECK(bh_entities_init(&pool,12,NULL),"real entity pool for factory identities");
    CHECK(DirectSoundCreate(NULL,&device,NULL)==DS_OK && device,
        "real DirectSound interface for queued sound request, no playback");
    if(!device)goto done;
    CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK &&
          bh_sound_queue_init(&sound,device,NULL,NULL),"real sound queue ownership");
    memset(samples,0,sizeof(samples));wave.format_tag=1;wave.channels=1;
    wave.sample_rate=8000;wave.byte_rate=16000;wave.block_align=2;
    wave.bits=16;wave.sample_bytes=64;wave.samples=samples;
    resource=bh_sound_register_wave(&sound,&wave,0,3,0);
    CHECK(resource!=BH_SOUND_INVALID,"real PCM resource available to trigger method");
    if(resource==BH_SOUND_INVALID)goto done;
    CHECK(bh_weapon_factory_open(&factory,&pool,&sound),"factory binds real query owner");
    for(i=0;i<5;i++)CHECK(bh_weapon_factory_create(&factory,i)!=NULL,
        "factory installs actual verified +124 identity for each weapon");
    CHECK(bh_weapon_factory_create(&factory,9)!=NULL,
        "factory installs verified index9 +124 identity");
    CHECK(bh_weapon_method_available(0x0042f730UL) &&
        bh_weapon_method_available(0x004309d0UL) &&
        bh_weapon_method_available(0x00432560UL) &&
        bh_weapon_method_available(0x00433dc0UL) &&
        !bh_weapon_method_available(0x004331c0UL),
        "recovered +124 identities declared; unrecovered index3 stays unavailable");
    memset(&context,0,sizeof(context));context.sound=&sound;
    context.shift_sound=resource;context.messages=&messages;
    context.warning_seen=&warning_seen;
    player.position.x=3;player.origin.x=-4;
    weapon=factory.registry.slots[0].entity;state=bh_weapon_object_state(weapon);
    state->common.ammo=5;state->common.word_e8=0x7;
    CHECK(bh_weapon_empty_trigger(&context,0x0042f730UL,weapon,&player) &&
        state->common.word_e8==0x6 && context.requests==0,
        "direct source owner call links complete index0/1 nonempty branch");
    state->common.word_e8=0x7;
    CHECK(bh_weapon_empty_dispatch(&context,&factory.registry,0,&player) &&
        state->common.word_e8==0x6 && context.requests==0 && !warning_seen,
        "index0/1 trigger clears only bit0 and leaves nonempty ammo/sound alone");
    weapon=factory.registry.slots[2].entity;state=bh_weapon_object_state(weapon);
    state->common.ammo=3;state->common.word_e8=0x7;
    CHECK(bh_weapon_empty_dispatch(&context,&factory.registry,2,&player) &&
        state->common.word_e8==0 && context.requests==0,
        "index2 trigger clears whole e8 word");
    weapon=factory.registry.slots[4].entity;state=bh_weapon_object_state(weapon);
    state->common.ammo=3;state->common.word_e8=0x7;
    CHECK(bh_weapon_empty_dispatch(&context,&factory.registry,4,&player) &&
        state->common.word_e8==0 && state->common.word_108==1,
        "index4 trigger clears e8 and sets latch+108 even with ammo");
    weapon=factory.registry.slots[0].entity;state=bh_weapon_object_state(weapon);
    state->common.ammo=0;state->common.word_e8=0x7;
    CHECK(bh_weapon_empty_dispatch(&context,&factory.registry,0,&player) &&
        context.requests==1 && context.warnings==1 && warning_seen==1 &&
        messages.ring_count==1 && !strcmp(messages.ring[0],bh_weapon_empty_warning),
        "empty ammo queues actual sound and exact once-only refill message");
    CHECK(sound.resources[resource]->voices[0] &&
        sound.resources[resource]->voices[0]->position.x==3 &&
        sound.resources[resource]->voices[0]->velocity.x==-4,
        "sound owner receives live player +0c/+48 vectors");
    if(sound.resources[resource]->voices[0]){
        BHSoundVoice *voice=sound.resources[resource]->voices[0];
        handle=voice->index+((unsigned long)voice->generation<<16);
        CHECK(bh_sound_stop(&sound,handle)==1,"queued voice stopped without playback");
    }
    state=bh_weapon_object_state(factory.registry.slots[1].entity);state->common.ammo=0;
    CHECK(bh_weapon_empty_dispatch(&context,&factory.registry,1,&player) &&
        context.requests==2 && context.warnings==1 && messages.ring_count==1,
        "later empty trigger still requests sound but does not duplicate warning");
    if(sound.resources[resource]->voices[0]){
        BHSoundVoice *voice=sound.resources[resource]->voices[0];
        handle=voice->index+((unsigned long)voice->generation<<16);
        CHECK(bh_sound_stop(&sound,handle)==1,"second queued voice stopped");
    }
    warning_seen=0;messages.arena_used=8192;
    CHECK(!bh_weapon_empty_dispatch(&context,&factory.registry,1,&player) &&
        context.error==BH_WEAPON_EMPTY_MESSAGE && warning_seen==1 &&
        context.warnings==2 && messages.error==BH_MESSAGE_ARENA_FULL,
        "message failure reported after original one-shot warning flag mutation");
    if(sound.resources[resource]->voices[0]){
        BHSoundVoice *voice=sound.resources[resource]->voices[0];
        handle=voice->index+((unsigned long)voice->generation<<16);
        CHECK(bh_sound_stop(&sound,handle)==1,"queued voice stopped after message failure");
    }
    weapon=factory.registry.slots[9].entity;state=bh_weapon_object_state(weapon);
    state->common.ammo=2;*(float*)((unsigned char*)&player+0x240)=1.0f;
    CHECK(bh_weapon_index9_trigger(&context,weapon,&player) &&
        state->common.ammo==2 && context.requests==3,
        "direct source owner call links complete index9 positive-timer branch");
    CHECK(bh_weapon_empty_dispatch(&context,&factory.registry,9,&player) &&
        state->common.ammo==2 && context.requests==3 &&
        raw_float((unsigned char*)&player+0x240)==0x3f800000UL,
        "index9 positive player timer suppresses ammo and sound effects");
    *(float*)((unsigned char*)&player+0x240)=0.0f;
    CHECK(bh_weapon_empty_dispatch(&context,&factory.registry,9,&player) &&
        state->common.ammo==1 &&
        raw_float((unsigned char*)&player+0x240)==0x41700000UL &&
        raw_float((unsigned char*)&player+0x244)==0x3f800000UL &&
        raw_float((unsigned char*)&player+0x248)==0x41700000UL &&
        context.requests==3,
        "index9 active ammo decrements and installs exact player +240/244/248 timer effects");
    *(float*)((unsigned char*)&player+0x240)=0.0f;state->common.ammo=0;
    CHECK(bh_weapon_empty_dispatch(&context,&factory.registry,9,&player) &&
        state->common.ammo==0 && context.requests==4 &&
        context.warnings==2 && warning_seen==1,
        "index9 empty ammo uses real queued sound and shared once-only warning path");
    if(sound.resources[resource]->voices[0]){
        BHSoundVoice *voice=sound.resources[resource]->voices[0];
        handle=voice->index+((unsigned long)voice->generation<<16);
        CHECK(bh_sound_stop(&sound,handle)==1,"index9 queued sound stopped");
    }
    state=bh_weapon_object_state(factory.registry.slots[3].entity);
    state->common.word_e8=0x12345678UL;
    CHECK(!bh_weapon_empty_dispatch(&context,&factory.registry,3,&player) &&
        context.error==BH_WEAPON_EMPTY_METHOD &&
        state->common.word_e8==0x12345678UL,
        "unrecovered +124 method rejects rather than using a no-op");
    weapon=factory.registry.slots[2].entity;
    CHECK(bh_entity_release(&pool,weapon),"index2 allocation released");
    CHECK(!bh_weapon_empty_dispatch(&context,&factory.registry,2,&player) &&
        context.error==BH_WEAPON_EMPTY_STALE,
        "released generation cannot invoke gameplay method");
    for(i=0;i<5;i++)if(i!=2)CHECK(bh_entity_release(&pool,factory.registry.slots[i].entity),
        "real weapon allocations returned");
    CHECK(bh_entity_release(&pool,factory.registry.slots[9].entity),
        "index9 weapon allocation returned");
    CHECK(bh_weapon_factory_close(&factory),"factory binding closes after release");
done:
    bh_sound_queue_dispose(&sound);if(device)IDirectSound_Release(device);
    CHECK(bh_entities_dispose(&pool),"real pool disposed");
    CHECK(bh_math_environment_supported(),"source path leaves PC53 profile intact");
    bh_math_environment_leave(&fp);
    if(count)*count=checks;
    printf("{\"scenario\":\"message-weapon\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_MESSAGE_WEAPON_STANDALONE
int main(void){unsigned long count;return bh_message_weapon_checkpoint(&count);}
#endif
