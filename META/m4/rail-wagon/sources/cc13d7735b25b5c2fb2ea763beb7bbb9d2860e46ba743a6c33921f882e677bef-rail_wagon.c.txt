#include "../src/bh_rail_wagon.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_sound_load.h"
#include "../src/bh_model.h"
#include "../src/bh_render_resources.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<24)printf("FAIL rail-wagon %d: %s\n",__LINE__,s);}}while(0)
static BHVec3 v(float x,float y,float z){BHVec3 a;a.x=x;a.y=y;a.z=z;return a;}
static void word(unsigned char *p,unsigned long value){memcpy(p,&value,4);}
static void scalar(unsigned char *p,float value){memcpy(p,&value,4);}
static int save(const char *name,const void *p,unsigned long n){FILE *f=fopen(name,"wb");int ok;if(!f)return 0;ok=fwrite(p,1,n,f)==n;return fclose(f)==0&&ok;}
static int fixtures(void){unsigned char model[272],wave[108];
    memset(model,0,sizeof(model));word(model,3);word(model+100,1);word(model+164,1);word(model+172,0xffffffffUL);word(model+184,1);word(model+188,1);
    word(model+200,1);scalar(model+216,6);scalar(model+228,1);word(model+244,0xffffffffUL);word(model+248,0xffffffffUL);word(model+252,0xffffffffUL);word(model+256,0xffffffffUL);word(model+260,1);word(model+268,0xffffffffUL);
    memset(wave,0,sizeof(wave));memcpy(wave,"RIFF",4);word(wave+4,100);memcpy(wave+8,"WAVEfmt ",8);word(wave+16,16);wave[20]=1;wave[22]=1;
    word(wave+24,8000);word(wave+28,16000);wave[32]=2;wave[34]=16;memcpy(wave+36,"data",4);word(wave+40,64);
    return save("rail-wagon-model.bsp",model,sizeof(model))&&save("rail-wagon-silent.wav",wave,sizeof(wave));}
static void worlds(BHWorldGeometry *w,BHPickupRoom *rooms,unsigned long n){unsigned long i;memset(w,0,n*sizeof(*w));memset(rooms,0,n*sizeof(*rooms));
    for(i=0;i<n;++i){w[i].navigation.next=&w[(i+1)%n].navigation;w[i].minimum=v(-5,-20,n==1?-20000:i==0?-5:15);w[i].maximum=v(n==1?20000:35,20,n==1?20000:i==0?15:35);rooms[i].world=w+i;}
    if(n==1){w[0].minimum.x=w[0].minimum.y=-20000;w[0].maximum.y=20000;}}
static int paths(BHRailPaths *p){BHVec3 zero=v(0,0,0),a,b;long i;
    if(!bh_rail_paths_init(p,NULL))return 0;
    for(i=0;i<12;++i){if(i<3){a=v(0,0,(float)i*10);b=v(0,0,(float)(i+1)*10);}
        else if(i<6){a=v((float)(i-3)*10,0,30);b=v((float)(i-2)*10,0,30);}
        else if(i<9){a=v(30,0,(float)(9-i)*10);b=v(30,0,(float)(8-i)*10);}
        else{a=v((float)(12-i)*10,0,0);b=v((float)(11-i)*10,0,0);}
        if(!bh_rail_add(p,&a,&b,&zero,&zero))return 0;}
    return bh_rail_link(p)&&p->chain_count==1;
}
static void configure(BHRailWagonContext *c,BHRailPaths *rails,BHEntityPool *pool,BHWorldGeometry *w,unsigned long n,BHPickupRoom *rooms,BHDrawQueue *draw,BHSoundQueue *sound,unsigned long resource,unsigned long model){
    memset(c,0,sizeof(*c));c->rails=rails;c->pool=pool;c->first_world=w;c->world_count=n;c->rooms=rooms;c->room_count=n;c->queue=draw;c->sound=sound;c->sound_resource=resource;c->model=model;}
static void remove_all(BHRailWagonContext *c){unsigned long n=0;long event=4;while(c->pool->active&&n++<1000){BHEntity *e=c->pool->active;
    if(bh_rail_wagon_state(e))bh_rail_wagon_query(e,&event);else bh_entity_release(c->pool,e);}CHECK(!c->pool->used,"every owned entity released by actual lifecycle definitions");}
static void active_cases(BHSoundQueue *sound,unsigned long resource,BHRenderResources *resources,unsigned long model_handle){
    BHRailPaths rails;BHEntityPool pool;BHWorldGeometry world[2];BHPickupRoom rooms[2];BHDrawQueue draw;BHRailWagonContext c;
    BHEntity *a,*b,*third,*rider,*recycled;BHRailWagonState *s,*bs,*ts;BHRailWagonPose *pose,*rpose;BHVec3 point=v(0,0,2.5f),angles=v(0,0,0),queued;
    BHEntityPositionPacket position;struct {long event;BHEntity *entity;} packet;long event;unsigned long before,instance,oldgen,clear,errors;BHRailSegment *old;const BHDrawResource *descriptor;
    memset(&rails,0,sizeof(rails));memset(&pool,0,sizeof(pool));memset(&draw,0,sizeof(draw));
    descriptor=bh_render_resources_lookup(model_handle,resources);
    CHECK(descriptor&&descriptor->tag==BH_RESOURCE_TAG_MODEL&&descriptor->data.model.instance->model&&descriptor->data.model.instance->cache->model==descriptor->data.model.instance->model,"actual admitted resource lookup owns model and cache");
    CHECK(paths(&rails)&&bh_entities_init(&pool,32,NULL)&&bh_draw_queue_init(&draw,128,NULL),"actual closed twelve-segment rail, pool and canonical draw owners");
    if(!rails.segments||!pool.chunks||!draw.commands)goto done;worlds(world,rooms,2);configure(&c,&rails,&pool,world,2,rooms,&draw,sound,resource,model_handle);CHECK(bh_draw_queue_begin(&draw),"actual bucket initialization");
    a=bh_rail_wagon_spawn(&c,&point,&angles);CHECK(a&&c.spawned==1&&rooms[0].actors==a&&a->query==bh_rail_wagon_query,"whole constructor installs actual fixed callbacks and room actor membership");if(!a)goto done;
    s=bh_rail_wagon_state(a);pose=bh_rail_wagon_pose(a);CHECK(s->kind==9&&s->segment==rails.segments&&s->direction==1&&s->elapsed==0.25f&&s->leader==a&&s->sound_instance==BH_SOUND_INVALID,"nearest projection, direction, initial interpolation and source resource bindings");
    position.event=3;bh_rail_wagon_query(a,&position);CHECK(position.position.x==0&&position.position.y==-5&&position.position.z==2.5f,"whole position query preserves actual five-unit up-axis offset");
    s->draw(a);CHECK(draw.count==10&&draw.commands[9].kind==BH_DRAW_MODEL_RESOURCE&&draw.commands[9].resource==model_handle&&bh_render_resources_lookup(draw.commands[9].resource,resources)==descriptor,"actual resource command references owning loaded model identity");
    memcpy(&queued,draw.commands[9].words,12);CHECK(queued.x==0&&queued.y==0&&queued.z==2.5f,"actual interpolated wagon render pose stored in canonical command");
    instance=s->sound_instance;CHECK(instance!=BH_SOUND_INVALID&&bh_sound_voice(sound,instance)!=NULL&&s->sound_cooldown==2,"actual deferred moving sound request retains source voice without playback");
    s->draw(a);CHECK(s->sound_instance==instance&&bh_sound_voice(sound,instance)!=NULL,"second draw updates existing real source voice instead of requesting another");
    s->elapsed=0.966666638851165771484375f;before=c.transitions;a->step(a);
    CHECK(s->elapsed==1.0f&&c.transitions==before,"unrounded +1/30 comparison postpones transition although binary32 stored elapsed reaches one");
    a->step(a);CHECK(c.transitions==before+1&&s->segment==rails.segments+1&&s->sound_cooldown==1,"next actual step crosses segment and decrements sound cooldown only on transition");
    s->segment=rails.segments+1;s->elapsed=0.99f;s->duration=1;s->speed=30;before=c.migrations;a->step(a);
    CHECK(c.migrations==before+1&&rooms[0].actors==NULL&&rooms[1].actors==a&&s->segment==rails.segments+2,"actual source world locate moves the shared room actor ring");
    CHECK(s->sound_instance==BH_SOUND_INVALID&&!bh_sound_voice(sound,instance),"second crossing stops actual deferred voice and expires source handle");
    rider=bh_entity_acquire(&pool);CHECK(rider!=NULL,"actual live rider entity");if(rider){
        memset(rider->opaque0,0,sizeof(rider->opaque0));memset(rider->opaque1,0x77,36);clear=123;memcpy(rider->payload+(0x23c-0xe4),&clear,4);
        s->segment=rails.segments;s->elapsed=0.2f;s->duration=1;s->speed=10;pose->up=v(0,1,0);pose->angles=v(1,0.5f,0.3f);
        CHECK(bh_rail_wagon_set_rider(a,rider,NULL),"explicit source binding retains rider generation outside original fields");pool.origin=v(1,2,3);a->step(a);rpose=bh_rail_wagon_pose(rider);
        memcpy(&clear,rider->payload+(0x23c-0xe4),4);
        CHECK(fabs(rider->position.z-((double)s->elapsed*10+5))<0.00001&&rider->position.y==-5&&rider->origin.z==10,"whole rider branch publishes lookahead pose and actual entity velocity");
        CHECK(fabs(rpose->angles.x-0.2f)<0.000001&&fabs(rpose->angles.y-0.1f)<0.000001&&rpose->angles.z==0.3f,"yaw/pitch smoothing and independently unscaled roll update");
        CHECK(clear==0&&!memcmp(rider->opaque1,&pool.origin,12)&&!memcmp(rider->opaque1+12,&pool.origin,12)&&!memcmp(rider->opaque1+24,&pool.origin,12),"original rider stop word clears while all three actual force vectors copy live nonzero shared origin");pool.origin=v(0,0,0);
        s->elapsed=0.8f;a->step(a);CHECK(rider->position.z>13&&rider->position.z<14,"lookahead crosses into next real segment while retaining whole rider branch");
        CHECK(bh_rail_wagon_set_rider(a,NULL,NULL)&&bh_entity_release(&pool,rider),"rider ownership ends before source pool release");}
    s->segment=rails.segments;s->elapsed=0.25f;s->duration=1;s->speed=30;s->direction=1;s->leader=a;a->step=bh_rail_wagon_tick;
    point=v(0,0,5);b=bh_rail_wagon_spawn(&c,&point,&angles);CHECK(b&&b->step==bh_rail_wagon_follow,"second real constructor broadcasts coupling and installs actual follower callback before own query registration");if(!b)goto done;bs=bh_rail_wagon_state(b);
    CHECK(bs->leader==a&&fabs(bs->spacing-2.5f)<0.0001,"raw ten-back/twenty-forward walk retains signed source spacing");
    bs->elapsed=1;before=c.follows;b->step(b);CHECK(c.follows==before+1&&bs->speed==s->speed,"whole follower recomputes distance from actual live leader");
    bs->draw(b);instance=bs->sound_instance;s->segment=rails.segments+2;s->elapsed=0.5f;s->duration=1;s->speed=30;bs->elapsed=1;before=c.migrations;b->step(b);
    CHECK(bs->segment==rails.segments+2&&c.migrations==before+1&&bs->sound_cooldown==1&&bh_sound_voice(sound,instance),"follower segment change migrates actual room ring and decrements live voice cooldown");
    s->segment=rails.segments;s->elapsed=0.1f;bs->elapsed=1;before=c.migrations;b->step(b);
    CHECK(bs->segment==rails.segments+11&&c.migrations==before+1&&bs->sound_cooldown==0&&bs->sound_instance==BH_SOUND_INVALID&&!bh_sound_voice(sound,instance),"follower backward distance walk migrates home and stops deferred voice on second changed segment");
    point=v(0,0,7.5f);third=bh_rail_wagon_spawn(&c,&point,&angles);CHECK(third!=NULL,"third actual wagon for equal-spacing overwrite");if(!third)goto done;ts=bh_rail_wagon_state(third);
    s->elapsed=ts->elapsed=0.25f;bs->elapsed=0.5f;s->duration=ts->duration=bs->duration=1;s->segment=ts->segment=bs->segment=rails.segments;s->direction=ts->direction=bs->direction=1;
    bs->leader=b;s->leader=a;ts->leader=third;a->step=third->step=bh_rail_wagon_tick;
    packet.event=19;packet.entity=b;bh_rail_wagon_query(a,&packet);CHECK(bs->leader==a,"actual coupling establishes non-self leader");
    bh_rail_wagon_query(third,&packet);CHECK(bs->leader==third&&bs->spacing==2.5f,"equal spacing overwrites existing leader as raw >= rule requires");
    s->elapsed=0.4f;bh_rail_wagon_query(a,&packet);CHECK(bs->leader==third&&bs->spacing==2.5f,"smaller new separation preserves existing leader");
    s->elapsed=0.1f;bh_rail_wagon_query(a,&packet);CHECK(bs->leader==a&&fabs(bs->spacing-4.0f)<0.000001,"larger new separation replaces existing leader");
    ts->elapsed=0;bh_rail_wagon_query(third,&packet);CHECK(bs->leader==third,"still larger separation can restore another actual live leader");
    packet.entity=(BHEntity*)1;errors=c.failures;bh_rail_wagon_query(a,&packet);CHECK(c.failures==errors+1&&c.error==BH_WAGON_LIFETIME,"foreign coupling pointer rejected by actual pool membership before dereference");
    oldgen=third->generation;event=20;bh_rail_wagon_query(third,&event);recycled=bh_entity_acquire(&pool);CHECK(recycled==third&&recycled->generation!=oldgen,"real release/reacquire reuses former leader slot with changed generation");
    errors=c.failures;b->step(b);CHECK(c.failures==errors+1&&c.error==BH_WAGON_LIFETIME&&recycled->step==NULL,"follower rejects recycled leader without rewriting its new owner");
    event=4;bh_rail_wagon_query(b,&event);bh_rail_wagon_query(a,&event);bh_entity_release(&pool,recycled);
    CHECK(!rooms[0].actors&&!rooms[1].actors&&pool.used==0,"both actual deletion events unlink all room membership and release pool owners");
    point=v(0,0,2.5f);a=bh_rail_wagon_spawn(&c,&point,&angles);CHECK(a!=NULL,"source remains usable after complete wagon lifetime");if(a){s=bh_rail_wagon_state(a);old=rails.segments[0].next;rails.segments[0].next=NULL;s->elapsed=1;
        errors=c.failures;a->step(a);CHECK(c.failures==errors+1&&c.error==BH_WAGON_TOPOLOGY,"open rail transition reports guard instead of null dereference");rails.segments[0].next=old;event=4;bh_rail_wagon_query(a,&event);}
    angles=v(3.14159265358979323846f,0,0);point=v(0,0,2.5f);a=bh_rail_wagon_spawn(&c,&point,&angles);
    CHECK(a&&bh_rail_wagon_state(a)->direction==0&&bh_rail_wagon_state(a)->elapsed==0.75f,"actual rotated constructor reverses interpolation direction");
    if(a){s=bh_rail_wagon_state(a);position.event=3;bh_rail_wagon_query(a,&position);CHECK(fabs(position.position.z-2.5f)<0.000001,"reverse query preserves same physical projection");
        s->elapsed=1;before=c.transitions;a->step(a);CHECK(c.transitions==before+1&&s->segment==rails.segments+11&&a->position.x==0&&a->position.z==0,"reverse main step follows previous rail and publishes its end point");event=4;bh_rail_wagon_query(a,&event);}
done:
    if(pool.chunks){if(rails.segments&&draw.commands){configure(&c,&rails,&pool,world,2,rooms,&draw,sound,resource,model_handle);remove_all(&c);}CHECK(bh_entities_dispose(&pool),"actual entity owner disposal");}
    if(draw.commands)CHECK(bh_draw_queue_dispose(&draw),"actual canonical command owner disposal");bh_rail_paths_dispose(&rails);
}
static void real_paths(const char *archive,BHSoundQueue *sound,unsigned long resource,BHRenderResources *resources,unsigned long model_handle){BHRailPaths rails;BHSceneLevel level;BHVFile *file;BHEntityPool pool;BHDrawQueue draw;BHWorldGeometry world;BHPickupRoom room;BHRailWagonContext c;BHVec3 point,angles;BHEntity *e;BHRailWagonState *s;unsigned long i;long event=4;
    bh_rail_paths_init(&rails,NULL);bh_scene_level_init(&level,NULL);memset(&pool,0,sizeof(pool));memset(&draw,0,sizeof(draw));
    file=bh_vfile_open("TEST.LEV","rb");CHECK(file&&bh_scene_level_read(&level,file),"actual LEV placement owner for full rail-wagon resource composition");if(file)bh_vfile_close(file);
    for(i=0;i<level.placement_count;++i){BHScenePlacement *p=level.placements+i;point=v((float)p->x,(float)p->y,(float)p->z);angles=v((float)((p->flags&3)*1.57079632679489661923),0,0);bh_rail_placement(&rails,p->name,&point,&angles);}
    CHECK(bh_rail_link(&rails)&&rails.count==132&&rails.chain_count==1,"actual world rail data provides stable closed runtime path");
    CHECK(bh_entities_init(&pool,8,NULL)&&bh_draw_queue_init(&draw,64,NULL)&&bh_draw_queue_begin(&draw),"real runtime composition owns pool and source queue");
    worlds(&world,&room,1);configure(&c,&rails,&pool,&world,1,&room,&draw,sound,resource,model_handle);
    point=rails.segments[0].start;point.x=(point.x+rails.segments[0].end.x)*0.5f;point.y=(point.y+rails.segments[0].end.y)*0.5f;point.z=(point.z+rails.segments[0].end.z)*0.5f;angles=v(0,0,0);
    e=bh_rail_wagon_spawn(&c,&point,&angles);CHECK(e!=NULL,"whole constructor binds actual132-segment source rail and immutable RCWAGON/metro resources");
    if(e){s=bh_rail_wagon_state(e);s->draw(e);CHECK(draw.count==10&&s->sound_instance!=BH_SOUND_INVALID&&draw.commands[9].resource==model_handle&&bh_render_resources_lookup(draw.commands[9].resource,resources)!=NULL,"actual archive runtime emits owning RCWAGON resource handle and deferred metro voice");
        s->elapsed=s->duration;e->step(e);CHECK(c.transitions>0&&!c.failures,"bounded whole callback step crosses actual rail segment using real math/world owners");bh_rail_wagon_query(e,&event);}
    CHECK(!pool.used&&!room.actors,"actual archive runtime deletion closes actor ownership");bh_entities_dispose(&pool);bh_draw_queue_dispose(&draw);bh_scene_level_dispose(&level);bh_rail_paths_dispose(&rails);
}
int bh_rail_wagon_checkpoint(const char *archive,unsigned long *count){BHMathEnvironment fp;BHSceneView view;BHTextureContext textures;BHModelContext models;BHRenderResources resources;BHModelInstanceContext prototype;BHMainCamera camera;BHClipContext clip;BHNativePrimitive native;unsigned long model_handle=0;BHSoundQueue sound;BHSoundLoad load;LPDIRECTSOUND device=NULL;unsigned long resource=BH_SOUND_INVALID;DWORD status;int entered,mounted=0;
    checks=failures=0;memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&models,0,sizeof(models));memset(&resources,0,sizeof(resources));memset(&prototype,0,sizeof(prototype));memset(&camera,0,sizeof(camera));memset(&native,0,sizeof(native));memset(&sound,0,sizeof(sound));memset(&load,0,sizeof(load));
    entered=bh_math_environment_enter(&fp);CHECK(entered,"admitted explicit PC53 source environment");if(!entered)goto done;
    CHECK(bh_scene_view_init(&view,64,64)&&bh_texture_context_init(&textures,view.draw,view.device,NULL)&&bh_texture_formats(&textures)&&bh_model_context_init(&models,&textures,NULL),"actual private offscreen model/texture dependencies");if(!view.device||!textures.allocator.release)goto done;
    CHECK(SUCCEEDED(DirectSoundCreate(NULL,&device,NULL))&&bh_sound_queue_init(&sound,device,NULL,NULL),"actual DirectSound/resource owners without playback");if(!sound.allocator.release)goto done;load.queue=&sound;
    CHECK(bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0)&&bh_clip_init(&clip,view.projection.near_z,view.projection.far_z,view.projection.left,view.projection.right,view.projection.top,view.projection.bottom),"actual camera and clip definitions bind model publication");
    native.textures=&textures;native.near_z=view.projection.near_z;prototype.camera=&camera;prototype.config=&view.projection;prototype.clip=&clip;prototype.emit=bh_native_primitive_emit;prototype.emit_user=&native;
    CHECK(bh_render_resources_init(&resources,&models,&prototype,NULL),"actual source resource owner with real native emitter binding");
    CHECK(fixtures(),"owned structurally valid BSP and silent RIFF fixtures");model_handle=bh_render_resources_load_model(&resources,"rail-wagon-model.bsp");resource=bh_sound_load(&load,"rail-wagon-silent.wav",30,1,1,8);
    CHECK(model_handle&&resource!=BH_SOUND_INVALID,"actual complete model/resource and sound loaders publish clean fixture owners");if(model_handle&&resource!=BH_SOUND_INVALID)active_cases(&sound,resource,&resources,model_handle);
    if(archive){mounted=bh_vfile_mount(archive);CHECK(mounted,"immutable archive mounted as data only");
        bh_texture_set_path(&textures,"TEXTURES",2);bh_model_set_path(&models,"BSP\\",BH_BSP_PREFIX_ONLY);model_handle=bh_render_resources_load_model(&resources,"RCWAGON.BSP");resource=bh_sound_load(&load,"sound\\metro.wav",30,1,1,8);
        CHECK(model_handle&&resource!=BH_SOUND_INVALID,"actual immutable RCWAGON.BSP and metro.wav through whole owning loaders");if(model_handle&&resource!=BH_SOUND_INVALID)real_paths(archive,&sound,resource,&resources,model_handle);}
    if(sound.resources)for(resource=0;resource<sound.resource_count;++resource){status=DSBSTATUS_PLAYING;CHECK(sound.resources[resource]->buffers[0]&&IDirectSoundBuffer_GetStatus(sound.resources[resource]->buffers[0],&status)==DS_OK&&!(status&DSBSTATUS_PLAYING),"all real native source buffers remain nonplaying after wagon request/update/stop lifetimes");}
done:
    bh_render_resources_dispose(&resources);if(mounted)CHECK(bh_vfile_detach_archive(),"fixture detaches its owned archive");bh_vfile_shutdown();
    if(sound.allocator.release)CHECK(bh_sound_queue_dispose(&sound),"actual deferred/native sound resource disposal");if(device)IDirectSound_Release(device);
    if(textures.allocator.release)bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);remove("rail-wagon-model.bsp");remove("rail-wagon-silent.wav");
    if(entered)bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"rail-wagon\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_RAIL_WAGON_STANDALONE
int main(int argc,char **argv){return bh_rail_wagon_checkpoint(argc>1?argv[1]:NULL,NULL);}
#endif
