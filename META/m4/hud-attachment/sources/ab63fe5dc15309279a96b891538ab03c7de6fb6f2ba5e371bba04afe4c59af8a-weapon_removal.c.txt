#include "../src/bh_weapon_removal.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-removal: %s\n",s);}}while(0)
int bh_weapon_removal_checkpoint(unsigned long *count){
 BHEntity weapon,before,target,other;BHWeaponCommonState *s;BHWeaponRemovedPacket packet;
 BHSoundQueue sound;BHSoundResource *r;BHSoundVoice *voice;LPDIRECTSOUND device=NULL;BHWave wave;unsigned char samples[64];
 BHMathEnvironment fp;unsigned long id,handle;int i;
 checks=failures=0;memset(&sound,0,sizeof(sound));memset(&wave,0,sizeof(wave));
 if(!bh_math_environment_enter(&fp))return 2;
 memset(&weapon,0xa5,sizeof(weapon));s=bh_weapon_common_state(&weapon);s->hud.target=&target;s->word_100=-1;before=weapon;
 for(i=0;i<20;++i)if(i!=17){packet.event=i;packet.entity=&target;CHECK(bh_weapon_target_removed(NULL,&weapon,&packet)&&!memcmp(&weapon,&before,sizeof(weapon)),"other event packets leave all fields intact without requiring sound binding");}
 packet.event=17;packet.entity=&other;CHECK(bh_weapon_target_removed(NULL,&weapon,&packet)&&!memcmp(&weapon,&before,sizeof(weapon)),"different removed target preserves handle/charge/cache and target");
 packet.entity=&target;CHECK(bh_weapon_target_removed(NULL,&weapon,&packet)&&!s->hud.target&&s->word_100==-1&&s->hud.charge==(long)0xa5a5a5a5UL,"matching target clears even without a sound handle; charge survives");
 CHECK(DirectSoundCreate(NULL,&device,NULL)==DS_OK&&device,"actual DirectSound interface without playback");if(!device)goto done;
 CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK&&bh_sound_queue_init(&sound,device,NULL,NULL),"real queued sound owner");
 memset(samples,0,sizeof(samples));wave.format_tag=1;wave.channels=1;wave.sample_rate=8000;wave.byte_rate=16000;wave.block_align=2;wave.bits=16;wave.sample_bytes=64;wave.samples=samples;
 id=bh_sound_register_wave(&sound,&wave,0,1,0);CHECK(id!=BH_SOUND_INVALID,"real PCM resource upload");if(id==BH_SOUND_INVALID)goto done;
 r=sound.resources[id];handle=bh_sound_request(&sound,id,1,1,NULL,NULL);voice=r->voices[0];CHECK(handle!=BH_SOUND_INVALID&&voice&&voice->pending,"actual queued voice pending before removal");
 s->hud.target=&target;s->word_100=(long)handle;packet.entity=&target;
 CHECK(bh_weapon_target_removed(&sound,&weapon,&packet)&&!s->hud.target&&s->word_100==-1&&!r->voices[0],"matching event clears target before real stop and resets handle afterward");
 s->hud.target=NULL;s->word_100=(long)handle;packet.entity=NULL;CHECK(bh_weapon_target_removed(&sound,&weapon,&packet)&&s->word_100==-1,"NULL matches NULL and stale actual stop result is discarded");
 s->hud.target=&target;s->word_100=0;packet.entity=&target;CHECK(!bh_weapon_target_removed(NULL,&weapon,&packet)&&!s->hud.target&&s->word_100==-1,"unbound required sound diagnosed after preserved target/handle effects");
 CHECK(!bh_weapon_target_removed(&sound,NULL,&packet)&&!bh_weapon_target_removed(&sound,&weapon,NULL),"source absent-owner guards");
done:
 bh_sound_queue_dispose(&sound);if(device)IDirectSound_Release(device);CHECK(bh_math_environment_supported(),"PC53 caller profile preserved");bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"weapon-removal\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;
}
