#include "../src/bh_sound_startup.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL sound-startup %d: %s\n",__LINE__,s);}}while(0)
int bh_sound_startup_checkpoint(unsigned long *count){
    BHSoundQueue queue,saved;BHSoundStartupStats stats,unchanged;DSCAPS device_caps;DSBCAPS primary_caps;
    LPDIRECTSOUND retained_device;LPDIRECTSOUNDBUFFER retained_primary;DWORD status=0;unsigned long iteration;int result;
    checks=0;failures=0;
    memset(&queue,0,sizeof(queue));memset(&stats,0x5a,sizeof(stats));unchanged=stats;
    CHECK(!bh_sound_startup(NULL,GetDesktopWindow(),&stats)&&!memcmp(&stats,&unchanged,sizeof(stats)),"NULL source guard makes no API/stat mutation");
    CHECK(!bh_sound_startup(&queue,GetDesktopWindow(),&stats)&&!memcmp(&stats,&unchanged,sizeof(stats)),"uninitialized owner guard");
    for(iteration=0;iteration<2;++iteration){
        CHECK(bh_sound_queue_init(&queue,NULL,NULL,NULL),"real fresh queue owner");
        queue.listener_active=-7;queue.listener.x=3;queue.listener.y=-4;queue.listener.z=5;
        result=bh_sound_startup(&queue,GetDesktopWindow(),&stats);
        CHECK(result==1&&queue.device&&stats.create_result==DS_OK,"actual default native device creation");
        CHECK(stats.create_calls>=1&&stats.create_calls<=10&&stats.sleep_calls==stats.create_calls-1,"bounded actual creation/retry counts");
        CHECK(stats.cooperative_calls==1&&stats.cooperative_result==DS_OK&&stats.primary_calls==1,"real normal cooperative level and primary request");
        CHECK(!queue.listener_active&&queue.listener.x==3&&queue.listener.y==-4&&queue.listener.z==5,"only original listener-active word cleared");
        CHECK(!queue.resource_count&&!queue.voice_count&&!queue.resources&&!queue.voices,"startup does not load or fabricate samples/voices");
        if(!queue.device){bh_sound_queue_dispose(&queue);continue;}
        memset(&device_caps,0,sizeof(device_caps));device_caps.dwSize=sizeof(device_caps);
        CHECK(IDirectSound_GetCaps(queue.device,&device_caps)==DS_OK,"real initialized device capabilities");
        CHECK(queue.primary&&stats.primary_result==DS_OK&&stats.play_calls==1&&stats.play_result==DS_OK,"actual original primary looping Play");
        if(queue.primary){
            memset(&primary_caps,0,sizeof(primary_caps));primary_caps.dwSize=sizeof(primary_caps);
            CHECK(IDirectSoundBuffer_GetCaps(queue.primary,&primary_caps)==DS_OK&&(primary_caps.dwFlags&DSBCAPS_PRIMARYBUFFER),"real primary descriptor consumer");
            CHECK(IDirectSoundBuffer_GetStatus(queue.primary,&status)==DS_OK&&(status&DSBSTATUS_PLAYING)&&(status&DSBSTATUS_LOOPING),"native primary actually looping");
            CHECK(IDirectSoundBuffer_Stop(queue.primary)==DS_OK,"fixture bounds primary loop immediately without samples");
        }
        saved=queue;memset(&stats,0x5a,sizeof(stats));unchanged=stats;
        CHECK(!bh_sound_startup(&queue,GetDesktopWindow(),&stats)&&!memcmp(&queue,&saved,sizeof(queue))&&!memcmp(&stats,&unchanged,sizeof(stats)),"occupied owner guard preserves real interfaces");
        retained_device=queue.device;retained_primary=queue.primary;IDirectSound_AddRef(retained_device);if(retained_primary)IDirectSoundBuffer_AddRef(retained_primary);
        CHECK(bh_sound_queue_dispose(&queue)&&!queue.device&&!queue.primary&&!queue.allocator.allocate,"real existing owning disposal clears queue");
        if(retained_primary)CHECK(IDirectSoundBuffer_Release(retained_primary)==0,"primary source ownership releases final held reference");
        CHECK(IDirectSound_Release(retained_device)==0,"device source ownership releases final held reference");
    }
    *count=checks;printf("{\"scenario\":\"sound-startup\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures;
}
