#include "../src/bh_menu_runtime.h"
#include "../src/bh_atlas_page.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL menu runtime %s\n",n);}}while(0)
typedef struct Fixture {
 BHEntityPool pool;BHPlayerInputOwner input_owner;BHInputSnapshot snapshot;BHSoundQueue sound;
 BHConfig config;BHMenuActions action_context;BHMenuActionsRoot action_roots[2];
 BHMenuSettingsMenu menus[2];BHMenuSettingsRow row,*rows[2][3];BHMenuAction actions[2][3];
 BHMenuRuntime runtime[2],*destinations[2][3];BHMenuRuntimeContext context;
 BHNetworkSessions sessions;BHProviderRecord provider,*provider_pointer;
 BHMenuTransition transition;BHDrawQueue queue;
 BHFlybyScene scene;BHFlybyCameraContext flyby;BHFlybyCameraState state;BHCameraStateContext camera;
 BHCameraRouteContext route;BHWorldGeometry world;BHVec3 waypoints[11];BHPlayerCameraStart starts[11];
 BHAtlasImage atlas;BHAtlasPage page;BHTextQueue text;BHFlybyCredits credits;
 long mode,paused,waypoint,changed,stay;
} Fixture;
static int setup(Fixture *f) {
 unsigned long i,j;memset(f,0,sizeof(*f));
 if(!bh_entities_init(&f->pool,8,NULL)||!bh_player_input_owner_init(&f->input_owner,NULL)||!bh_sound_queue_init(&f->sound,NULL,NULL,NULL)||!bh_draw_queue_init(&f->queue,64,NULL))return 0;
 f->context.pool=&f->pool;f->context.snapshot=&f->snapshot;f->context.sound=&f->sound;
 f->context.sessions=&f->sessions;f->context.provider=(const BHProviderRecord**)&f->provider_pointer;f->provider_pointer=&f->provider;
 f->context.mode=&f->mode;f->context.paused=&f->paused;f->context.waypoint=&f->waypoint;f->context.input_changed=&f->changed;f->context.stay=&f->stay;
 f->context.transition=&f->transition;f->context.root=f->runtime;f->context.flyby=&f->scene;
 f->transition.pool=&f->pool;f->transition.draw=&f->queue;f->action_context.config=&f->config;
 for(i=0;i<2;++i){BHMenuRuntime *m=f->runtime+i;
  m->context=&f->context;m->entity=bh_entity_acquire(&f->pool);m->actions=f->action_roots+i;m->action_context=&f->action_context;m->destinations=f->destinations[i];
  m->input=bh_player_input_create(&f->input_owner,1);m->pending_selection=-1;m->back_selection=-1;m->slide_voice=BH_SOUND_INVALID;m->waypoint=-1;m->distance=4.5f;
  f->action_roots[i].menu=f->menus+i;f->action_roots[i].row_actions=f->actions[i];f->action_roots[i].action_count=3;
  f->menus[i].rows=f->rows[i];f->menus[i].row_count=3;for(j=0;j<3;++j)f->rows[i][j]=&f->row;
  f->actions[i][0]=f->actions[i][2]=bh_menu_action_video;
  f->destinations[i][0]=m;f->destinations[i][2]=f->runtime+(1-i);
  if(!m->entity||!m->input||!bh_menu_runtime_bind(m))return 0;
 }
 f->scene.camera_entity=bh_entity_acquire(&f->pool);f->scene.entities=&f->pool;f->scene.flyby_camera=&f->flyby;f->scene.flyby_state=&f->state;
 f->scene.mode_count=&f->mode;f->scene.selected_waypoint=&f->waypoint;f->scene.starts=f->starts;f->scene.start_count=11;
 for(i=0;i<11;++i){f->starts[i].position.x=(float)i;f->starts[i].position.y=20;f->starts[i].position.z=30;f->waypoints[i]=f->starts[i].position;}
 f->route.waypoints=f->waypoints;f->route.waypoint_count=11;f->route.first=&f->world;f->route.scene_count=1;
 f->world.navigation.next=&f->world.navigation;f->world.previous=&f->world;
 f->world.minimum.x=f->world.minimum.y=f->world.minimum.z=-100;f->world.maximum.x=f->world.maximum.y=f->world.maximum.z=100;
 f->flyby.route=&f->route;f->flyby.camera=&f->camera;f->flyby.starts=f->starts;f->flyby.start_count=11;
 f->page.atlas=&f->atlas;f->page.size=128;f->atlas.rectangle_count=96;for(i=0;i<96;++i){f->atlas.rectangles[i].width=8;f->atlas.rectangles[i].height=12;}
 bh_text_queue_init(&f->text);f->credits.gate=-1;f->flyby.credit_font=&f->page;f->flyby.text=&f->text;f->flyby.credits=&f->credits;
 return bh_flyby_scene_bind(&f->scene);
}
static void dispose(Fixture *f){bh_flyby_scene_unbind(&f->scene);bh_player_input_owner_dispose(&f->input_owner);bh_sound_queue_dispose(&f->sound);bh_draw_queue_dispose(&f->queue);bh_entities_dispose(&f->pool);}
static long setting(Fixture *f,unsigned long offset){long n;memcpy(&n,f->config.settings+offset,4);return n;}
int bh_menu_runtime_checkpoint(unsigned long *count) {
 Fixture f;BHMenuRuntime *m;unsigned long before,used;BHEntity *music;float distance;int ok;
 checks=0;failures=0;CHECK(setup(&f),"actual pool cached input sound and scene owners");m=f.runtime;
 f.paused=1;before=m->phase;m->pending_selection=2;CHECK(bh_menu_runtime_step(m)&&m->phase==before&&m->pending_selection==2,"pause suppresses every menu mutation");f.paused=0;
 CHECK(bh_menu_runtime_step(m)&&f.menus[0].selection==2&&m->pending_selection==-1&&m->phase==before+1,"pending row consumed and exact phase increment");
 f.menus[0].selection=0;f.menus[0].display_selection=0;m->input_state.values[0]=1;
 CHECK(bh_menu_runtime_step(m)&&f.menus[0].selection==2&&f.changed==1,"cached up wraps and skips disabled row");
 m->input_state.values[1]=1;CHECK(bh_menu_runtime_step(m)&&f.menus[0].selection==0,"cached down wraps actionable row");
 m->input_state.values[4]=1;before=(unsigned long)setting(&f,4);
 CHECK(bh_menu_runtime_step(m)&&setting(&f,4)==1-(long)(before&1)&&!m->exiting&&m->capture,"actual source row action and self destination capture");
 f.menus[0].selection=2;m->input_state.values[4]=1;m->push=0;
 CHECK(bh_menu_runtime_step(m)&&m->exiting&&m->input->flags==0&&f.runtime[1].distance==100&&f.runtime[1].input->flags==2,"destination activation and actual cached input enable owners");
 CHECK(f.runtime[1].entity->step==bh_menu_runtime_step_entity&&f.runtime[1].entity->refresh==bh_menu_runtime_refresh_entity,"whole menu callbacks installed");
 m->distance=-4.5f;CHECK(bh_menu_runtime_step(m)&&!m->entity->step&&!m->entity->refresh,"exiting below minus five disables real callbacks");
 m->exiting=0;m->distance=4.5f;m->capture=0;m->push=1;f.menus[0].selection=0;f.menus[0].display_selection=0;
 memset(&m->input_state,0,sizeof(m->input_state));m->input->state[1]=0;m->input_state.values[3]=0;before=f.context.sounds;
 m->scroll=1.1f;CHECK(bh_menu_runtime_step(m)&&f.menus[0].selection==2&&m->scroll<1&&f.context.sounds>before,"cached analog scroll threshold and source sound requests");
 f.provider.kind=2;f.provider.data[96]=0;m->actions->timeout=.01f;CHECK(bh_menu_runtime_step(m)&&m->actions->timeout<0&&f.sessions.enum_calls==0,"empty modem address timer skips session enumeration");
 f.provider.kind=0;f.sessions.count=1;f.sessions.records[0].instance.Data1=123;m->actions->timeout=.01f;f.menus[0].selection=2;
 CHECK(bh_menu_runtime_step(m)&&m->actions->timeout>.9f&&f.menus[0].selection==2,"session timeout retains GUID choice across actual absent-peer refresh");
 m->back_selection=0;m->input_state.values[8]=1;distance=m->distance;before=f.context.sounds;
 CHECK(bh_menu_runtime_step(m)&&f.menus[0].selection==0&&m->distance==distance&&f.context.sounds>before,"back action early return before animation");
    m->input_blocked=1;m->push=0;m->capture=0;f.menus[0].selection=2;f.menus[0].display_selection=0;m->input_state.values[4]=1;
    CHECK(bh_menu_runtime_step(m)&&f.menus[0].display_selection==.2f&&m->input_state.values[4]==1&&!m->push,"blocked input still animates increasing row selection");
    f.menus[0].selection=0;f.menus[0].display_selection=.1f;
    CHECK(bh_menu_runtime_step(m)&&f.menus[0].display_selection==0,"decreasing display selection clamps to integer row");
    m->capture=1;m->push=.1f;CHECK(bh_menu_runtime_step(m)&&!m->capture&&!m->push,"capture push release clamps zero and clears flag");m->input_blocked=0;
    m->input_state.values[4]=2;m->push=.95f;CHECK(bh_menu_runtime_step(m)&&m->push==1,"stable selected push approaches exact one");
 f.mode=3;f.sessions.count=5;used=f.pool.used;ok=bh_menu_runtime_return(&f.context);
 CHECK(ok&&f.mode==0&&f.sessions.count==0&&m->distance==100&&!m->exiting&&m->input->flags==2,"whole return restores main menu and clears network state");
 CHECK(f.waypoint==10&&f.context.camera_steps==50&&f.state.route.waypoint_index==10&&f.state.route.current==&f.world,"actual world ring locate and fifty source camera settling updates");
 CHECK(f.state.orbit_time>1.66&&f.state.orbit_time<1.67&&f.scene.camera_entity->step==bh_flyby_scene_camera_step_entity&&f.scene.camera_entity->refresh==bh_flyby_scene_refresh_entity,"actual flyby callbacks and timer reset");
 music=f.transition.music;CHECK(music&&music->step==bh_menu_music_tick&&bh_menu_transition_time(music)==.2&&f.pool.used==used+2,"real fade and deferred music entities");
 CHECK(bh_menu_runtime_return(&f.context)&&f.transition.music==music&&f.pool.used==used+3,"repeat return creates only fade while retaining actual music owner");
 dispose(&f);if(count)*count+=checks;printf("{\"scenario\":\"menu-runtime\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_MENU_RUNTIME_STANDALONE
int main(void){unsigned long n=0;return bh_menu_runtime_checkpoint(&n);}
#endif
