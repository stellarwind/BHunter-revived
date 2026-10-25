#include "../src/bh_input_devices.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<15)printf("FAIL input devices %d: %s\n",__LINE__,s);}}while(0)
typedef struct Fixture {
    BHInputDevices devices;BHInputCollection collection;BHInputSnapshot snapshot;BHPlayerInputOwner lists;
    unsigned long maps[255],map_count,captures,caps_calls,releases,recenters,sequence;
    unsigned long allocations,frees,live,fail_at,denied;
    MMRESULT capture_result[2],caps_result[2],release_result[2];unsigned long caps[2];
    HWND window;
} Fixture;
static Fixture *active;
static UINT WINAPI map_key(UINT key,UINT type){CHECK(key<255 && type==2,"exact key0..254 mapping loop and type2");++active->map_count;return active->maps[key];}
static MMRESULT WINAPI capture(HWND window,UINT device,UINT period,BOOL changed){CHECK(window==active->window && device<2 && period==100 && !changed,"original HWND/device/100ms/continuous capture arguments");++active->captures;active->sequence=active->sequence*4+device+1;return active->capture_result[device];}
static MMRESULT WINAPI capabilities(UINT device,LPJOYCAPSA caps,UINT bytes){CHECK(device<2 && bytes==0x194,"real JOYCAPS size and device");++active->caps_calls;if(!active->caps_result[device])caps->wCaps=active->caps[device];return active->caps_result[device];}
static MMRESULT WINAPI release_capture(UINT device){CHECK(device<2,"valid release device");++active->releases;active->sequence=active->sequence*4+device+1;return active->release_result[device];}
static BOOL WINAPI recenter(int x,int y){CHECK(x==160 && y==100,"initializer passes original recenter coordinates only to private API fixture");++active->recenters;active->sequence=active->sequence*4+3;return 1;}
static void *__cdecl allocate(unsigned long bytes,void *user){Fixture *f=(Fixture *)user;void *p;++f->allocations;CHECK(bytes==8 || bytes==10,"original8/10-byte formatted-name allocation sizes");if(f->allocations==f->fail_at){++f->denied;return NULL;}p=malloc(bytes);if(p)++f->live;return p;}
static void __cdecl free_name(void *memory,void *user){Fixture *f=(Fixture *)user;if(memory){++f->frees;--f->live;free(memory);}}
static void setup(Fixture *f){BHSceneAllocator allocator;unsigned long i;memset(f,0,sizeof(*f));CHECK(bh_player_input_owner_init(&f->lists,NULL) && bh_input_collection_init(&f->collection,&f->snapshot,&f->lists),"real collection/list owner bindings");
    allocator.allocate=allocate;allocator.release=free_name;allocator.user=f;CHECK(bh_input_devices_bind(&f->devices,&f->collection,&allocator),"real device/key ownership adapter");
    CHECK(f->devices.api.map_key==MapVirtualKeyA && f->devices.api.capture==joySetCapture && f->devices.api.capabilities==joyGetDevCapsA && f->devices.api.release==joyReleaseCapture,"shipping API bindings resolve actual user32/winmm definitions");
    f->devices.api.map_key=map_key;f->devices.api.capture=capture;f->devices.api.capabilities=capabilities;f->devices.api.release=release_capture;f->collection.api.recenter=recenter;
    for(i=0;i<255;++i)f->maps[i]=i;f->caps[0]=3;f->caps[1]=2;active=f;
}
static void finish(Fixture *f){bh_input_devices_dispose(&f->devices);CHECK(!f->live && f->allocations-f->frees==f->denied,"only generated strings freed; allocation failure and static borrowed entries accounted");bh_player_input_owner_dispose(&f->lists);active=NULL;}
static void startup(void){Fixture f;unsigned long i;setup(&f);
    CHECK(!strcmp(f.devices.names[0],"<No key>") && !strcmp(f.devices.names[1],"Mouse Button L") && !strcmp(f.devices.names[2],"Mouse Button R"),"original static data anchors");
    f.maps['A']='Q';f.maps['B']=0x80000057UL;f.maps[7]=0;f.maps[21]='z';
    memset(&f.snapshot,0x5a,sizeof(f.snapshot));f.collection.enabled[0]=f.collection.enabled[1]=1;f.collection.deadzone_mask=0;
    f.window=(HWND)0x1234;CHECK(bh_input_devices_start(&f.devices,f.window),"complete device initialization with isolated API samples");
    CHECK(f.map_count==255 && f.captures==2 && f.caps_calls==2 && f.recenters==1 && f.sequence==27,"complete original API order and exact name-table boundary");
    CHECK(!strcmp(f.devices.names['A'],"Key 'Q'") && !strcmp(f.devices.names['B'],"Key 'W'"),"localized uppercase overrides static defaults and ignores high bits");
    CHECK(!strcmp(f.devices.names[7],"Key #7") && !strcmp(f.devices.names[21],"Key 'z'"),"missing names choose VK numeric versus mapped character");
    CHECK(f.collection.enabled[0] && f.collection.enabled[1] && f.collection.capabilities[0]==3 && f.collection.capabilities[1]==2,"capture success enables joystick and binds real caps flags");
    CHECK(!f.snapshot.history_head && f.snapshot.pair3[0]==512 && f.snapshot.pair3[1]==512 && !f.snapshot.pair4[0] && !f.snapshot.pair4[1] && !f.collection.delta[0] && !f.collection.delta[1],"history and mouse pair/delta defaults");
    CHECK(!f.collection.calibrated[0] && !f.collection.calibrated[1] && f.collection.deadzone_mask==0xffffffffUL && !f.snapshot.mouse_axes[0] && !f.snapshot.mouse_axes[1],"calibration/deadzone/filter defaults");
    for(i=0;i<255;++i)CHECK(!f.snapshot.current[i] && !f.snapshot.previous[i],"only255 key bytes reset in each exact region");
    for(i=0;i<32;++i)CHECK(!f.snapshot.history[i],"all32 history bytes reset");
    for(i=0;i<8;++i)CHECK(!f.snapshot.axes[i] && f.collection.minimum[i]==1 && f.collection.maximum[i]==-1,"eight axes/minmax defaults");
    f.release_result[0]=JOYERR_UNPLUGGED;f.sequence=0;CHECK(!bh_input_devices_stop(&f.devices) && f.releases==2 && f.sequence==6 && !f.collection.enabled[0] && !f.collection.enabled[1],"stop still releases both and clears flags after first API error");
    finish(&f);
}
static void failures_and_reuse(void){Fixture f;unsigned long allocations;setup(&f);f.capture_result[0]=JOYERR_UNPLUGGED;f.caps_result[1]=MMSYSERR_ERROR;f.devices.caps[1].wCaps=7;
    f.fail_at=1;CHECK(!bh_input_devices_start(&f.devices,NULL) && f.devices.allocation_failures==1,"allocation failure diagnosed but initialization continues");
    CHECK(!f.collection.enabled[0] && f.collection.enabled[1] && f.caps_calls==1 && f.collection.capabilities[1]==7,"failed capture skips caps; failed caps retains initialized storage and capture still enables");
    CHECK(bh_input_devices_stop(&f.devices) && f.releases==1,"shutdown releases only enabled device");
    f.fail_at=0;allocations=f.allocations;f.maps['A']='Z';CHECK(bh_input_devices_start(&f.devices,NULL),"repeated startup replaces localized strings without leaks");
    CHECK(!strcmp(f.devices.names['A'],"Key 'Z'") && f.allocations>allocations,"replacement and retry of prior failed slot");
    CHECK(bh_input_devices_stop(&f.devices),"repeat stop");finish(&f);
}
static void live_owner_guard(void){Fixture f;BHPlayerInputList *list;setup(&f);list=bh_player_input_create(&f.lists,1);CHECK(list!=NULL,"real input list for source lifetime guard");
    CHECK(!bh_input_devices_start(&f.devices,NULL) && !f.map_count && !f.captures && f.lists.first==list,"source guard preserves live input lists rather than dropping their owner");
    CHECK(MapVirtualKeyA('A',2)!=0,"actual read-only native key translation executes");finish(&f);
}
int bh_input_devices_checkpoint(unsigned long *count){checks=0;failures=0;startup();failures_and_reuse();live_owner_guard();if(count)*count=checks;
    printf("{\"scenario\":\"input-devices\",\"checks\":%lu,\"failures\":%d,\"native_capture_calls\":0,\"native_recenter_calls\":0,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
