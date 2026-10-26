#include "../src/bh_flyby_construct.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;if(failures<10)printf("FAIL flyby construct line%d\n",__LINE__);}} while(0)
int __cdecl bh_flyby_construct_checkpoint(unsigned long *count) {
 BHEntityPool pool;BHEntity *player,*camera=NULL,*made;BHFlybyConstruct c;BHFlybyScene scene;
 BHFlybyCameraState state;BHFlybyCameraContext flyby;BHCameraRouteContext route;BHCameraStateContext camera_context,player_context;
 BHPlayerCameraContext reset;BHPlayerCameraStart starts[24];BHVec3 waypoints[24];BHFlybyCredits credits;
 BHAtlasPage page;BHAtlasImage atlas;BHTextQueue text;BHMathEnvironment env;long selected=0,mode=0,slot=0;unsigned long i;double before;
 checks=0;failures=0;CHECK(bh_math_environment_enter(&env));memset(&scene,0,sizeof(scene));memset(&state,0,sizeof(state));memset(&flyby,0,sizeof(flyby));
 memset(&route,0,sizeof(route));memset(&camera_context,0,sizeof(camera_context));memset(&player_context,0,sizeof(player_context));memset(&reset,0,sizeof(reset));
 memset(starts,0,sizeof(starts));memset(&page,0,sizeof(page));memset(&atlas,0,sizeof(atlas));memset(&c,0,sizeof(c));
 CHECK(bh_entities_init(&pool,2,NULL));player=bh_entity_acquire(&pool);CHECK(player!=NULL);CHECK(bh_camera_state_init(bh_player_camera_state(player),0));
 for(i=0;i<24;++i){starts[i].position.x=(float)i;starts[i].position.y=2;starts[i].position.z=3;waypoints[i]=starts[i].position;}
 page.atlas=&atlas;atlas.rectangle_count=96;bh_text_queue_init(&text);credits.gate=-1;credits.index=0;
 route.waypoints=waypoints;route.waypoint_count=24;flyby.route=&route;flyby.camera=&camera_context;flyby.starts=starts;flyby.start_count=24;
 flyby.credit_font=&page;flyby.text=&text;flyby.credits=&credits;
 reset.player=player;reset.starts=starts;reset.start_count=24;reset.camera=&player_context;
 scene.current_player=&player;scene.entities=&pool;scene.player_camera=&reset;scene.flyby_camera=&flyby;scene.flyby_state=&state;
 scene.starts=starts;scene.start_count=24;scene.mode_count=&mode;scene.selected_waypoint=&selected;scene.player_slot=&slot;
 c.scene=&scene;c.current_camera=&camera;made=bh_flyby_construct(&c);
 CHECK(made && made==camera && scene.camera_entity==made && c.settled==50 && pool.used==2);
 CHECK(made->step==bh_flyby_scene_camera_step_entity && made->refresh==bh_flyby_scene_refresh_entity && scene.bound_entity==made);
 CHECK(fabs(state.orbit_time-50.0/30)<1e-12 && state.camera.current.mode==10 && state.route.waypoint_index==0 && state.route.phase==0);
 CHECK(state.position.x==0 && state.position.y==2 && state.position.z==3 && !memcmp(&made->position,&state.position,12));
 CHECK(player->position.x==23 && player->position.y==2 && player->position.z==3 && bh_player_camera_state(player)->current.mode==1);
 CHECK(!bh_text_queue_count(&text));
 before=state.orbit_time;made->step(made);CHECK(state.orbit_time>before && scene.error==BH_FLYBY_SCENE_OK);
 CHECK(!bh_flyby_construct(&c) && pool.used==2 && camera==made);
 bh_flyby_scene_unbind(&scene);CHECK(!made->step && !made->refresh);CHECK(bh_entity_release(&pool,made));
 camera=NULL;scene.camera_entity=NULL;selected=24;CHECK(!bh_flyby_construct(&c) && pool.used==1 && !camera);
 selected=0;camera=player;flyby.credit_font=NULL;
 CHECK(!bh_flyby_construct(&c) && c.error==3 && camera==player && pool.used==1 && !scene.bound_entity && !scene.camera_entity);
 flyby.credit_font=&page;camera=NULL;
 made=bh_flyby_construct(&c);CHECK(made && c.settled==50 && pool.used==2);
 bh_flyby_scene_unbind(&scene);CHECK(bh_entity_release(&pool,made));
 CHECK(bh_entities_dispose(&pool));bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"flyby-construct\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_FLYBY_CONSTRUCT_STANDALONE
int main(void) {return bh_flyby_construct_checkpoint(NULL);}
#endif
