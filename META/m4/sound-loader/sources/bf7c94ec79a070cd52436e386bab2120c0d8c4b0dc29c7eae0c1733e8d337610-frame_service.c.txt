#include "../src/bh_frame_service.h"
#include "../src/bh_input_devices.h"
#include "../src/bh_input_messages.h"
#include "../src/bh_cd_close.h"
#include "../src/bh_network_shutdown.h"
#include "../src/bh_network_transport.h"
#include "../src/bh_force_feedback.h"
#include "../src/bh_display_shutdown.h"
#include "../src/bh_fp.h"
#include "../src/bh_contact_effects.h"
#include "../src/bh_frame_worker.h"
#include "../src/bh_scene_view.h"
#include <stdio.h>
#include <string.h>
#ifndef HWND_MESSAGE
#define HWND_MESSAGE ((HWND)-3)
#endif
static unsigned long checks,failures,cursor_calls,recenter_calls;
static BHFrameService *observed_service;
static BHPlayerInputState *observed_destination;
static const void *observed_packet;
static unsigned long observer_calls;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL frame-service: %s\n",s);}}while(0)
/* Isolated test instrumentation only: force a legal live-binding change at
   entity refresh so an incorrectly cached early packet/player pointer fails.
   This observer is never installed by a production constructor or binding. */
static void __cdecl observe_late_bindings(BHEntity *entity,void *packet){
    CHECK(packet==NULL,"real refresh dispatch passes its original NULL packet");
    ++observer_calls;observed_service->packet=observed_packet;
    observed_service->packet_bytes=8;observed_service->player_input=observed_destination;
}
/* These are isolated input API fixtures, never installed in a game target. */
static MMRESULT WINAPI joystick(UINT i,LPJOYINFOEX sample){return JOYERR_UNPLUGGED;}
static BOOL WINAPI cursor(LPPOINT p){++cursor_calls;p->x=160;p->y=100;return TRUE;}
static BOOL WINAPI recenter(int x,int y){++recenter_calls;return x==160&&y==100;}
static void __cdecl produce(void *user){bh_frame_service_tick((BHFrameService*)user);}
static LRESULT __cdecl message(void *user,HWND window,UINT code,WPARAM key,LPARAM bits){
    return bh_input_message((BHInputSnapshot*)user,window,code,key,bits);
}
static void consume_published(BHFrameBuffers *frames,BHDrawQueue *slots){
    BHSceneView scene;BHTextureContext textures;BHRenderState state;BHFadeContext fade;
    BHPresentContext present;BHPresentBegin begin;BHFrameDisplay display;
    BHRenderExecute render;BHTextQueue text;BHFrameCallback callback;BHFrameWorker worker;
    unsigned long i;
    memset(&scene,0,sizeof(scene));memset(&textures,0,sizeof(textures));memset(&fade,0,sizeof(fade));
    memset(&present,0,sizeof(present));memset(&begin,0,sizeof(begin));memset(&display,0,sizeof(display));
    memset(&render,0,sizeof(render));memset(&worker,0,sizeof(worker));callback.render=NULL;
    CHECK(bh_scene_view_init(&scene,640,480),"actual private640x480 source renderer for published-frame consumption");
    if(!scene.device){bh_scene_view_dispose(&scene);return;}
    textures.device=scene.device;
    CHECK(bh_render_state_init(&state,&textures),"actual native state owner bound for nine published bucket headers");
    fade.render=&state;fade.width=640;fade.height=480;fade.near_plane=scene.projection.near_z;
    present.fade=&fade;present.color=scene.color;present.auxiliary=scene.depth;begin.present=&present;
    display.textures=&textures;display.color=scene.color;display.auxiliary=scene.depth;
    bh_text_queue_init(&text);render.display=&display;render.text=&text;render.state=&state;
    worker.frames=frames;for(i=0;i<3;++i)worker.queues[i]=slots+i;
    worker.render=&render;worker.callback=&callback;worker.begin=&begin;
    worker.viewport=&scene.projection;worker.width=640;worker.height=480;
    worker.primary=scene.color;worker.back=scene.depth;worker.ready=1;worker.display_state=3;worker.stop=1;
    CHECK(bh_frame_callback_start(&callback),"real renderer registration wrapper before worker entry");
    CHECK(!bh_frame_worker_run(&worker)&&worker.done&&worker.iterations==1&&worker.selected==frames->latest&&worker.rendered==1,
          "complete source producer publication crosses registered whole worker exactly once");
    CHECK(render.draw_calls==9&&!render.failures&&!present.failures&&!begin.failures,
          "actual registered consumer drains all nine original bucket headers through real native source owners");
    CHECK(worker.error==BH_FRAME_WORKER_NATIVE&&worker.flips==1&&worker.last_hresult==DDERR_NOTFLIPPABLE&&
          worker.failures==1&&!frames->slots[worker.selected].busy,
          "only actual offscreenFlip failure remains after rendering, buffer release and natural worker acknowledgement");
    CHECK(!render.draw&&!frames->writing&&slots[worker.selected].commands==frames->slots[worker.selected].data,
          "borrowed selected-frame metadata and canonical producer storage retain real ownership after drain");
    bh_scene_view_dispose(&scene);
}
int bh_frame_service_checkpoint(unsigned long *count){
    BHFrameService service;BHFrameBuffers frames;BHDrawQueue draw,slots[3];
    BHInputCollection input;BHInputSnapshot snapshot;BHPlayerInputOwner lists;
    BHPlayerInputList *list=NULL;BHPlayerInputState player;BHEntityPool entities;
    BHSoundQueue sound;BHInputDevices devices;BHCdClose cd;BHNetworkShutdown network;
    BHForceFeedback force;BHDisplayShutdown display;BHMathEnvironment fp;
    volatile LONG stop=0,worker_done=0,paused=0;volatile unsigned long counter=0;
    unsigned long i,before,raw;long active;BHEntity *entity,*spark=NULL;HRESULT hr;MSG msg;FILE *file;
    BHContactEffects effects;BHVec3 effect_position={1,2,3};BHSparkState *spark_state;
    BHPlayerInputState changed_player;
    const unsigned char changed_packet[8]={8,1,2,7,0x78,0x56,0x34,0x12};
    LPDIRECTPLAY3A peer=NULL;HWND window=NULL;int com=0;
    checks=failures=cursor_calls=recenter_calls=0;
    memset(&service,0,sizeof(service));memset(&frames,0,sizeof(frames));memset(&draw,0,sizeof(draw));
    memset(slots,0,sizeof(slots));memset(&lists,0,sizeof(lists));memset(&entities,0,sizeof(entities));
    memset(&sound,0,sizeof(sound));memset(&devices,0,sizeof(devices));memset(&player,0,sizeof(player));
    memset(&cd,0,sizeof(cd));memset(&network,0,sizeof(network));memset(&force,0,sizeof(force));memset(&display,0,sizeof(display));
    if(!bh_math_environment_enter(&fp))return 2;
    CHECK(bh_player_input_owner_init(&lists,NULL)&&bh_input_collection_init(&input,&snapshot,&lists),"actual source input/list owners");
    input.api.joystick=joystick;input.api.cursor=cursor;input.api.recenter=recenter;
    list=bh_player_input_create(&lists,1);CHECK(list!=NULL,"real source packet-producing binding list");if(!list)goto done;
    bh_player_input_bind_key(list,5,"isolated",0,0,'A');snapshot.current['A']=1;
    CHECK(bh_entities_init(&entities,2,NULL)&&bh_sound_queue_init(&sound,NULL,NULL,NULL),"actual empty entity and inactive sound owners");
    CHECK(bh_frame_buffers_bind(&frames,NULL)&&bh_frame_buffers_start(&frames,produce,&service,30,3500UL*84UL,message,&snapshot),
          "original three3500-record buffers with actual source producer/message bridges");
    if(!frames.mutex||!frames.bytes)goto done;
    service.input=&input;service.entities=&entities;service.sound=&sound;service.frames=&frames;service.draw=&draw;
    for(i=0;i<3;++i)service.slot_draw[i]=slots+i;
    service.player_input=&player;service.packet=list->state;service.packet_bytes=sizeof(list->state);
    service.paused=&paused;service.frame_counter=&counter;service.stop=&stop;service.worker_done=&worker_done;
    CHECK(bh_frame_service_tick(&service)&&counter==1&&service.produced==1,"whole normal producer collects and publishes an actual source frame");
    active=frames.latest;
    CHECK(active==0&&draw.commands==frames.slots[active].data&&draw.capacity==3500&&draw.count==9,
          "canonical borrowed descriptor selects actual oldest frame and initializes complete bucket headers");
    CHECK(slots[0].commands==draw.commands&&slots[0].count==draw.count&&slots[0].capacity==3500&&!frames.slots[0].busy&&!frames.writing,
          "per-slot metadata copied before publication for real worker consumption");
    CHECK(player.values[5]==1&&service.changed==1&&snapshot.previous['A']==1&&cursor_calls==1&&recenter_calls==1,
          "real collected binding packet applied only to isolated source player state");
    memcpy(&raw,&service.last_dt,4);CHECK(raw==0x3d088889&&service.sound_stats.clock_calls==1&&!service.sound_stats.play_calls,
          "original binary32 time step and real inactive-listener sound tick without playback");
    CHECK(service.step_calls==1&&service.refresh_calls==1&&service.packet_calls==1,"both actual empty-pool dispatch owners and packet consumer execute");
    consume_published(&frames,slots);
    entity=bh_entity_acquire(&entities);CHECK(entity!=NULL&&!entity->step&&!entity->refresh,"real active entity retains verified inactive RET-sentinel callbacks");
    CHECK(bh_contact_effects_init(&effects,&entities,&draw,141,142,143,144),
          "actual admitted effect producer retains canonical borrowed queue descriptor");
    spark=bh_contact_spark_create(&effects,&effect_position,1,0);
    CHECK(spark!=NULL&&spark->step&&spark->refresh,"real source effect constructor installs actual step and queue-producing refresh callbacks");
    paused=1;snapshot.current['A']=0;
    CHECK(bh_frame_service_tick(&service)&&counter==1&&frames.latest==1&&player.values[5]==0,
          "paused counter gate still steps refreshes sound applies key-up and publishes next frame");
    CHECK(slots[0].commands!=slots[1].commands&&slots[0].count==9&&slots[0].commands==frames.slots[0].data,
          "previous slot metadata retains its allocation after canonical descriptor advances");
    spark_state=bh_contact_spark_state(&effects,spark);
    CHECK(spark_state&&spark_state->lifetime<1&&slots[1].count==10&&slots[1].commands[9].kind==BH_DRAW_BILLBOARD&&
          slots[1].commands[9].resource==141&&effects.queue==&draw,
          "actual entity step and refresh append their draw command into the second selected allocation");
    memset(&changed_player,0xa5,sizeof(changed_player));observed_service=&service;
    observed_destination=&changed_player;observed_packet=changed_packet;observer_calls=0;
    if(entity)entity->refresh=observe_late_bindings;
    paused=0;counter=0xffffffffUL;
    CHECK(bh_frame_service_tick(&service)&&counter==0&&frames.latest==2,"frame counter preserves original32bit wrap and third actual buffer selection");
    CHECK(slots[2].count==10&&slots[2].commands[9].resource==141&&slots[1].commands[9].resource==141&&
          slots[2].commands!=slots[1].commands,"same real producer follows canonical descriptor while older published queue retains its command");
    CHECK(observer_calls==1&&changed_player.values[7]==0x12345678UL&&changed_player.values[8]==0xa5a5a5a5UL&&
          player.values[7]==0,"late refresh-time packet/player binding changes reach actual packet consumer without an early cached destination");
    if(entity)entity->refresh=NULL;
    service.packet=list->state;service.packet_bytes=sizeof(list->state);service.player_input=&player;
    service.packet_bytes=2;list->state[1]=1;list->flags=0;before=service.produced;
    CHECK(!bh_frame_service_tick(&service)&&service.error==BH_FRAME_SERVICE_PACKET&&service.produced==before+1&&!frames.writing,
          "source packet bounds guard records failure while final frame publication still occurs");
    service.packet_bytes=sizeof(list->state);list->state[1]=0;
    stop=1;worker_done=0;before=service.input_calls;
    CHECK(bh_frame_service_tick(&service)&&stop==1&&!service.shutdown_calls&&service.input_calls==before,
          "stop waits for worker acknowledgement without another producer tick");
    stop=2;worker_done=1;
    CHECK(bh_frame_service_tick(&service)&&!service.shutdown_calls,"signed stop greater-than-one gate suppresses repeated teardown");
    CHECK(bh_input_devices_bind(&devices,&input,NULL),"actual input shutdown owner bound without acquisition/start");
    window=CreateWindowEx(0,"STATIC","BHunter private frame service",0,0,0,0,0,HWND_MESSAGE,NULL,GetModuleHandle(NULL),NULL);
    CHECK(window!=NULL,"actual private message-only shutdown window");if(!window)goto done;
    display.window=window;display.log=fopen("frame-service-private.log","wb");
    CHECK(display.log!=NULL,"actual optional owned shutdown log");
    if(display.log)fputs("whole service cleanup",display.log);
    CHECK(bh_sound_resource_allocate(&sound)!=NULL,"real sound-owned allocation for actual teardown free");
    hr=CoInitialize(NULL);CHECK(SUCCEEDED(hr),"same-thread COM initialization to balance original shutdown tail");if(FAILED(hr))goto done;com=1;
    hr=CoCreateInstance(&CLSID_DirectPlay,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectPlay3A,(void**)&peer);
    CHECK(SUCCEEDED(hr)&&peer,"actual unopened native COM resource owned until service cleanup");
    if(peer){network.first=(IUnknown*)peer;peer=NULL;}
    force.enabled=1;service.devices=&devices;service.cd=&cd;service.network=&network;
    service.force_feedback=&force;service.display=&display;
    stop=1;
    CHECK(bh_frame_service_tick(&service)&&stop==2&&service.shutdown_calls==1,"acknowledged shutdown increments stop before all real cleanup owners");com=0;
    CHECK(service.cleanup_count==6,"six owning cleanup calls, common RET sentinel excluded");
    for(i=0;i<6;++i)CHECK(service.cleanup_order[i]==i+1,"input CD sound network force-feedback display cleanup order matches original");
    CHECK(!sound.resources&&!sound.allocator.release&&!network.first&&network.uninitializes==1&&!force.enabled&&display.requested==1,
          "actual sound allocation freed, unopened COM released, force flag cleared and display shutdown requested");
    CHECK(PeekMessage(&msg,window,WM_CLOSE,WM_CLOSE,PM_REMOVE)&&!msg.wParam&&!msg.lParam&&!display.log&&display.files_closed==1,
          "actual final private WM_CLOSE and optional fclose execute");
    before=service.shutdown_calls;CHECK(bh_frame_service_tick(&service)&&service.shutdown_calls==before,"positive stop2 blocks repeated COM cleanup");
    /* Negative values also pass the original SIGNED <=1 shutdown comparison. */
    CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL),"fresh sound lifetime for negative signed-stop fixture");
    hr=CoInitialize(NULL);CHECK(SUCCEEDED(hr),"balanced same-thread COM for signed-negative shutdown");
    if(SUCCEEDED(hr)){stop=-1;CHECK(bh_frame_service_tick(&service)&&stop==0&&service.shutdown_calls==before+1,
          "signed-negative shutdown increments toward zero and performs cleanup rather than unsigned-gating");}
done:
    observed_service=NULL;observed_destination=NULL;observed_packet=NULL;
    if(peer)IDirectPlay3_Release(peer);if(com)CoUninitialize();
    if(display.log)fclose(display.log);if(window)DestroyWindow(window);remove("frame-service-private.log");
    if(frames.mutex)CHECK(bh_frame_buffers_dispose(&frames),"actual frame allocations/mutex released after producer stops");
    if(entities.allocator.release)bh_entities_dispose(&entities);if(sound.allocator.release)bh_sound_queue_dispose(&sound);
    bh_player_input_owner_dispose(&lists);
    CHECK(bh_math_environment_supported(),"whole service preserves explicit callerPC53 profile");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"frame-service\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false,\"physical_input\":false,\"sessions_opened\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_FRAME_SERVICE_STANDALONE
int main(void){return bh_frame_service_checkpoint(NULL);}
#endif
