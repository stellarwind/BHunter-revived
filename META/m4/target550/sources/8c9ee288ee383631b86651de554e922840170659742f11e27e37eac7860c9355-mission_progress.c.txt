#include "../src/bh_mission_progress.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure line %d\n",__LINE__);}}while(0)
typedef struct Memory {int fail;unsigned long live;} Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory *m=(Memory*)user;void *p;if(m->fail)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory*)user;if(p)--m->live;free(p);}
static short progression(BHConfig *c){short v;memcpy(&v,c->settings+2,2);return v;}
static void put(BHConfig *c,short n){memcpy(c->settings+2,&n,2);}
static void expire(BHEntityPool *pool,BHEntity *e){unsigned long n=0;while(pool->used&&n++<130)e->step(e);CHECK(n>=119&&n<=122&&!pool->used);}
int bh_mission_progress_checkpoint(unsigned long *count){
 BHConfig config,loaded;BHScriptLifetimeContext life;BHMissionProgressContext c;BHScriptLifetimeState s;
 BHEntityPool pool;BHEntity *e,*filler;BHSoundQueue sounds;BHSoundResource *first,*second;BHMathEnvironment env;
 BHSceneAllocator allocator;Memory memory={0,0};char temp[MAX_PATH],path[MAX_PATH],bad[MAX_PATH+40];unsigned long attempts;
 checks=failures=0;CHECK(bh_math_environment_enter(&env));CHECK(GetTempPathA(sizeof(temp),temp)>0);CHECK(GetTempFileNameA(temp,"BHM",0,path)!=0);CHECK(DeleteFileA(path));
 CHECK(bh_config_init(&config,NULL));CHECK(bh_config_load(&config,path));CHECK(bh_config_valid(&config));put(&config,0);
 allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;CHECK(bh_entities_init(&pool,1,&allocator));CHECK(bh_sound_queue_init(&sounds,NULL,NULL,NULL));first=bh_sound_resource_allocate(&sounds);second=bh_sound_resource_allocate(&sounds);CHECK(first&&second);
 if(first&&second){first->streaming=second->streaming=1;first->base_frequency=second->base_frequency=8000;
  memset(&life,0,sizeof(life));life.timers.pool=&pool;life.sounds=&sounds;memset(&c,0,sizeof(c));c.config=&config;c.path=path;c.lifetime=&life;c.first_sound=first->index;c.unlocked_sound=second->index;
  e=bh_mission_progress(&c,3);CHECK(e&&progression(&config)==3&&c.save_result==1&&c.save_attempts==1);bh_script_lifetime_read(e,&s);CHECK(s.mission_countdown==4&&s.mission_resource==second->index&&e->step==bh_mission_sound_tick&&e->query==NULL);
  CHECK(bh_config_init(&loaded,NULL));CHECK(bh_config_load(&loaded,path)&&progression(&loaded)==3);bh_config_dispose(&loaded);expire(&pool,e);CHECK(second->stream_head!=NULL&&first->stream_head==NULL);
  attempts=c.save_attempts;e=bh_mission_progress(&c,0);CHECK(e&&progression(&config)==3&&c.save_result== -1&&c.save_attempts==attempts);bh_script_lifetime_read(e,&s);CHECK(s.mission_resource==first->index);expire(&pool,e);CHECK(first->stream_head!=NULL);
  strcpy(bad,path);strcat(bad,"\\missing-parent\\progress.ini");c.path=bad;e=bh_mission_progress(&c,5);CHECK(e&&progression(&config)==5&&c.error==2&&c.save_result==0&&config.error==BH_CONFIG_IO);expire(&pool,e);
  c.path=path;e=bh_mission_progress(&c,32768);CHECK(e&&progression(&config)== -32768&&c.save_result==1);expire(&pool,e);
  e=bh_mission_progress(&c,-1);CHECK(e&&progression(&config)== -1&&c.save_result==1);bh_script_lifetime_read(e,&s);CHECK(s.mission_resource==first->index);expire(&pool,e);
  filler=bh_entity_acquire(&pool);CHECK(filler!=NULL);memory.fail=1;e=bh_mission_progress(&c,7);CHECK(!e&&progression(&config)==7&&c.save_result==1&&c.error==3&&pool.used==1);memory.fail=0;CHECK(bh_entity_release(&pool,filler));
  CHECK(bh_config_init(&loaded,NULL));CHECK(bh_config_load(&loaded,path)&&progression(&loaded)==7);bh_config_dispose(&loaded);
 }
 CHECK(bh_sound_queue_dispose(&sounds));CHECK(bh_entities_dispose(&pool)&&memory.live==0);bh_config_dispose(&config);CHECK(DeleteFileA(path));bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"mission-progress\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_MISSION_PROGRESS_STANDALONE
int main(void){return bh_mission_progress_checkpoint(NULL);}
#endif
