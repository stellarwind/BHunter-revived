#include "bh_world_reset.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <string.h>
static float stored(double v){float f;bh_round_float(&f,v);return f;}
static void setword(BHEntity *e,unsigned long o,unsigned long v){memcpy((char*)e+o,&v,4);}
static int live(BHEntityPool *p,BHEntity *e){BHEntity *at;unsigned long n=0;for(at=p->active;at&&n++<p->capacity;at=at->next)if(at==e)return 1;return 0;}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
int __cdecl bh_world_reset(BHWorldReset *c){BHWorldResetNode *n;BHEntityPool *pool;BHEntity *e,*timer;BHWorldGeometry *w;BHScriptMotionActor motion;BHScriptLifetimeState life;BHScriptVMState vm;BHVec3 old,position,zero={0,0,0};unsigned long i,j,next;float angle,dx,dy;double dz;
 if(!c||!c->vm||!c->vm->lifetime||!(pool=c->vm->lifetime->timers.pool)||!c->nodes||!c->count||c->count>65535||!c->loot_rows||!bh_math_environment_supported()){if(c)c->error=1;return 0;}c->error=0;c->timers=c->immediate=0;
 for(i=0;i<c->count;++i)if(!c->nodes[i].entity||!c->nodes[i].placement||!c->nodes[i].definition||!live(pool,c->nodes[i].entity)){c->error=1;return 0;}
 for(i=0;i<c->count;++i){n=c->nodes+i;next=(i+1)%c->count;if(n->placement->geometry.navigation.next!=&c->nodes[next].placement->geometry.navigation||n->placement->geometry.previous!=&c->nodes[(i+c->count-1)%c->count].placement->geometry){c->error=1;return 0;}}
 memset(c->loot_rows,0,4*32*4);
 for(i=0;i<c->count;++i){n=c->nodes+i;e=n->entity;w=&n->placement->geometry;old=e->position;bh_script_motion_actor_read(e,&motion);angle=stored((w->flags&3UL)*1.5707963267948966);motion.angles.x=angle;motion.angles.y=motion.angles.z=0;
  for(j=0;j<3;++j){((float*)&position)[j]=stored((double)n->placement->coordinates[j]);if(!_finite(((float*)&old)[j])||!_finite(((float*)&motion.minimum)[j])||!_finite(((float*)&motion.maximum)[j])){c->error=3;return 0;}}
  e->position=position;identity(&motion.local_to_world);bh_mat34_translate_local(position.x,position.y,position.z,&motion.local_to_world);bh_mat34_rotate_y(angle,&motion.local_to_world);identity(&motion.world_to_local);bh_mat34_rotate_y(-angle,&motion.world_to_local);bh_mat34_translate_local(-position.x,-position.y,-position.z,&motion.world_to_local);
  setword(e,0x148,0);setword(e,0x14c,0);dx=stored((double)old.x-position.x);dy=stored((double)old.y-position.y);dz=(double)old.z-position.z;motion.minimum.x=stored((double)motion.minimum.x-dx);motion.maximum.x=stored((double)motion.maximum.x-dx);motion.minimum.y=stored((double)motion.minimum.y-dy);motion.maximum.y=stored((double)motion.maximum.y-dy);motion.minimum.z=stored((double)motion.minimum.z-dz);motion.maximum.z=stored((double)motion.maximum.z-dz);e->origin=zero;setword(e,0x114,n->definition->flags);bh_script_motion_actor_write(e,&motion);
  w->position=position;w->query_flags=n->definition->flags;w->transform=motion.local_to_world;w->inverse=motion.world_to_local;w->minimum=motion.minimum;w->maximum=motion.maximum;n->placement->angles=motion.angles;
  if(n->definition->script){timer=bh_entity_acquire(pool);if(timer){bh_script_lifetime_read(timer,&life);life.actor=e;life.script_loaded=c->vm->script_loading;life.sound=0;if(!bh_script_lifetime_bind(c->vm->lifetime,timer,&life,bh_script_unloaded_query)){c->error=2;return 0;}setword(timer,0xf8,1);setword(timer,0x100,0);setword(timer,0x114,0);setword(timer,0xe8,(unsigned long)n->definition->script);timer->position=pool->origin;timer->step=bh_script_timer_unconditional_step();++c->timers;
    if(n->definition->script->query==bh_script_vm_query&&bh_script_vm_read(n->definition->script,&vm)&&vm.opcode==20){++c->immediate;timer->step(timer);if(c->vm->error){c->error=2;return 0;}}
  }}
  setword(c->nodes[(i+1)%c->count].entity,0xb4,0x47c35000UL);
 }
 return 1;
}
