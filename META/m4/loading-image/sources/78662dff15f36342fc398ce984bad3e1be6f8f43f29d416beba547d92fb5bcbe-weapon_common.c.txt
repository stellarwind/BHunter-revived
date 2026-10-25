#include "../src/bh_weapon_common.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-common: %s\n",s);}}while(0)
static unsigned long bits(float value){unsigned long word;memcpy(&word,&value,4);return word;}
typedef struct AllocationObservation {BHEntity *player;int armed,mutations;} AllocationObservation;
/* Isolated allocator observation delegates real malloc/free. It exercises
   synchronous mutation before the real sound owner reads borrowed vectors. */
static void *__cdecl allocate(unsigned long bytes,void *raw){
    AllocationObservation *o=(AllocationObservation *)raw;
    if(o->armed){o->armed=0;++o->mutations;o->player->position.x=31;o->player->origin.x=-61;}
    return malloc(bytes);
}
static void __cdecl release(void *p,void *raw){free(p);}
int bh_weapon_common_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;BHEntity weapon,before,expected,player,target;
    BHWeaponCommonState *s,*e;BHWeaponReadyContext context;
    BHSoundQueue sound;BHSoundResource *resource;BHSoundVoice *voice;
    LPDIRECTSOUND device=NULL;BHWave wave;unsigned char samples[64];
    unsigned long id,i,seed,first,second,handle;float pitch,frequency;
    AllocationObservation allocation;BHSceneAllocator allocator;
    static const long delays[]={0,1,2,-1,-2,LONG_MAX,LONG_MIN};
    checks=failures=0;memset(&sound,0,sizeof(sound));memset(&wave,0,sizeof(wave));
    if(!bh_math_environment_enter(&fp))return 2;
    memset(&allocation,0,sizeof(allocation));allocator.allocate=allocate;allocator.release=release;allocator.user=&allocation;
    memset(&weapon,0xa5,sizeof(weapon));s=bh_weapon_common_state(&weapon);
    CHECK((unsigned char *)s==(unsigned char *)&weapon+0xe8 &&
        (unsigned char *)&s->hud==(unsigned char *)&weapon+0xf8 &&
        (unsigned char *)&s->ammo==(unsigned char *)&weapon+0x10c &&
        (unsigned char *)&s->delay==(unsigned char *)&weapon+0x114,
        "actual entity-owned common state shares HUD/charge/ammo fields at verified offsets");
    CHECK(!bh_weapon_common_state(NULL) && !bh_weapon_delay_tick(NULL),"source accessor rejects absent allocation");
    for(i=0;i<sizeof(delays)/sizeof(delays[0]);++i){
        s->delay=delays[i];before=weapon;expected=before;e=bh_weapon_common_state(&expected);
        e->delay=delays[i]?(long)((unsigned long)delays[i]-1UL):0;
        CHECK(bh_weapon_delay_tick(&weapon) && !memcmp(&weapon,&expected,sizeof(weapon)),
            "complete nonzero countdown owner preserves all other allocation bytes and signed wrap");
    }
    CHECK(DirectSoundCreate(NULL,&device,NULL)==DS_OK && device,"actual DirectSound owner, no playback");
    if(!device)goto done;
    CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK &&
        bh_sound_queue_init(&sound,device,NULL,&allocator),"real sound queue takes native interface ownership");
    memset(samples,0,sizeof(samples));wave.format_tag=1;wave.channels=1;wave.sample_rate=8000;
    wave.byte_rate=16000;wave.block_align=2;wave.bits=16;wave.sample_bytes=64;wave.samples=samples;
    id=bh_sound_register_wave(&sound,&wave,0,1,0);
    CHECK(id!=BH_SOUND_INVALID,"real PCM resource uploaded to actual native buffer without Play");
    if(id==BH_SOUND_INVALID)goto done;
    resource=sound.resources[id];memset(&player,0,sizeof(player));memset(&target,0,sizeof(target));
    player.position.x=3;player.position.y=-4;player.position.z=5;
    player.origin.x=-6;player.origin.y=7;player.origin.z=8;
    memset(&context,0,sizeof(context));context.sound=&sound;context.sound_resource=id;
    allocation.player=&player;allocation.armed=1;
    for(i=0;i<16;++i){
        seed=1+i*997;first=(seed*214013UL+2531011UL);second=first*214013UL+2531011UL;
        first=(first>>16)&32767UL;second=(second>>16)&32767UL;
        memset(&weapon,0xa5,sizeof(weapon));s=bh_weapon_common_state(&weapon);s->hud.target=&target;
        expected=weapon;e=bh_weapon_common_state(&expected);
        e->word_e8=e->word_f4=e->word_f0=0;e->hud.target=NULL;e->hud.charge=0;e->word_100=-1;
        srand(seed);CHECK(bh_weapon_ready_reset(&context,&weapon,&player),"complete reset and real native-backed queued sound request");
        CHECK(!memcmp(&weapon,&expected,sizeof(weapon)) && s->available_ec==0xa5a5a5a5UL &&
            (unsigned long)s->ammo==0xa5a5a5a5UL && (unsigned long)s->delay==0xa5a5a5a5UL,
            "only original reset fields mutate; availability/ammo/delay and method region survive");
        CHECK(context.random_calls==1 && (unsigned long)rand()==second,"exact one shared CRT random call including source sequence");
        voice=resource->voices[0];
        CHECK(voice && voice->pending==1 && voice->positioned && voice->moving &&
            !memcmp(&voice->position,&player.position,12) && !memcmp(&voice->velocity,&player.origin,12),
            "actual sound voice receives player position0c and velocity48, no guessed origin field");
        if(!i)CHECK(allocation.mutations==1 && voice && voice->position.x==31 && voice->velocity.x==-61,
            "original live player vector pointers survive real sound allocation callback mutation");
        bh_round_float(&pitch,.9+(double)first/163840.0);
        bh_round_float(&frequency,8000.0*pitch);
        CHECK(voice && bits(voice->frequency)==bits(frequency) && voice->volume<0 && voice->volume>-200,
            "negative raw pitch multiplier gives independently computed .9..1.1 frequency and real .9 level mapping");
        if(voice){handle=voice->index+((unsigned long)voice->generation<<16);
            CHECK(bh_sound_stop(&sound,handle)==1,"real queued voice lifetime released without any playback");}
    }
    context.sound_resource=BH_SOUND_INVALID;memset(&weapon,0xa5,sizeof(weapon));s=bh_weapon_common_state(&weapon);
    srand(9);first=(unsigned long)rand();second=(unsigned long)rand();srand(9);
    CHECK(!bh_weapon_ready_reset(&context,&weapon,&player) && context.error==2 && s->word_e8==0 &&
        !s->hud.target && s->hud.charge==0 && s->word_100==-1 && (unsigned long)rand()==second,
        "missing sound resource reports source error after original reset and random side effects");
done:
    bh_sound_queue_dispose(&sound);if(device)IDirectSound_Release(device);
    CHECK(bh_math_environment_supported(),"shared callback preserves caller PC53 profile");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"weapon-common\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_WEAPON_COMMON_STANDALONE
int main(void){unsigned long count;return bh_weapon_common_checkpoint(&count);}
#endif
