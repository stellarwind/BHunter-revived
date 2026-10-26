#include "../src/bh_trail.h"
#include "../src/bh_callback_helpers.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL trail %s\n",n);}}while(0)
static void number(BHEntity *e,unsigned long offset,float value){memcpy((unsigned char*)e+offset,&value,4);}
static float scalar(BHEntity *e,unsigned long offset){float v;memcpy(&v,(unsigned char*)e+offset,4);return v;}
static BHEntity *link(BHEntity *e,unsigned long offset){BHEntity *v;memcpy(&v,(unsigned char*)e+offset,4);return v;}
static float word(const unsigned long *w){float f;memcpy(&f,w,4);return f;}
/* Fixture attachment implements an actual event3 location response. It is
   isolated test behavior, never a game implementation or admission. */
static void attachment(BHEntity *e,void *raw){BHEntityPositionPacket *p=raw;if(p->event==3)p->position=e->position;}
int bh_trail_checkpoint(unsigned long *count) {
    BHEntityPool pool;BHDrawQueue queue;BHTrailContext c;BHTrailParameters p;
    BHEntity *generator,*first,*second,*third,*actor,*created,*pair_a,*pair_b;BHDrawCommand *header;
    BHVec3 position={1,2,3},velocity={30,60,90};struct {long event;BHEntity *actor;} removed;
    checks=0;failures=0;memset(&pool,0,sizeof(pool));memset(&queue,0,sizeof(queue));memset(&c,0,sizeof(c));memset(&p,0,sizeof(p));
    CHECK(bh_entities_init(&pool,16,NULL)&&bh_draw_queue_init(&queue,32,NULL),"real pool and command arena");c.pool=&pool;c.queue=&queue;
    generator=bh_entity_acquire(&pool);p.bucket=3;p.image=77;p.point_word=0xabcdef01;p.lifetime=-1;p.decay=.25f;p.interval=.1f;p.node_lifetime=1;p.width=2;p.width_step=.5f;p.texture_scale=.75f;p.drift.x=1;p.drift.y=2;p.drift.z=3;
    CHECK(bh_trail_bind(&c,generator,&p),"explicit real generator state binding");bh_trail_generate(generator);first=link(generator,0xf8);
    CHECK(first&&first->step==bh_trail_node_tick&&first->refresh==bh_trail_refresh&&c.emitted==1,"whole generator installs recovered callbacks");
    CHECK(scalar(first,0xfc)==0&&scalar(first,0xf0)==1&&scalar(first,0x100)==.25f,"first node original scalar initialization");
    bh_trail_refresh(first,NULL);CHECK(queue.count==9,"single node original no path command");
    bh_trail_generate(generator);CHECK(c.emitted==1,"interval gate no node");
    generator->position.x=3;generator->position.y=4;number(generator,0x110,0);bh_trail_generate(generator);second=link(generator,0xf8);
    CHECK(second&&second!=first&&link(second,0xec)==first&&link(first,0xe8)==second&&!first->refresh,"real doubly linked trail and refresh transfer");
    CHECK(scalar(second,0xfc)==5,"actual distance accumulation");
    bh_trail_refresh(second,NULL);header=queue.commands+9;
    CHECK(queue.count==12&&header->kind==BH_DRAW_PATH_HEADER&&header->resource==77&&header->words[0]==2,"actual header plus contiguous path points");
    CHECK(queue.commands[10].kind==BH_DRAW_PATH_POINT&&word(&queue.commands[10].resource)==3&&word(queue.commands[10].words)==4&&word(queue.commands[10].words+2)==2,"actual position and width payload");
    CHECK(word(queue.commands[10].words+3)==0&&word(queue.commands[11].words+3)==0&&queue.commands[10].words[6]==0xabcdef01,"original endpoint alpha zero and raw extra word");
    CHECK(header->next==10&&queue.commands[10].next==11,"real queue chain");
    bh_trail_node_tick(second);CHECK(second->position.x==4&&second->position.y==6&&second->position.z==3&&scalar(second,0xf0)==.75f&&scalar(second,0xf4)==2.5f,"node lifetime drift width update");
    actor=bh_entity_acquire(&pool);actor->query=attachment;actor->position.x=20;actor->position.y=30;actor->position.z=40;
    /* Retain current chain while binding the real attachment to existing
       generator source words. Constructor recovery is not implied. */
    memcpy((unsigned char*)generator+0xf4,&actor,4);memcpy((unsigned char*)generator+0xe8,&actor->generation,4);
    number(generator,0x110,0);bh_trail_generate(generator);third=link(generator,0xf8);
    CHECK(third&&third!=second&&third->position.x==20&&third->position.y==30&&third->position.z==40,"actual query event3 follows attachment");
    ++actor->generation;number(generator,0x110,0);bh_trail_generate(generator);CHECK(!link(generator,0xf4),"generation mismatch clears attachment");
    number(second,0xf0,.125f);bh_trail_node_tick(second);CHECK(link(third,0xec)==first&&link(first,0xe8)==third&&pool.used==5,"expired middle node unlinks both neighbors and real pool release");
    number(first,0xf0,.25f);bh_trail_node_tick(first);CHECK(scalar(first,0xf0)==0,"zero remaining node lifetime survives");
    bh_trail_node_tick(first);CHECK(!link(third,0xec)&&pool.used==4,"strictly negative lifetime releases tail");
    number(generator,0xfc,.01f);bh_trail_generate(generator);CHECK(pool.used==3,"positive generator lifetime expires with real release");
    created=bh_trail_create(&c,81,3,0x12345678,&position,actor,6,4,10,2,3,15,&velocity,.75f);
    CHECK(created&&created->query==bh_effect_target_removed&&created->step==bh_trail_generate&&created->position.x==1,"whole constructor real query registration and position");
    CHECK(scalar(created,0x100)==(float)(1.0/60.0)&&scalar(created,0x104)==.1f&&scalar(created,0x114)==.5f,"constructor decrement interval and width rate");
    CHECK(scalar(created,0x120)==1&&scalar(created,0x124)==2&&scalar(created,0x128)==3,"constructor scales drift by original double tick");
    removed.event=17;removed.actor=actor;created->query(created,&removed);
    CHECK(!link(created,0xf4)&&scalar(created,0xfc)==0,"actual existing attachment removal callback");
    CHECK(bh_trail_create_pair(&c,82,3,0x10,&position,NULL,6,4,10,2,3,15,&velocity,.75f,&pair_a,&pair_b)==2,"whole paired constructor creates both real generators");
    CHECK(pair_a&&pair_b&&scalar(pair_a,0x120)==1&&scalar(pair_b,0x120)==1.7f&&scalar(pair_a,0x108)==scalar(pair_b,0x108),"paired staged velocity and equal scaled node values");
    CHECK(!bh_trail_create(&c,81,3,0,&position,NULL,6,0,10,2,3,15,&velocity,.75f),"zero source fade denominator guard");
    CHECK(bh_draw_queue_dispose(&queue)&&bh_entities_dispose(&pool),"real resources dispose");
    if(count)*count+=checks;printf("{\"scenario\":\"trail\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_TRAIL_STANDALONE
int main(void){unsigned long n=0;return bh_trail_checkpoint(&n);}
#endif
