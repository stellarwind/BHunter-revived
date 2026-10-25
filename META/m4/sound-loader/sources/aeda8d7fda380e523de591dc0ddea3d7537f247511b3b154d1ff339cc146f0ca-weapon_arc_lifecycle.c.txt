#include "../src/bh_weapon_arc_lifecycle.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

static unsigned long checks,failures,query_calls,query_event[32];
static BHEntity *live_weapon,*live_player,*replacement,*change_next;
static int query_mode,allocation_armed,allocation_mutations;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-arc-lifecycle: %s\n",s);}}while(0)
static void *__cdecl allocate(unsigned long bytes,void *user){
    if(allocation_armed){BHWeaponCommonState *s=bh_weapon_common_state(live_weapon);
        allocation_armed=0;++allocation_mutations;s->word_108=7;s->hud.target=replacement;s->hud.charge=77;
        live_player->position.x=31;live_player->origin.x=-61;}
    return malloc(bytes);
}
static void __cdecl release(void *p,void *user){free(p);}
static void set_aim(BHEntity *p,float x,float y,float z){BHVec3 v;v.x=x;v.y=y;v.z=z;memcpy((unsigned char *)p+0x280,&v,12);}
static void set_next(BHEntity *p,BHEntity *next){memcpy(p->payload+12,&next,4);}
/* Isolated observable event3 receiver. Actual production query methods are
   borrowed by source; no callback default is installed by lifecycle owners. */
static void __cdecl position_query(BHEntity *actor,void *raw){
    BHEntityPositionPacket *p=(BHEntityPositionPacket *)raw;
    if(query_calls<32)query_event[query_calls]=p->event;++query_calls;p->position=actor->position;
    if(query_mode==1){p->event=17;if(query_calls==1)set_next(actor,change_next);}
    if(query_mode==2){bh_weapon_common_state(live_weapon)->hud.charge=91;set_aim(live_player,0,0,1);}
    if(query_mode==3){bh_weapon_common_state(live_weapon)->hud.target=replacement;}
}
typedef struct Mesh {
    BHBsp bsp;BHBspVertex vertices[4];BHBspEdge edges[4];BHBspFace face;
    BHBspNode node;BHBspLeaf leaf;BHBspMaterial material;unsigned char storage;
    BHGeometry geometry;BHSegmentQuery query;
} Mesh;
static int mesh_init(Mesh *m){
    unsigned long i;memset(m,0,sizeof(*m));m->bsp.vertices=m->vertices;m->bsp.edges=m->edges;
    m->bsp.faces=&m->face;m->bsp.nodes=&m->node;m->bsp.leaves=&m->leaf;
    m->bsp.materials=&m->material;m->bsp.storage=&m->storage;
    m->bsp.counts[0]=m->bsp.counts[3]=m->bsp.counts[4]=m->bsp.counts[5]=1;
    m->bsp.counts[1]=m->bsp.counts[2]=4;
    for(i=0;i<4;++i){m->vertices[i].y=(i==0||i==3)?-20:20;m->vertices[i].z=i<2?-20:20;
        m->edges[i].vertex=i;m->edges[i].next=i<3?i+1:-1;}
    m->node.normal[0]=1;m->node.front_node=m->node.back_node=-1;
    m->node.front_leaf=0;m->node.back_leaf=-1;m->leaf.face=0;m->leaf.next=-1;
    return bh_geometry_init(&m->geometry,&m->bsp,NULL)&&bh_segment_query_init(&m->query,&m->geometry);
}
static void clear_voices(BHSoundQueue *q){
    unsigned long i;for(i=0;i<q->resource_count;++i){BHSoundVoice *v=q->resources[i]->voices[0];
        if(v)bh_sound_stop(q,v->index+((unsigned long)v->generation<<16));}
}
static void reset(BHEntity *w,BHEntity *p){
    BHWeaponCommonState *s;memset(w,0,sizeof(*w));memset(p,0,sizeof(*p));s=bh_weapon_common_state(w);s->word_100=-1;
    set_aim(p,0,0,1);query_calls=0;query_mode=0;allocation_armed=0;
}
int bh_weapon_arc_lifecycle_checkpoint(unsigned long *count){
    BHMathEnvironment fp;BHEntity weapon,player,actors[6],before,expected;
    BHWeaponCommonState *s,*e;BHWeaponArcLifecycleContext c;BHWorldGeometry world;
    BHWorldSegmentRoom room;BHWorldSegmentContext query;Mesh mesh;
    BHSoundQueue sound;BHSceneAllocator allocator;LPDIRECTSOUND device=NULL;
    BHWave wave;unsigned char samples[64];BHSoundVoice *voice,*saved_free;unsigned long ids[4],i,handle,seed=891,first,second;
    float pitch,frequency;int have_mesh=0;
    checks=failures=0;memset(&sound,0,sizeof(sound));memset(&c,0,sizeof(c));memset(&world,0,sizeof(world));
    memset(&room,0,sizeof(room));memset(&query,0,sizeof(query));memset(&wave,0,sizeof(wave));
    memset(actors,0,sizeof(actors));live_weapon=&weapon;live_player=&player;replacement=actors+5;
    if(!bh_math_environment_enter(&fp))return 2;
    allocator.allocate=allocate;allocator.release=release;allocator.user=NULL;
    CHECK(DirectSoundCreate(NULL,&device,NULL)==DS_OK && device,"actual native sound device without playback");if(!device)goto done;
    CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK &&
        bh_sound_queue_init(&sound,device,NULL,&allocator),"real source sound queue ownership");
    memset(samples,0,sizeof(samples));wave.format_tag=1;wave.channels=1;wave.sample_rate=8000;wave.byte_rate=16000;
    wave.block_align=2;wave.bits=16;wave.sample_bytes=64;wave.samples=samples;
    for(i=0;i<4;++i){ids[i]=bh_sound_register_wave(&sound,&wave,0,1,0);CHECK(ids[i]!=BH_SOUND_INVALID,"actual PCM-backed sound cue resource");if(ids[i]==BH_SOUND_INVALID)goto done;}
    c.sound=&sound;c.activate_sound=ids[0];c.release_sound=ids[1];c.charge16_sound=ids[2];c.charge51_sound=ids[3];c.limit=32;
    world.navigation.next=&world.navigation;world.previous=&world;
    world.minimum.x=world.minimum.y=world.minimum.z=-500;world.maximum.x=world.maximum.y=world.maximum.z=500;
    world.transform.m[0][0]=world.transform.m[1][1]=world.transform.m[2][2]=1;world.inverse=world.transform;
    room.world=&world;room.kind=1;
    CHECK(bh_world_segment_init(&query,&world,1,&room,1,NULL,0,16),"actual empty room world-segment miss owner");c.world=&query;
    for(i=0;i<6;++i){actors[i].query=position_query;actors[i].position.z=10+(float)i;}
    reset(&weapon,&player);memset(&weapon,0xa5,sizeof(weapon));s=bh_weapon_common_state(&weapon);s->hud.target=actors;
    expected=weapon;e=bh_weapon_common_state(&expected);e->word_e8=0;e->hud.target=NULL;e->hud.charge=0;e->word_100=-1;e->word_108=0;
    first=(seed*214013UL+2531011UL);second=first*214013UL+2531011UL;first=(first>>16)&32767;second=(second>>16)&32767;
    srand(seed);CHECK(bh_weapon_arc_activate(&c,&weapon,&player) && !memcmp(&weapon,&expected,sizeof(weapon)),"activation mutates exact whole-allocation subset");
    CHECK(c.random_calls==1 && (unsigned long)rand()==second && c.requests==1,"activation consumes exactly one shared CRT random");
    bh_round_float(&pitch,.9+(double)first/163840.0);bh_round_float(&frequency,8000.0*pitch);voice=sound.resources[ids[0]]->voices[0];
    CHECK(voice && voice->frequency==frequency && voice->volume< -300 && voice->volume> -400,"real activation voice has independent exact pitch and .8 level mapping");clear_voices(&sound);
    reset(&weapon,&player);allocation_armed=1;saved_free=sound.free_voices;sound.free_voices=NULL;
    CHECK(bh_weapon_arc_activate(&c,&weapon,&player) && allocation_mutations==1,"activation crosses actual allocator mutation");s=bh_weapon_common_state(&weapon);
    sound.free_voices=saved_free;
    CHECK(s->hud.target==replacement && s->hud.charge==77 && !s->word_108,"latch clears after sound, allocator target/charge writes remain");
    voice=sound.resources[ids[0]]->voices[0];CHECK(voice && voice->position.x==31 && voice->velocity.x==-61,"live player vector addresses cross real sound allocation");clear_voices(&sound);
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);handle=bh_sound_request(&sound,ids[2],1,1,&player.position,&player.origin);
    s->word_100=(long)handle;s->hud.target=actors;s->hud.charge=55;s->word_108=9;s->word_f0=123;s->ammo=88;before=weapon;
    expected=before;e=bh_weapon_common_state(&expected);e->word_e8=0;e->hud.target=NULL;e->hud.charge=0;e->word_100=-1;e->word_108=0;
    CHECK(bh_weapon_arc_deactivate(&c,&weapon) && c.stops==1 && !memcmp(&weapon,&expected,sizeof(weapon)) && !bh_sound_voice(&sound,handle),"deactivation stops actual voice and preserves unrelated allocation bytes");
    CHECK(bh_weapon_arc_deactivate(&c,&weapon) && !c.stops,"sentinel deactivation skips native voice stop");
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->hud.target=actors;s->hud.charge=33;
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && !s->hud.target && !s->hud.charge && s->word_100==-1 && c.requests==1 && !c.queries,
        "untriggered owner releases charged target and clears charge without query");clear_voices(&sound);
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->word_108=1;s->hud.charge=71;c.head=actors;
    actors[0].position.z=100;actors[1].position.z=20;actors[2].position.z=20;actors[3].position.z=-2;actors[4].position.z=150;
    for(i=0;i<4;++i)set_next(actors+i,actors+i+1);set_next(actors+4,NULL);
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && s->hud.target==actors+1 && !s->hud.charge && !s->word_108 && c.queries==5 && c.world_queries==2,
        "acquisition queries ordered list and selects strict nearest positive projection, ties/boundary rejected");
    CHECK(s->word_104==(unsigned long)&world,"real world query publishes actual shared room cache104");
    for(i=0;i<3;++i){long charge=i==0?15:(i==1?50:LONG_MAX);reset(&weapon,&player);s=bh_weapon_common_state(&weapon);
        s->word_108=1;s->hud.target=actors+1;s->hud.charge=charge;
        CHECK(bh_weapon_arc_track(&c,&weapon,&player) && s->hud.target==actors+1 && s->hud.charge==(long)((unsigned long)charge+1UL) && c.queries==1,
            "retained target charge increments with exact signed32 wrap");
        if(i<2){CHECK(c.requests==1 && c.updates==0 && (unsigned long)s->word_100!=BH_SOUND_INVALID && sound.resources[ids[i==0?2:3]]->voices[0],"threshold16/51 requests correct actual cue and stores live handle");}
        else CHECK(!c.requests && !c.updates,"wrapped charge does not invent threshold cues");clear_voices(&sound);
    }
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->hud.target=actors;s->word_108=1;s->hud.charge=20;
    handle=bh_sound_request(&sound,ids[2],1,1,&player.position,&player.origin);s->word_100=(long)handle;actors[0].position.z=150;
    player.position.x=1;actors[0].position.x=1;
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && s->hud.target==actors && s->hud.charge==21 && c.updates==1 && !c.requests,
        "retention allows exact150 projection and updates existing actual voice");clear_voices(&sound);
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->hud.target=actors;s->word_108=1;s->hud.charge=32;actors[0].position.z=-10;actors[0].position.x=0;
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && !s->hud.target && s->hud.charge==33 && c.requests==1 && !s->word_108,
        "lost triggered target preserves charge and emits release cue above32");clear_voices(&sound);
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->hud.target=actors;s->word_108=1;actors[0].position=player.position;
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && s->hud.target==actors && s->hud.charge==1 && (_status87()&_SW_INVALID),
        "coincident retained target keeps original masked0/0 unordered acceptance and sticky invalid");_clear87();
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->word_108=1;c.head=actors;set_next(actors,NULL);actors[0].position=player.position;
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && s->hud.target==actors && !s->hud.charge && c.world_queries==1 && (_status87()&_SW_INVALID),
        "coincident acquisition keeps raw C0-only unordered cone acceptance and real world query");_clear87();
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->hud.target=actors;s->word_108=1;actors[0].position.z=10;
    set_aim(&player,0,0,-1);query_mode=2;
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && s->hud.target==actors && s->hud.charge==91,"query mutations of player aim and live charge remain observable");
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->hud.target=actors;s->word_108=1;query_mode=3;
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && s->hud.target==replacement,"query replacement of shared HUD target is retained");
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->word_108=1;c.head=actors;
    actors[0].position.z=20;actors[5].position.z=10;change_next=actors+5;query_mode=1;set_next(actors+5,NULL);
    CHECK(bh_weapon_arc_track(&c,&weapon,&player) && c.queries==2 && s->hud.target==actors+5 && query_event[0]==3 && query_event[1]==17,
        "successor is reloaded after query and packet event initializes once for full list");
    have_mesh=mesh_init(&mesh);CHECK(have_mesh,"actual BSP geometry and segment query blocker");
    if(have_mesh){
        reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->word_108=1;c.head=actors;set_next(actors,NULL);
        player.position.x=3;actors[0].position.x=-3;actors[0].position.z=0;set_aim(&player,-1,0,0);
        room.query=&mesh.query;world.query_flags=2;
        CHECK(bh_weapon_arc_track(&c,&weapon,&player) && !s->hud.target && c.world_queries==1 && !s->hud.charge,
            "actual BSP hit blocks acquisition through complete world-segment owner");
        room.query=NULL;world.query_flags=0;s->word_108=1;
        CHECK(bh_weapon_arc_track(&c,&weapon,&player) && s->hud.target==actors && c.world_queries==1,
            "same ray with absent actual collision model acquires visible target");
    }
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->word_108=1;c.head=actors;actors[0].position.x=0;actors[0].position.z=10;set_next(actors,actors);
    c.limit=2;CHECK(!bh_weapon_arc_track(&c,&weapon,&player) && c.error==BH_WEAPON_ARC_LIFECYCLE_LIMIT && !s->hud.charge && c.queries==2,
        "cycle guard bounds undefined list domain while retaining prior query effects");c.limit=32;set_next(actors,NULL);
    reset(&weapon,&player);s=bh_weapon_common_state(&weapon);s->word_108=1;c.head=actors;query.busy=1;
    CHECK(!bh_weapon_arc_track(&c,&weapon,&player) && c.error==BH_WEAPON_ARC_LIFECYCLE_WORLD && !s->hud.target && !s->hud.charge,
        "actual world dependency error is not fabricated into visibility");query.busy=0;
done:
    if(have_mesh){bh_segment_query_dispose(&mesh.query);bh_geometry_dispose(&mesh.geometry);}
    clear_voices(&sound);bh_sound_queue_dispose(&sound);if(device)IDirectSound_Release(device);
    CHECK(bh_math_environment_supported(),"arc lifecycle retains caller PC53 environment");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"weapon-arc-lifecycle\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_WEAPON_ARC_LIFECYCLE_STANDALONE
int main(void){unsigned long count;return bh_weapon_arc_lifecycle_checkpoint(&count);}
#endif
