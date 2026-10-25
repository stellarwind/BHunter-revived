#include "../src/bh_player_scene.h"
#include "../src/bh_fp.h"
#include "../src/bh_main_camera.h"
#include "../src/bh_view_camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned long checks,failures;
#define CHECK(v,n) do{++checks;if(!(v)){++failures;printf("player-scene: %s\n",n);}}while(0)
static void put_word(BHEntity *p,unsigned long offset,unsigned long value){
    memcpy((unsigned char *)p+offset,&value,4);
}
static void put_float(BHEntity *p,unsigned long offset,float value){
    memcpy((unsigned char *)p+offset,&value,4);
}
static float get_float(unsigned long word){float value;memcpy(&value,&word,4);return value;}
static float read_float(const BHEntity *p,unsigned long offset){
    float value;memcpy(&value,(const unsigned char *)p+offset,4);return value;
}
static unsigned long read_word(const BHEntity *p,unsigned long offset){
    unsigned long value;memcpy(&value,(const unsigned char *)p+offset,4);return value;
}
/* This receiver exists only in the isolated linked test. Production target
   queries remain the installed source owners on their actual allocations. */
static BHEntity *test_player;
static unsigned long test_query_count;
static void __cdecl mutate_selection_query(BHEntity *entity,void *raw){
    BHEntityPositionPacket *packet=(BHEntityPositionPacket *)raw;
    (void)entity;++test_query_count;
    packet->position.x=0;packet->position.y=0;packet->position.z=10;
    *((unsigned char *)test_player+0x2b7)=9;
    *((unsigned char *)test_player+0x2b9)=2;
    put_word(test_player,0xe8,0);
}
static unsigned long find_kind(const BHDrawQueue *q,unsigned long kind){
    unsigned long i,count=0;for(i=9;i<q->count;++i)if(q->commands[i].kind==kind)++count;return count;
}
static void tail_states(void){
    BHEntityPool pool;BHEntity *p;BHDrawQueue q;BHPlayerScene s;
    long frame=80,mode=0,width=641,height=481;unsigned long i;
    int after;
    memset(&pool,0,sizeof(pool));memset(&q,0,sizeof(q));memset(&s,0,sizeof(s));
    CHECK(bh_entities_init(&pool,2,0) && bh_draw_queue_init(&q,32,0),"real pool and queue owners initialize");
    p=bh_entity_acquire(&pool);CHECK(p!=0,"source player allocation acquired");
    s.player=p;s.queue=&q;s.frame_count=&frame;s.mode_count=&mode;
    s.display_width=&width;s.display_height=&height;
    s.cockpit_resource=71;s.overlay_image=72;s.trail_image=73;
    put_word(p,0x21c,61);
    srand(1);
    CHECK(bh_player_scene_draw(&s) && q.count==10 && q.commands[9].kind==BH_DRAW_MODEL_RESOURCE &&
          q.commands[9].resource==61 && s.random_calls==0 && rand()==41,
          "main gate closed, only independently gated player model is queued");
    CHECK(bh_draw_queue_begin(&q),"new source frame");
    frame=74;*((unsigned char *)p+0x2b4)=0x10;
    put_float(p,0x2c0,1);put_float(p,0x2c4,2);put_float(p,0x2c8,3);
    put_float(p,0x2fc,11);put_float(p,0x308,21);
    srand(1);
    CHECK(bh_player_scene_draw(&s) && q.count==17 && s.attempted==8 &&
          s.queued==8 && s.random_calls==4,"all below75 tail producers run");
    CHECK(q.commands[9].kind==BH_DRAW_MODEL_RESOURCE &&
          q.commands[10].kind==BH_DRAW_STATE && q.commands[10].words[0]==3 &&
          q.commands[11].kind==BH_DRAW_SCREEN_QUAD &&
          q.commands[11].resource==72 &&
          get_float(q.commands[11].words[0])==320 &&
          get_float(q.commands[11].words[2])==320 &&
          get_float(q.commands[11].words[5])==1 && q.commands[11].words[6]==0,
          "mode and overlay raw reservations retain signed integer midpoint");
    CHECK(q.commands[12].kind==BH_DRAW_MODEL_RESOURCE && q.commands[12].resource==71 &&
          q.commands[13].kind==BH_DRAW_SEGMENT && q.commands[16].kind==BH_DRAW_SEGMENT &&
          get_float(q.commands[13].words[0])==11 && get_float(q.commands[13].words[3])==21 &&
          fabs(get_float(q.commands[13].words[7])-(sqrt(26.0)+.5))<.00001,
          "cockpit then four trails use actual endpoints and signed motion expression");
    after=rand();
    srand(1);for(i=0;i<4;++i)rand();
    CHECK(after==rand(),"four trail draws advance the shared CRT stream exactly");
    CHECK(bh_draw_queue_dispose(&q) && bh_draw_queue_init(&q,10,0),"full source queue configured");
    s.queue=&q;srand(1);
    CHECK(bh_player_scene_draw(&s) && q.count==10 && s.attempted==8 &&
          s.queued==1 && s.dropped==7 && s.random_calls==4,
          "full queue drops every later publication and still consumes four random values");
    after=rand();srand(1);for(i=0;i<4;++i)rand();
    CHECK(after==rand(),"queue saturation preserves CRT ordering");
    CHECK(bh_draw_queue_begin(&q),"reset capped queue");
    frame=75;srand(1);
    CHECK(bh_player_scene_draw(&s) && q.count==10 && s.random_calls==0 &&
          q.commands[9].kind==BH_DRAW_MODEL_RESOURCE && rand()==41,
          "at75 only player resource remains and random stream is untouched");
    put_word(p,0x21c,0);bh_draw_queue_begin(&q);
    mode=0;*((unsigned char *)p+0x2bb)=1;
    CHECK(bh_player_scene_draw(&s) && q.count==9,
          "scene byte alone does not open main section when mode count is zero");
    s.frame_count=0;
    CHECK(!bh_player_scene_draw(&s) && s.error==BH_PLAYER_SCENE_ARGUMENT && q.count==9,
          "missing live global is a binding error without queue mutation");
    CHECK(bh_draw_queue_dispose(&q) && bh_entities_dispose(&pool),"owners disposed");
}
static void active_states(void){
    BHEntityPool pool;BHWeaponFactory factory;BHSoundQueue sound;
    BHEntity player,*actor=0,*pickup=0,*target_actor;BHDrawQueue q;
    BHSceneBackground background;BHSceneBindings bindings;BHSceneOwner scene;
    BHAttachmentContext graph;BHRadarFrame radar_frame;BHHudContext hud;
    BHRadarContext radar;BHHudMarkers markers;BHPickupDrawContext projection;
    BHHudWorldContext world_status;BHHudFlatContext flat_status;
    BHHudScoreDraw score_draw;BHHudScores scores;BHEntity *remote[8];
    BHArcContext arc;BHWeaponHudTargetContext target;BHViewConfig view;BHMainCamera camera;
    BHPlayerScene draw;BHCameraState *embedded;long mode=1,frame=100,width=320,height=240,confirmation=0;
    unsigned long i;int after;float flash;
    memset(&pool,0,sizeof(pool));memset(&factory,0,sizeof(factory));memset(&sound,0,sizeof(sound));
    memset(&player,0,sizeof(player));memset(&q,0,sizeof(q));memset(&background,0,sizeof(background));
    memset(&bindings,0,sizeof(bindings));memset(&scene,0,sizeof(scene));memset(&graph,0,sizeof(graph));
    memset(&radar_frame,0,sizeof(radar_frame));memset(&hud,0,sizeof(hud));memset(&radar,0,sizeof(radar));
    memset(&markers,0,sizeof(markers));memset(&projection,0,sizeof(projection));
    memset(&world_status,0,sizeof(world_status));memset(&flat_status,0,sizeof(flat_status));
    memset(&score_draw,0,sizeof(score_draw));memset(&scores,0,sizeof(scores));memset(remote,0,sizeof(remote));
    memset(&arc,0,sizeof(arc));memset(&target,0,sizeof(target));memset(&view,0,sizeof(view));
    memset(&camera,0,sizeof(camera));memset(&draw,0,sizeof(draw));
    CHECK(bh_entities_init(&pool,12,0) && bh_draw_queue_init(&q,1024,0) &&
          bh_sound_queue_init(&sound,0,0,0),"active fixture uses real source pool queue and sound");
    CHECK(bh_weapon_factory_open(&factory,&pool,&sound),"real weapon factory opened");
    for(i=0;i<10;++i){
        BHEntity *weapon=bh_weapon_factory_create(&factory,(unsigned int)i);
        CHECK(weapon!=0,"real indexed weapon allocated");
        put_word(&player,0x258+i*4,(unsigned long)weapon);
    }
    put_float(&player,0x18,1);put_float(&player,0x28,1);put_float(&player,0x38,1);
    put_float(&player,0x294,10);put_float(&player,0x288,1);
    put_float(&player,0x24c,.25f);put_float(&player,0x23c,.5f);
    *((unsigned char *)&player+0x2b6)=0;*((unsigned char *)&player+0x2b7)=9;
    *((unsigned char *)&player+0x2bb)=1;
    embedded=bh_player_camera_state(&player);embedded->position=player.position;
    CHECK(bh_view_config_init(&view,320,240) &&
          bh_main_camera_settings(&view,60,.01f,25000) &&
          bh_main_camera_pose(&view,&camera,0,0,0,0,0,0),
          "real camera and view state initialized");
    projection.queue=&q;projection.camera=&camera;projection.view=&view;
    graph.queue=&q;graph.frame=&radar_frame;graph.viewport_width=320;
    radar_frame.near_z=.01f;hud.attachment=&graph;hud.frame=&radar_frame;hud.background_image=4;
    CHECK(bh_radar_bind(&radar,&q,&radar_frame,6,7),"real radar owner bound");
    markers.projection=&projection;markers.radar=&radar;markers.limit=12;
    world_status.hud=&hud;world_status.atlas_image=8;world_status.weapon_icons_image=9;
    flat_status.hud=&hud;flat_status.atlas_image=8;flat_status.weapon_icons_image=9;
    CHECK(bh_scene_background_init(&background,&q,1,1,5,5) &&
          bh_scene_owner_init(&scene,&bindings,320,240),
          "real background and scene owners initialized");
    arc.queue=&q;arc.image=5;target.projection=&projection;target.atlas_image=7;
    score_draw.queue=&q;score_draw.page_image=8;scores.remote=remote;
    scores.local_name="Player";scores.local_score=10;scores.local_index=0;
    draw.player=&player;draw.queue=&q;draw.sound=&sound;draw.hud=&hud;
    draw.markers=&markers;draw.radar=&radar;draw.world_status=&world_status;
    draw.flat_status=&flat_status;draw.score_draw=&score_draw;draw.scores=&scores;
    draw.background=&background;draw.scene=&scene;draw.weapons=&factory.registry;
    draw.arc=&arc;draw.target=&target;draw.actor_head=&actor;draw.pickup_head=&pickup;
    draw.mode_count=&mode;draw.frame_count=&frame;draw.display_width=&width;
    draw.display_height=&height;draw.confirmation_gate=&confirmation;
    draw.material_image=8;draw.horizon_image=1;draw.aim_image=2;
    draw.aim_overlay_image=3;draw.particle_image=10;draw.confirmation_image=11;
    draw.trail_image=5;draw.overlay_image=4;draw.cockpit_resource=12;draw.pickup_limit=12;
    for(i=0;i<8;++i){background.slots[i].elapsed=0;background.slots[i].delay=100;}
    put_float(&player,0x254,.099f);srand(1);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_CAMERA)==1 &&
          find_kind(&q,BH_DRAW_HORIZON)==1 && find_kind(&q,BH_DRAW_MATERIAL_WORDS)==1 &&
          find_kind(&q,BH_DRAW_FADE)==1 && read_float(&player,0x254)==.099f,
          "active main section queues owners and low flash queues zero fade without decay");
    CHECK(bh_draw_queue_begin(&q),"unordered flash frame reset");
    put_word(&player,0x254,0x7fc00000UL);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_FADE)==1 &&
          read_word(&player,0x254)==0x7fc00000UL,
          "unordered flash follows original zero-fade branch without integer color conversion");
    CHECK(bh_draw_queue_begin(&q),"active frame reset");
    put_float(&player,0x254,.1f);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_FADE)==0 &&
          read_float(&player,0x254)<.1f,
          "binary32 0.1 exceeds double threshold, so it decays without visible fade");
    CHECK(bh_draw_queue_begin(&q),"damage frame reset");
    put_float(&player,0x254,.5f);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_FADE)==1 &&
          read_float(&player,0x254)<.5f,"positive flash queues squared fade and decays");
    for(i=9;i<q.count;++i)if(q.commands[i].kind==BH_DRAW_FADE)break;
    CHECK(i<q.count && fabs(get_float(q.commands[i].resource)-.25)<.00001 &&
          q.commands[i].words[0]==((0xff00UL+95UL)<<8),
          "fade amount and signed color come from distinct live player fields");
    CHECK(bh_draw_queue_begin(&q),"late frame reset");
    frame=180;mode=1;flash=read_float(&player,0x254);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_FADE)==1 &&
          read_float(&player,0x254)==flash,
          "single-player late frame uses timed fade and preserves damage");
    for(i=9;i<q.count;++i)if(q.commands[i].kind==BH_DRAW_FADE)break;
    CHECK(i<q.count && q.commands[i].words[0]==0 &&
          fabs(get_float(q.commands[i].resource)-.6)<.00001,
          "late fade has zero color and frame-based amount");
    CHECK(bh_draw_queue_begin(&q),"multiplayer frame reset");
    mode=2;put_float(&player,0x254,.5f);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_FADE)==1 &&
          read_float(&player,0x254)<.5f,
          "multiple players retain flash path after frame150 and run scoreboard");
    CHECK(bh_draw_queue_begin(&q),"synchronous target-query frame reset");
    target_actor=bh_entity_acquire(&pool);
    CHECK(target_actor && bh_entity_set_query(&pool,target_actor,mutate_selection_query),
          "isolated event3 receiver has real pool lifetime");
    test_player=&player;test_query_count=0;
    bh_weapon_object_state(factory.registry.slots[5].entity)->common.hud.target=target_actor;
    *((unsigned char *)&player+0x2b6)=5;*((unsigned char *)&player+0x2b7)=5;
    put_word(&player,0xe8,1);world_status.attempted=flat_status.attempted=0;
    frame=100;mode=1;put_float(&player,0x254,0);
    CHECK(bh_player_scene_draw(&draw) && test_query_count==1 &&
          *((unsigned char *)&player+0x2b7)==9 &&
          world_status.attempted>0 && flat_status.attempted==0 &&
          *((unsigned char *)&player+0x2b9)==1,
          "primary event3 mutation is visible to secondary selector, status mode and countdown copyback");
    bh_weapon_object_state(factory.registry.slots[5].entity)->common.hud.target=0;
    *((unsigned char *)&player+0x2b6)=0;*((unsigned char *)&player+0x2b7)=9;
    CHECK(bh_entity_release(&pool,target_actor),"isolated event3 target released");
    CHECK(bh_draw_queue_begin(&q),"particle frame reset");
    mode=1;frame=100;put_float(&player,0x254,0);put_float(&player,0x240,1);
    srand(1);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_ORIENTED)==32 &&
          draw.random_calls==128,"transient burst queues32 draws and consumes128 CRT values");
    after=rand();srand(1);for(i=0;i<128;++i)rand();
    CHECK(after==rand(),"particle producer shares one CRT stream in original call order");
    CHECK(bh_draw_queue_begin(&q),"unordered transient frame reset");
    put_word(&player,0x240,0x7fc00000UL);srand(1);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_ORIENTED)==0 &&
          draw.random_calls==0 && rand()==41,
          "unordered transient gate skips burst and leaves CRT stream untouched");
    CHECK(bh_draw_queue_begin(&q),"unordered remaining frame reset");
    put_float(&player,0x240,1);put_word(&player,0x244,0x7fc00000UL);srand(1);
    CHECK(bh_player_scene_draw(&draw) && find_kind(&q,BH_DRAW_ORIENTED)==32 &&
          draw.random_calls==128,
          "unordered remaining lifetime uses effect-based fade and all128 random calls");
    put_float(&player,0x244,0);
    CHECK(bh_draw_queue_dispose(&q) && bh_draw_queue_init(&q,9,0),"exhausted active queue configured");
    draw.queue=&q;graph.queue=&q;radar.queue=&q;projection.queue=&q;
    background.queue=&q;arc.queue=&q;score_draw.queue=&q;
    for(i=0;i<8;++i){background.slots[i].elapsed=0;background.slots[i].delay=100;}
    srand(1);
    CHECK(bh_player_scene_draw(&draw) && q.count==9 && draw.random_calls==128,
          "full queue preserves all transient random draws and source state");
    after=rand();srand(1);for(i=0;i<128;++i)rand();
    CHECK(after==rand(),"full queue does not alter CRT stream position");
    for(i=0;i<10;++i)bh_entity_release(&pool,factory.registry.slots[i].entity);
    CHECK(bh_weapon_factory_close(&factory),"factory registry closed after real releases");
    CHECK(bh_sound_queue_dispose(&sound) && bh_entities_dispose(&pool) &&
          bh_draw_queue_dispose(&q),"active source owners disposed");
}
int bh_player_scene_checkpoint(unsigned long *count){
    BHMathEnvironment guard;checks=failures=0;
    if(!bh_math_environment_enter(&guard)){if(count)*count=0;return 1;}
    tail_states();active_states();bh_math_environment_leave(&guard);
    if(count)*count=checks;
    printf("{\"scenario\":\"player-scene\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);
    return (int)failures;
}
