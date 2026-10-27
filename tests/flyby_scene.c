#include "../src/bh_flyby_scene.h"
#include "../src/bh_fp.h"
#include "../src/bh_atlas_page.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(v,n) do{++checks;if(!(v)){++failures;printf("flyby-scene: %s\n",n);}}while(0)
static void put_long(BHEntity *entity,unsigned long offset,long value) {
    memcpy((unsigned char *)entity+offset,&value,4);
}
static long get_long(const BHEntity *entity,unsigned long offset) {
    long value;memcpy(&value,(const unsigned char *)entity+offset,4);return value;
}
static void put_vector(BHEntity *entity,unsigned long offset,BHVec3 value) {
    memcpy((unsigned char *)entity+offset,&value,12);
}
static BHVec3 get_vector(const BHEntity *entity,unsigned long offset) {
    BHVec3 value;memcpy(&value,(const unsigned char *)entity+offset,12);return value;
}
static unsigned long count_kind(const BHDrawQueue *queue,unsigned long kind) {
    unsigned long i,count=0;
    for(i=9;i<queue->count;++i)if(queue->commands[i].kind==kind)++count;
    return count;
}
typedef struct Fixture {
    BHEntityPool entities;BHEntity *camera_entity,*player,*receiver,*current_player;
    BHDrawQueue queue;BHSoundQueue sound;BHSceneBackground background;
    BHSceneBindings bindings;BHSceneOwner scene;
    BHPlayerInputOwner input_owner;BHPlayerInputList *input_list;BHInputProfile profile;
    BHPlayerCameraStart starts[23];BHCameraStateContext player_camera_state;
    BHPlayerCameraContext player_camera;
    BHCameraRouteContext route;BHVec3 waypoints[23];
    BHCameraStateContext flyby_camera_state;BHAtlasImage atlas;
    BHAtlasPage page;BHTextQueue text;BHFlybyCredits credits;
    BHFlybyCameraContext flyby_camera;BHFlybyCameraState flyby_state;
    BHFlybyScene draw;
    long mode,selected,scenario,slot,width,height;
    float near_z;unsigned long event_value,mission_identity;
} Fixture;

static Fixture *query_fixture;static long received[2][3];static unsigned long receives;
static int mutate_camera_binding;
/* Isolated test receiver. The production scene uses the real pool dispatcher;
   no source or runtime callback resolves to this fixture outside this test. */
static void __cdecl capture_query(BHEntity *entity,void *raw) {
    long *words=(long *)raw;(void)entity;
    if(receives<2){received[receives][0]=words[0];received[receives][1]=words[1];
        received[receives][2]=words[2];}
    ++receives;
    if(receives==1){query_fixture->event_value=42;query_fixture->slot=1;
        if(mutate_camera_binding)query_fixture->draw.camera_entity=query_fixture->receiver;}
}
static int setup(Fixture *f) {
    unsigned long i;BHVec3 zero={0,0,0};
    memset(f,0,sizeof(*f));
    if(!bh_entities_init(&f->entities,4,0) || !bh_draw_queue_init(&f->queue,128,0) ||
       !bh_sound_queue_init(&f->sound,0,0,0) ||
       !bh_scene_background_init(&f->background,&f->queue,1,2,3,4) ||
       !bh_scene_owner_init(&f->scene,&f->bindings,320,240) ||
       !bh_player_input_owner_init(&f->input_owner,0))return 0;
    f->camera_entity=bh_entity_acquire(&f->entities);
    f->player=bh_entity_acquire(&f->entities);
    f->receiver=bh_entity_acquire(&f->entities);
    f->input_list=bh_player_input_create(&f->input_owner,0);
    if(!f->camera_entity || !f->player || !f->receiver || !f->input_list ||
       !bh_entity_set_query(&f->entities,f->receiver,capture_query))return 0;
    f->current_player=f->player;
    put_long(f->player,0x188,(long)f->input_list);
    bh_camera_state_init(bh_player_camera_state(f->player),0);
    for(i=0;i<23;++i){f->starts[i].position.x=(float)i;f->waypoints[i]=f->starts[i].position;}
    f->route.waypoints=f->waypoints;f->route.waypoint_count=23;
    f->page.atlas=&f->atlas;f->page.size=128;
    f->atlas.rectangle_count=96;
    for(i=0;i<96;++i){f->atlas.rectangles[i].width=8;f->atlas.rectangles[i].height=12;}
    bh_text_queue_init(&f->text);f->credits.gate=-1;
    f->flyby_camera.route=&f->route;f->flyby_camera.camera=&f->flyby_camera_state;
    f->flyby_camera.starts=f->starts;f->flyby_camera.start_count=23;
    f->flyby_camera.credit_font=&f->page;f->flyby_camera.text=&f->text;
    f->flyby_camera.credits=&f->credits;
    bh_camera_state_init(&f->flyby_state.camera,1);
    f->player_camera.player=f->player;f->player_camera.starts=f->starts;
    f->player_camera.start_count=23;f->player_camera.camera=&f->player_camera_state;
    f->mode=0;f->selected=0;f->scenario=9;f->slot=0;
    f->width=320;f->height=240;f->near_z=.01f;f->event_value=41;
    f->draw.camera_entity=f->camera_entity;f->draw.current_player=&f->current_player;
    f->draw.entities=&f->entities;f->draw.queue=&f->queue;f->draw.sound=&f->sound;
    f->draw.background=&f->background;f->draw.scene=&f->scene;
    f->draw.input_owner=&f->input_owner;f->draw.input_profile=&f->profile;
    f->draw.player_camera=&f->player_camera;f->draw.flyby_camera=&f->flyby_camera;
    f->draw.flyby_state=&f->flyby_state;f->draw.starts=f->starts;f->draw.start_count=23;
    f->draw.mode_count=&f->mode;f->draw.selected_waypoint=&f->selected;
    f->draw.scenario=&f->scenario;f->draw.player_slot=&f->slot;
    f->draw.display_width=&f->width;f->draw.display_height=&f->height;
    f->draw.near_z=&f->near_z;f->draw.event_value=&f->event_value;
    f->draw.mission_identity=&f->mission_identity;
    f->draw.horizon_image=10;f->draw.full_image=11;
    f->draw.corner_left_image=12;f->draw.corner_right_image=13;
    f->draw.scenario_resources[0]=76;
    for(i=0;i<8;++i){f->background.slots[i].elapsed=0;
        f->background.slots[i].delay=100;}
    put_vector(f->camera_entity,0x160,zero);
    return 1;
}
static void dispose(Fixture *f) {
    bh_flyby_scene_unbind(&f->draw);
    bh_player_input_owner_dispose(&f->input_owner);
    bh_sound_queue_dispose(&f->sound);bh_draw_queue_dispose(&f->queue);
    bh_entities_dispose(&f->entities);
}
static void render_and_handoff(void) {
    Fixture f;BHVec3 pose;unsigned long i;
    CHECK(setup(&f),"real source scene/input/entity owners initialize");
    query_fixture=&f;receives=0;memset(received,0,sizeof(received));
    CHECK(bh_flyby_scene_bind(&f.draw) &&
          f.camera_entity->refresh==bh_flyby_scene_refresh_entity &&
          f.camera_entity->step==bh_flyby_scene_camera_step_entity,
          "exact entity generation binds real source refresh and step");
    CHECK(bh_entities_refresh(&f.entities) && f.draw.error==BH_FLYBY_SCENE_OK &&
          count_kind(&f.queue,BH_DRAW_CAMERA)==1 &&
          count_kind(&f.queue,BH_DRAW_HORIZON)==1 &&
          count_kind(&f.queue,BH_DRAW_STATE)==1 &&
          count_kind(&f.queue,BH_DRAW_RECT)==2 && receives==0,
          "mode-zero rendering submits camera horizon and corner rectangles without handoff");
    CHECK(f.draw.attempted==5 && f.draw.queued==5 && f.draw.dropped==0,
          "render command count and queue publication");
    CHECK(bh_draw_queue_begin(&f.queue),"next draw frame");
    f.selected=17;put_long(f.camera_entity,0x284,0);
    CHECK(bh_flyby_scene_draw(&f.draw) && count_kind(&f.queue,BH_DRAW_RECT)==1 &&
          count_kind(&f.queue,BH_DRAW_STATE)==0 && f.queue.commands[11].kind==BH_DRAW_RECT &&
          f.queue.commands[11].resource==11,
          "selected image bypasses backgrounds and corner overlays");
    CHECK(bh_draw_queue_begin(&f.queue),"handoff draw frame");
    f.selected=0;f.mode=1;f.scenario=0;f.slot=0;
    f.input_list->state[1]=1;
    f.input_list->state[2]=2;
    f.input_list->state[3]=7;
    {unsigned long value=1234;memcpy(f.input_list->state+4,&value,4);}
    CHECK(bh_flyby_scene_draw(&f.draw) && receives==2 &&
          received[0][0]==14 && received[0][1]==41 && received[0][2]==0x2707 &&
          received[1][0]==14 && received[1][1]==42 && received[1][2]==0xd48,
          "real dispatcher sends two live event packets and reloads value/selector");
    CHECK(f.input_list->flags==2 &&
          get_long(f.player,0x18c+7*4)==1234 &&
          f.input_list->state[1]==1 && f.input_list->state[2]==2 &&
          f.input_list->state[3]==7 &&
          get_long(f.player,0x210)==2 && get_long(f.player,0x21c)==76 &&
          f.mission_identity==0x00415530UL && f.player->position.x==0,
          "input packet, mode, scenario resource and both real camera resets mutate live owners");
    CHECK(f.camera_entity->refresh==0 &&
          f.camera_entity->step==bh_flyby_scene_rearm_entity,
          "handoff installs verified RET refresh and complete rearm owner");
    f.camera_entity->step(f.camera_entity);
    CHECK(f.camera_entity->refresh==0,"rearm holds callbacks while players remain");
    f.mode=0;f.camera_entity->step(f.camera_entity);
    CHECK(f.camera_entity->refresh==bh_flyby_scene_refresh_entity &&
          f.camera_entity->step==bh_flyby_scene_camera_step_entity,
          "mode-zero rearm restores both real source callbacks");
    f.flyby_state.route.waypoint_index=0;f.flyby_state.route.phase=0;
    f.camera_entity->step(f.camera_entity);
    pose=get_vector(f.camera_entity,0x160);
    CHECK(!memcmp(&pose,&f.flyby_state.camera.position,sizeof(pose)) &&
          get_long(f.camera_entity,0x284)==(long)f.flyby_state.route.waypoint_index &&
          f.draw.error==BH_FLYBY_SCENE_OK,
          "installed camera step executes recovered source update and publishes entity pose");
    pose=get_vector(f.camera_entity,0x16c);
    CHECK(!memcmp(&pose,&f.flyby_state.camera.angles,sizeof(pose)),
          "rendered camera angles share the menu camera output, not actor orbit angles");
    pose=get_vector(f.camera_entity,0x178);
    CHECK(!memcmp(&pose,&f.flyby_state.camera.velocity,sizeof(pose)),
          "listener uses camera output velocity");
    bh_flyby_scene_unbind(&f.draw);
    CHECK(f.camera_entity->refresh==0 && f.camera_entity->step==0,
          "teardown removes exact-generation source callbacks");
    for(i=0;i<8;++i)CHECK(f.background.slots[i].delay<100,
          "all eight background owners advance on ordinary frames");
    dispose(&f);
}
static void queue_and_guards(void) {
    Fixture f;unsigned long i,nan_word=0x7fc00000UL;
    CHECK(setup(&f),"guard fixture source owners initialize");
    CHECK(bh_draw_queue_dispose(&f.queue) &&
          bh_draw_queue_init(&f.queue,10,0),"capped real draw queue");
    f.background.queue=&f.queue;
    CHECK(bh_flyby_scene_draw(&f.draw) && f.queue.count==10 &&
          f.draw.attempted==5 && f.draw.queued==1 && f.draw.dropped==4,
          "full queue retains all rendering attempts and successful callback");
    CHECK(!bh_flyby_scene_bind(0),"null binding rejects");
    f.draw.display_width=0;
    CHECK(!bh_flyby_scene_draw(&f.draw) &&
          f.draw.error==BH_FLYBY_SCENE_ARGUMENT && f.queue.count==10,
          "missing display owner rejects before queue mutation");
    f.draw.display_width=&f.width;
    for(i=0;i<8;++i)CHECK(f.background.slots[i].delay<100,
          "background owner still advances with full queue");
    dispose(&f);
    CHECK(setup(&f),"unordered-distance source owners initialize");
    put_long(f.camera_entity,0x284,13);
    memcpy((unsigned char *)f.camera_entity+0x160,&nan_word,4);
    CHECK(!bh_flyby_scene_draw(&f.draw) && f.draw.error==BH_FLYBY_SCENE_OWNER &&
          count_kind(&f.queue,BH_DRAW_RECT)==1 &&
          count_kind(&f.queue,BH_DRAW_STATE)==0,
          "unordered blackout still queues image while sound rejects nonfinite listener");
    dispose(&f);
}
static void multiplayer_handoff(void) {
    Fixture f;int after;unsigned long i;
    CHECK(setup(&f),"multiplayer source owners initialize");
    query_fixture=&f;receives=0;memset(received,0,sizeof(received));
    f.mode=2;f.scenario=9;f.slot=5;
    srand(1);
    CHECK(bh_flyby_scene_draw(&f.draw) && receives==2 &&
          received[0][2]==0x2707 && received[1][2]==0xd03 &&
          f.player->position.x==-.25f,
          "multiplayer relocation uses real selected start and generic second event");
    after=rand();srand(1);for(i=0;i<1;++i)rand();
    CHECK(after==rand() && f.draw.random_calls==1,
          "multiplayer branch consumes exactly one shared CRT random value");
    CHECK(f.mission_identity==0 && get_long(f.player,0x21c)==0,
          "unknown scenario preserves existing resource and mission selection");
    dispose(&f);
}
static void captured_camera_identity(void) {
    Fixture f;
    CHECK(setup(&f),"query-mutation fixture source owners initialize");
    query_fixture=&f;receives=0;mutate_camera_binding=1;
    f.mode=1;f.scenario=9;f.slot=0;
    CHECK(bh_flyby_scene_bind(&f.draw) && bh_flyby_scene_draw(&f.draw) &&
          f.camera_entity->step==bh_flyby_scene_rearm_entity &&
          f.camera_entity->refresh==0 && f.receiver->step==0,
          "final callback writes retain original entity across synchronous query mutation");
    bh_flyby_scene_unbind(&f.draw);
    CHECK(f.camera_entity->step==0 && f.draw.bound_entity==0,
          "teardown uses stable bound entity even if context pointer changed");
    mutate_camera_binding=0;dispose(&f);
}
int bh_flyby_scene_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;
    if(!bh_math_environment_enter(&fp))return 1;
    render_and_handoff();queue_and_guards();multiplayer_handoff();captured_camera_identity();
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"flyby-scene\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_STANDALONE
int main(void){unsigned long count=0;return bh_flyby_scene_checkpoint(&count);}
#endif
