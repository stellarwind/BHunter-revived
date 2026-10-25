#include "../src/bh_script_wait.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<14)printf("FAIL script-wait %lu: %s\n",checks,s);}}while(0)
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory *m=(Memory*)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory*)user;--m->live;free(p);}
typedef struct Fixture {BHEntityPool pool;BHScriptTimerContext ctx;BHScriptTimerActor actors[2];BHEntity *a,*b,*wait,*fired,*next;Memory memory;} Fixture;
static int fixture(Fixture *f){
 BHSceneAllocator allocator;BHScriptTimerState timer;memset(f,0,sizeof(*f));allocator.allocate=allocate;allocator.release=release;allocator.user=&f->memory;
 if(!bh_entities_init(&f->pool,8,&allocator))return 0;
 f->a=bh_entity_acquire(&f->pool);f->b=bh_entity_acquire(&f->pool);f->wait=bh_entity_acquire(&f->pool);f->fired=bh_entity_acquire(&f->pool);f->next=bh_entity_acquire(&f->pool);
 if(!f->a||!f->b||!f->wait||!f->fired||!f->next)return 0;
 f->a->position.x=1;f->a->position.y=2;f->a->position.z=3;f->b->position.x=9;
 f->actors[0].entity=f->a;f->actors[0].active=1;f->actors[0].variables[0]=2;f->actors[0].variables[1]=345;f->actors[0].variables[21]=-32768;
 f->actors[1].entity=f->b;f->actors[1].active=1;
 if(!bh_script_timers_init(&f->ctx,&f->pool,f->actors,2))return 0;
 memset(&timer,0,sizeof(timer));timer.script_loaded=1;timer.variable_remaining=timer.variable_low=timer.variable_high=-1;
 return bh_script_timer_bind(&f->ctx,f->fired,&timer,1)&&bh_script_timer_bind(&f->ctx,f->next,&timer,1);
}
static void finish(Fixture *f){CHECK(bh_entities_dispose(&f->pool),"actual whole pool ownership disposed");CHECK(!f->memory.live,"all actual pool allocations released");}
static BHScriptWaitState initial(void){BHScriptWaitState s;memset(&s,0,sizeof(s));s.script_loaded=1;s.remaining=2;s.expected=345;s.variable_remaining=s.variable_expected=-1;s.retained14=0xdeadbeefUL;s.retained18=0x87654321UL;s.retained28=0xff000011UL;s.retained2c=0xfeedf00dUL;return s;}
static BHEntity *spawn(Fixture *f,BHScriptWaitState *s){BHScriptWaitPacket p;
 if(!bh_script_wait_bind(&f->ctx,f->wait,s))return NULL;p.event=8;p.actor=f->a;p.expected=999;f->wait->query(f->wait,&p);return f->pool.active;
}
static void dynamic_callbacks(void){
 Fixture f;BHScriptWaitState s,expected,*wait;BHScriptWaitPacket p,before;BHEntity *child,*timer_child;BHScriptTimerState *timer;unsigned long used,generation;BHEntity *q;int found=0;
 CHECK(fixture(&f),"actual pool, actor table and two admitted timer template queries");s=initial();s.fired=f.fired;s.next=f.next;
 CHECK(bh_script_wait_bind(&f.ctx,f.wait,&s),"bind real wait-template query");f.ctx.script_loading=7;
 p.event=8;p.actor=f.a;p.expected=12345;before=p;used=f.pool.used;f.wait->query(f.wait,&p);
 timer_child=f.pool.active;child=timer_child->next;wait=bh_script_wait_state(&f.ctx,child);timer=bh_script_timer_state(&f.ctx,timer_child);
 CHECK(wait&&timer&&f.pool.used==used+2,"wait registers child then real next-timer query creates actual timer child");
 if(!wait||!timer){finish(&f);return;}
 expected=s;expected.actor=f.a;expected.script_loaded=7;
 CHECK(!memcmp(wait,&expected,48),"all48 copied state bytes including retained fields, overridden actor/script flag");
 CHECK(!memcmp(&child->position,&f.a->position,sizeof(BHVec3)),"activation actor position copied to actual child");
 CHECK(timer->actor==f.a&&timer->script_loaded==7,"original activation forwarded to real next timer with actor/context");
 CHECK(!memcmp(&p,&before,sizeof(p)),"original extended activation packet unmodified");
 p.event=14;p.actor=f.a;p.expected=346;used=f.pool.used;child->query(child,&p);
 CHECK(wait->remaining==2&&f.pool.used==used,"matching actor alone cannot satisfy expected DWORD+8");
 p.actor=f.b;p.expected=345;child->query(child,&p);timer_child=f.pool.active;timer=bh_script_timer_state(&f.ctx,timer_child);
 CHECK(timer&&timer->actor==f.a&&f.pool.used==used+1,"matching expected with differing actor fires real timer using SAVED actor");
 CHECK(wait->remaining==1&&child->query==bh_script_wait_child&&!child->step,"first finite completion decrements without unregistering");
 child->query(child,&p);
 CHECK(wait->remaining==0&&!child->query&&child->step==bh_script_timer_delete_step(),"zero count unregisters then schedules SAME admitted delete owner");
 for(q=f.pool.query_head;q;q=q->query_next)if(q==child)found=1;
 CHECK(!found&&!bh_script_wait_state(&f.ctx,child),"completed child absent from actual query list and validated accessor");
 used=f.pool.used;generation=child->generation;
 CHECK(bh_entities_focus(&f.pool,child)&&bh_entities_step(&f.pool),"actual focused step invokes exposed original delete callback");
 CHECK(f.pool.used==used-1&&child->generation!=generation&&!f.pool.focused,"actual release invalidates borrowed child and clears focus");
 CHECK(!bh_script_wait_state(&f.ctx,child),"released wait state cannot be borrowed");finish(&f);
}
static void events_and_counts(void){
 Fixture f;BHScriptWaitState s,*state,snapshot;BHScriptWaitPacket p;BHEntity *child;unsigned long used;
 CHECK(fixture(&f),"independent real lifetime fixture");s=initial();s.remaining=0;child=spawn(&f,&s);state=bh_script_wait_state(&f.ctx,child);
 CHECK(state!=NULL,"literal wait activation without callbacks");if(!state){finish(&f);return;}
 p.event=14;p.actor=f.b;p.expected=s.expected;used=f.pool.used;child->query(child,&p);
 CHECK(!state->remaining&&child->query==bh_script_wait_child&&!child->step&&f.pool.used==used,"starting0 remains unlimited with NULL fired callback");
 snapshot=*state;p.event=99;child->query(child,&p);CHECK(!memcmp(state,&snapshot,48)&&f.pool.used==used,"irrelevant child event does not mutate48 state bytes or lifetime");
 p.event=10;p.actor=f.b;child->query(child,&p);CHECK(!child->step,"actor mismatch event10 ignored");
 p.actor=f.a;child->query(child,&p);CHECK(child->step==bh_script_timer_delete_step()&&child->query==bh_script_wait_child,"matching actor10 defers deletion without unregistering");
 CHECK(bh_entities_focus(&f.pool,child)&&bh_entities_step(&f.pool)&&f.pool.used==used-1,"matching-actor deferred actual deletion");
 s.remaining=LONG_MIN;child=spawn(&f,&s);state=bh_script_wait_state(&f.ctx,child);p.event=14;p.actor=f.b;p.expected=s.expected;child->query(child,&p);
 CHECK(state&&state->remaining==LONG_MAX&&child->query==bh_script_wait_child,"negative count decrements through explicit32-bit wrap");
 state->fired=f.a;state->remaining=2;used=f.pool.used;child->query(child,&p);
 CHECK(state->remaining==1&&f.pool.used==used,"nonnull fired with NULL query skips callback but decrements");
 p.event=20;state->script_loaded=1;child->query(child,&p);CHECK(f.pool.used==used&&child->query==bh_script_wait_child,"script-loaded child survives event20");
 state->script_loaded=0;child->query(child,&p);CHECK(f.pool.used==used-1&&!child->query,"unloaded child event20 immediately releases actual entity");
 child=spawn(&f,&s);used=f.pool.used;p.event=4;child->query(child,&p);
 CHECK(child->query==bh_script_wait_child&&child->step==bh_script_timer_delete_step()&&f.pool.used==used,"child event4 schedules deletion while retaining query");
 CHECK(bh_entities_focus(&f.pool,child)&&bh_entities_step(&f.pool)&&f.pool.used==used-1,"child event4 actual deferred release");
 p.event=4;used=f.pool.used;f.wait->query(f.wait,&p);
 CHECK(f.wait->query==bh_script_wait_template&&f.wait->step==bh_script_timer_delete_step()&&f.pool.used==used,"template event4 preserves query until actual step");
 CHECK(bh_entities_focus(&f.pool,f.wait)&&bh_entities_step(&f.pool)&&f.pool.used==used-1,"template actual shared deletion owner");finish(&f);
}
static void guards_and_failure(void){
 Fixture f;BHScriptWaitState s,*state,snapshot;BHScriptWaitPacket p;unsigned long used;BHEntity *child;
 CHECK(fixture(&f),"real allocation-failure fixture");s=initial();s.next=f.next;s.variable_remaining=21;s.variable_expected=1;
 f.actors[0].variables[1]=32767;CHECK(bh_script_wait_bind(&f.ctx,f.wait,&s),"signed-short variable selectors bound");
 while(f.pool.used<f.pool.capacity)CHECK(bh_entity_acquire(&f.pool)!=NULL,"fill real slots to require actual growth allocation");
 used=f.pool.used;f.memory.fail_at=f.memory.calls+1;p.event=8;p.actor=f.a;p.expected=0;f.wait->query(f.wait,&p);state=bh_script_wait_state(&f.ctx,f.wait);
 CHECK(f.ctx.error==BH_TIMER_POOL&&f.pool.used==used,"actual malloc growth failure publishes/forwards no child");
 CHECK(state&&state->remaining==-32768&&state->expected==32767,"template variable writes precede failed allocation and sign-extend");
 f.memory.fail_at=0;f.wait->query(f.wait,&p);child=f.pool.active->next;
 CHECK(f.pool.used==used+2&&bh_script_wait_state(&f.ctx,child),"retry creates wait and actual forwarded timer children");
 CHECK(bh_script_wait_state(&f.ctx,child)->remaining==-32768&&bh_script_wait_state(&f.ctx,child)->expected==32767,"resolved values cloned intact");
 state=bh_script_wait_state(&f.ctx,f.wait);state->variable_remaining=22;snapshot=*state;used=f.pool.used;
 f.wait->query(f.wait,&p);CHECK(f.ctx.error==BH_TIMER_ACTOR&&f.pool.used==used&&!memcmp(state,&snapshot,48),"invalid selector guard precedes all variable mutation/allocation");
 state->variable_remaining=-7;state->variable_expected=-1;state->next=f.b;used=f.pool.used;f.wait->query(f.wait,&p);
 CHECK(f.ctx.error==BH_TIMER_ARGUMENT&&f.pool.used==used+1,"missing next query guarded AFTER actual child publication");
 child=f.pool.active;CHECK(child->query==bh_script_wait_child,"missing-next-query guard does not delete completed activation child");
 used=f.pool.used;state=bh_script_wait_state(&f.ctx,f.wait);snapshot=*state;p.actor=NULL;
 f.wait->query(f.wait,&p);CHECK(f.ctx.error==BH_TIMER_ACTOR&&f.pool.used==used&&!memcmp(state,&snapshot,48),"NULL activation actor rejected before position/state mutation");
 CHECK(!bh_script_wait_bind(NULL,f.wait,&s)&&!bh_script_wait_bind(&f.ctx,NULL,&s),"source NULL binding guards");
 s=initial();s.variable_expected=22;CHECK(!bh_script_wait_bind(&f.ctx,f.wait,&s),"binding selector upper bound guard");
 p.event=20;p.actor=f.a;state->script_loaded=1;used=f.pool.used;f.wait->query(f.wait,&p);CHECK(f.pool.used==used,"loaded template survives event20");
 state->script_loaded=0;f.wait->query(f.wait,&p);CHECK(f.pool.used==used-1&&!f.wait->query,"unloaded template event20 immediately releases");finish(&f);
}
int __cdecl bh_script_wait_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
 dynamic_callbacks();events_and_counts();guards_and_failure();bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"script-wait\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;}
#ifdef BH_SCRIPT_WAIT_STANDALONE
int bh_script_timer_checkpoint(unsigned long *count);
int main(void){BHMathEnvironment fp;unsigned long count;int result;if(!bh_math_environment_enter(&fp))return 2;
 result=bh_script_wait_checkpoint(&count);result|=bh_script_timer_checkpoint(&count);bh_math_environment_leave(&fp);return result;}
#endif
