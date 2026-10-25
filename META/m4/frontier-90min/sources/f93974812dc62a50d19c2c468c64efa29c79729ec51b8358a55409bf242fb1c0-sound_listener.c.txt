#include "../src/bh_sound_queue.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL listener %d: %s\n",__LINE__,label);}} while(0)
static void listener_cases(void) {
    BHSoundQueue q;BHSoundResource *r;BHVec3 position={10,20,30},angles={0,0,0},velocity={1,-2,3};
    BHVec3 saved_position,saved_velocity,saved_right,point={13,24,30};
    double cy,sy,cx,sx,cz,sz;float priority;unsigned long i,nan=0x7fc00000UL;
    int expected_random;long saved_flag;
    CHECK(bh_sound_queue_init(&q,NULL,NULL,NULL),"real pool-only sound context");
    CHECK(q.voice_limit_policy==2 && !q.listener_active,"verified initial global policy and inactive listener");
    r=bh_sound_resource_allocate(&q);CHECK(r!=NULL,"real owned sound resource");
    srand(71);expected_random=rand();srand(71);
    CHECK(bh_sound_listener(&q,0,-7,&position,&angles,&velocity) && q.listener_active==-7 &&
        !memcmp(&q.listener,&position,12) && !memcmp(&q.listener_velocity,&velocity,12) &&
        q.listener_right.x==1 && q.listener_right.y==0 && q.listener_right.z==0,"exact nonzero signed enable and identity listener basis");
    CHECK(rand()==expected_random,"listener update consumes no shared RNG draws");
    CHECK(bh_sound_priority(&q,1,&point,r,-1,&priority) && priority==25,"real priority consumer uses restored position and signed active flag");
    angles.x=1.5707963267948966f;
    CHECK(bh_sound_listener(&q,0,1,&position,&angles,&velocity) && fabs(q.listener_right.x)<1e-6 && q.listener_right.y==0 && fabs(q.listener_right.z+1)<1e-6,"yaw rotates listener right toward negative Z");
    angles.x=0;angles.y=1.5707963267948966f;
    CHECK(bh_sound_listener(&q,0,1,&position,&angles,&velocity) && q.listener_right.x==1 && q.listener_right.y==0 && q.listener_right.z==0,"pitch leaves right axis unchanged");
    angles.y=0;angles.z=1.5707963267948966f;
    CHECK(bh_sound_listener(&q,0,1,&position,&angles,&velocity) && fabs(q.listener_right.x)<1e-6 && fabs(q.listener_right.y-1)<1e-6 && q.listener_right.z==0,"roll rotates right axis toward positive Y");
    for(i=0;i<17;++i) {
        angles.x=(float)i*.071f;angles.y=-(float)i*.037f;angles.z=(float)i*.053f;
        cy=cos((double)angles.x);sy=sin((double)angles.x);cx=cos((double)angles.y);sx=sin((double)angles.y);cz=cos((double)angles.z);sz=sin((double)angles.z);
        CHECK(bh_sound_listener(&q,0,1,&position,&angles,&velocity) &&
            fabs(q.listener_right.x-(cy*cz+sy*sx*sz))<3e-7 &&
            fabs(q.listener_right.y-cx*sz)<3e-7 &&
            fabs(q.listener_right.z-(-sy*cz+cy*sx*sz))<3e-7,"independent mixed-angle analytic Y-X-Z composition");
    }
    saved_position=q.listener;saved_velocity=q.listener_velocity;saved_right=q.listener_right;
    CHECK(bh_sound_listener(&q,0,0,NULL,NULL,NULL) && !q.listener_active &&
        !memcmp(&saved_position,&q.listener,12) && !memcmp(&saved_velocity,&q.listener_velocity,12) && !memcmp(&saved_right,&q.listener_right,12),"disabled listener retains position, velocity and basis");
    CHECK(bh_sound_priority(&q,1,&point,r,-1,&priority) && priority==9e37f,"real priority consumer observes disabled listener");
    saved_flag=q.listener_active;
    CHECK(!bh_sound_listener(&q,1,1,&position,&angles,&velocity) && q.listener_active==saved_flag && !memcmp(&saved_right,&q.listener_right,12),"original one-listener range enforced without output mutation");
    memcpy(&angles.x,&nan,4);
    CHECK(!bh_sound_listener(&q,0,1,&position,&angles,&velocity) && q.listener_active==saved_flag && !memcmp(&saved_position,&q.listener,12),"nonfinite enabled request rejects atomically");
    CHECK(!bh_sound_listener(&q,0,1,NULL,NULL,NULL) && !bh_sound_set_voice_limit(NULL,1),"invalid source bindings reject");
    CHECK(bh_sound_set_voice_limit(&q,0) && q.voice_limit_policy==2,"zero request selects original policy two");
    for(i=1;i<=3;++i)CHECK(bh_sound_set_voice_limit(&q,i) && q.voice_limit_policy==i,"original accepted limit values");
    CHECK(bh_sound_set_voice_limit(&q,-1) && q.voice_limit_policy==3,"negative request selects three");
    CHECK(bh_sound_set_voice_limit(&q,4) && q.voice_limit_policy==3,"request four selects three");
    CHECK(bh_sound_set_voice_limit(&q,(long)0x80000000UL) && q.voice_limit_policy==3,"signed lower extreme selects three");
    CHECK(bh_sound_set_voice_limit(&q,0x7fffffffL) && q.voice_limit_policy==3,"signed upper extreme selects three");
    CHECK(bh_sound_queue_dispose(&q),"source context disposal");
}
static unsigned long wave_fixture(unsigned char *bytes) {
    unsigned long riff=100,format_size=16,sample_rate=8000,byte_rate=16000,samples=64;
    unsigned short pcm=1,mono=1,align=2,bits=16;
    memset(bytes,0,108);memcpy(bytes,"RIFF",4);memcpy(bytes+4,&riff,4);memcpy(bytes+8,"WAVEfmt ",8);
    memcpy(bytes+16,&format_size,4);memcpy(bytes+20,&pcm,2);memcpy(bytes+22,&mono,2);
    memcpy(bytes+24,&sample_rate,4);memcpy(bytes+28,&byte_rate,4);memcpy(bytes+32,&align,2);memcpy(bytes+34,&bits,2);
    memcpy(bytes+36,"data",4);memcpy(bytes+40,&samples,4);return 108;
}
static void policy_consumer(void) {
    BHSoundQueue q;BHWave wave;unsigned char bytes[108];LPDIRECTSOUND device=NULL;
    HRESULT hr;unsigned long a,b,c;
    hr=DirectSoundCreate(NULL,&device,NULL);CHECK(hr==DS_OK && device,"real native policy consumer backend");
    if(hr!=DS_OK || !device)return;
    CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK,"native normal cooperative level");
    CHECK(bh_sound_queue_init(&q,device,NULL,NULL) && bh_wave_init(&wave,NULL) && bh_wave_decode(&wave,bytes,wave_fixture(bytes)),"real decoded PCM and owned native queue");
    a=bh_sound_register_wave(&q,&wave,0,0,0);
    CHECK(a!=BH_SOUND_INVALID && q.resources[a]->voice_limit==2 && !q.resources[a]->no_more,"default two-voice policy captured by source native registration");
    CHECK(bh_sound_set_voice_limit(&q,1),"configure one voice");b=bh_sound_register_wave(&q,&wave,0,0,0);
    CHECK(b!=BH_SOUND_INVALID && q.resources[b]->voice_limit==1 && q.resources[b]->no_more,"one-voice policy disables native duplication");
    CHECK(bh_sound_set_voice_limit(&q,-9),"configure fallback three voices");c=bh_sound_register_wave(&q,&wave,0,0,0);
    CHECK(c!=BH_SOUND_INVALID && q.resources[c]->voice_limit==3 && !q.resources[c]->no_more && q.resources[a]->voice_limit==2 && q.resources[b]->voice_limit==1,"new policy captured without rewriting prior resources");
    CHECK(bh_sound_queue_dispose(&q),"actual uploaded buffers released without playback");bh_wave_dispose(&wave);
    CHECK(IDirectSound_Release(device)==0,"last native device reference released");
}
int bh_sound_listener_checkpoint(unsigned long *count) {
    checks=0;failed=0;listener_cases();policy_consumer();*count=checks;
    printf("{\"scenario\":\"sound-listener\",\"checks\":%lu,\"failures\":%d,\"play_calls\":0,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
