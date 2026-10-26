#include "../src/bh_script_actor_draw.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure line %d\n",__LINE__);}}while(0)
static unsigned long seed(int emit){unsigned long i;for(i=0;i<65536;++i){srand(i);if((rand()<0x800)==emit)return i;}return 0;}
int bh_script_actor_draw_checkpoint(unsigned long *count){
 BHScriptActorDrawContext c;BHScriptParticleContext particles;BHDrawQueue queue;BHEntityPool pool;BHEntity a,*particle;
 BHActorRouteState s;BHScriptActorPath path=bh_script_actor_linear;BHMathEnvironment env;BHScriptParticleState *ps;
 unsigned long value=987,sd,i,expected;float pose[8];BHVec3 offset={2,3,4},velocity;
 checks=failures=0;CHECK(bh_math_environment_enter(&env));CHECK(bh_entities_init(&pool,8,0));CHECK(bh_draw_queue_init(&queue,16,0));CHECK(bh_script_particles_init(&particles,&pool,&queue));
 memset(&a,0,sizeof(a));memset(&c,0,sizeof(c));memset(&s,0,sizeof(s));c.queue=&queue;c.particles=&particles;c.smoke_resource=44;c.tick=15;
 s.duration_ticks=30;s.controls[1].x=100;bh_actor_route_write(&a,&s);memcpy((char*)&a+0x1a0,&path,4);memcpy((char*)&a+0x174,&offset,12);memcpy((char*)&a+0x114,&value,4);
 sd=seed(0);srand(sd);rand();expected=rand();srand(sd);CHECK(bh_script_actor_draw(&c,&a));CHECK(rand()==expected&&pool.used==0&&queue.count==10);
 CHECK(queue.commands[9].resource==987&&queue.commands[9].kind==BH_DRAW_MODEL_RESOURCE);memcpy(pose,queue.commands[9].words,24);
 CHECK(fabs(pose[0]-54)<.00001&&pose[1]==3&&fabs(pose[2]-2)<.00001&&fabs(pose[3]-1.5707963267948966)<.00001);
 memcpy(&velocity,(char*)&a+0x48,12);CHECK(velocity.x< -19.999&&velocity.x> -20.001&&velocity.y== -90&&fabs(velocity.z+60)<.0001);memcpy(&value,(char*)&a+0x108,4);CHECK(value==4);
 CHECK(bh_draw_queue_begin(&queue));sd=seed(1);srand(sd);for(i=0;i<7;++i)rand();expected=rand();srand(sd);
 CHECK(bh_script_actor_draw(&c,&a));CHECK(rand()==expected&&pool.used==1&&queue.count==10);particle=pool.active;ps=bh_script_particle_state(&particles,particle);
 CHECK(ps&&ps->resource==44&&ps->bucket==3&&ps->lifetime==2.5f&&ps->alpha==.25f&&ps->size==1&&ps->size_step==.5f&&ps->acceleration.y==-.003f);
 CHECK(particle->position.x==50&&particle->position.y==2&&particle->position.z==0);
 CHECK(bh_draw_queue_begin(&queue));c.glowing_model=987;c.glow_first=111;c.glow_second=112;c.animation_time=0;
 s.controls[2].x=100;s.controls[3].x=200;bh_actor_route_write(&a,&s);srand(seed(0));
 CHECK(bh_script_actor_vehicle_draw(&c,&a));CHECK(queue.count==12&&pool.used==1);
 memcpy(pose,queue.commands[9].words,24);CHECK(fabs(pose[0]-104)<.00001&&pose[1]==3&&fabs(pose[2]-2)<.00001&&pose[4]==0&&pose[5]==0);
 CHECK(queue.commands[10].resource==111&&queue.commands[11].resource==112&&queue.commands[10].kind==BH_DRAW_BILLBOARD);
 memcpy(pose,queue.commands[10].words,32);CHECK(pose[3]==9&&pose[4]==9&&pose[6]==.5f);
 memcpy(pose,queue.commands[11].words,32);CHECK(pose[3]==7.5f&&pose[4]==7.5f&&pose[6]==.5f);
 CHECK(bh_entities_dispose(&pool));CHECK(bh_draw_queue_dispose(&queue));bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"script-actor-draw\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SCRIPT_ACTOR_DRAW_STANDALONE
int main(void){return bh_script_actor_draw_checkpoint(0);}
#endif
