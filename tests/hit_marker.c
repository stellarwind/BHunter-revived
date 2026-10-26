#include "../src/bh_hit_marker.h"
#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<15)printf("FAIL hit-marker %lu: %s\n",checks,s);}}while(0)
static BHEntity *link(BHEntity*e,unsigned long a){BHEntity*p;memcpy(&p,(unsigned char*)e+a,4);return p;}
static float number(const BHEntity*e,unsigned long a){float f;memcpy(&f,(const unsigned char*)e+a,4);return f;}
static float word(const unsigned long*p){float f;memcpy(&f,p,4);return f;}
static void draw(BHEntity*e){BHHitMarkerDraw f;memcpy(&f,(unsigned char*)e+0xe8,4);CHECK(f==bh_hit_marker_draw,"actual installed draw identity");f(e);}
static void construction(void){BHEntityPool pool;BHDrawQueue q;BHHitMarkerContext c;BHEntity*owner,*e;BHVec3 p={1,2,3},n={1,0,0};BHDrawCommand*d;unsigned long i,packet=20;BHEntityPositionPacket pos;
 memset(&c,0,sizeof(c));CHECK(bh_entities_init(&pool,20,NULL),"real pool");CHECK(bh_draw_queue_init(&q,30,NULL),"real quad queue");owner=bh_entity_acquire(&pool);c.pool=&pool;c.queue=&q;c.image=73;
 for(i=0;i<4;++i){e=bh_hit_marker_create(&c,&p,&n,7,i,owner);CHECK(e!=NULL&&pool.used==i+2,"complete constructor acquires and registers real pooled entity");
 CHECK(number(e,0x100)==0&&number(e,0x104)==30&&number(e,0x108)==7,"original scalar fields");CHECK(number(e,0x10c)==(float)(i&1)*.5f&&number(e,0x110)==(float)(i&2)*.25f,"all four atlas quadrants");
 CHECK(e->query==bh_marker_query&&e->step==NULL&&e->refresh==NULL,"real query and verified RET callback sentinels");
 CHECK(link(owner,0x12c)==e&&link(e,0xf4)==owner&&link(e,0xf0)==NULL,"original owner newest insertion");
 CHECK(number(e,0x120)==0&&number(e,0x124)==-1&&number(e,0x128)==0,"perpendicular up chosen for side normal");draw(e);d=q.commands+9+i;
 CHECK(d->kind==BH_DRAW_QUAD&&d->resource==73&&word(d->words)==1&&word(d->words+1)==2&&word(d->words+2)==3,"actual quad producer keeps resource and position");
 CHECK(word(d->words+3)==0&&word(d->words+4)==-1&&word(d->words+6)==1&&word(d->words+7)==0,"normal/up argument order");
 CHECK(word(d->words+9)==7&&word(d->words+10)==7&&word(d->words+11)==0&&word(d->words+12)==1&&d->words[13]==0,"exact size angle modulation flags");
 CHECK(word(d->words+14)==(float)(i&1)*.5f&&word(d->words+15)==.5f&&word(d->words+16)==(float)(i&2)*.25f&&word(d->words+17)==.5f,"exact atlas start/extent pairs");}
 pos.event=3;memset(&pos.position,0,12);e->query(e,&pos);CHECK(!memcmp(&pos.position,&p,12),"registered position query");
 e->query(e,&packet);draw(e);CHECK(number(e,0x108)==0&&q.count==13&&c.count==4,"event20 hides draw without erasing list lifetime");
 CHECK(bh_hit_marker_dispose(&c)&&pool.used==1&&link(owner,0x12c)==NULL,"source adapter unlinks both real lists before release");CHECK(bh_entities_valid(&pool),"real entity/query rings valid after dispose");
 CHECK(bh_draw_queue_dispose(&q)&&bh_entities_dispose(&pool),"actual owners disposed");
}
static void normal_boundaries(void){BHEntityPool pool;BHDrawQueue q;BHHitMarkerContext c;BHEntity*e,*owner;BHVec3 p={0,0,0},n={0,0,0};unsigned long i,quiet=0x7fc00000UL;const float ys[]={-.50001f,-.5f,-.49999f,0,.49999f,.5f,.50001f};
 memset(&c,0,sizeof(c));CHECK(bh_entities_init(&pool,10,NULL)&&bh_draw_queue_init(&q,9,NULL),"normal fixture real pool/full queue");owner=bh_entity_acquire(&pool);c.pool=&pool;c.queue=&q;
 for(i=0;i<8;++i){if(i==7)memcpy(&n.y,&quiet,4);else n.y=ys[i];e=bh_hit_marker_create(&c,&p,&n,1,0,owner);CHECK(e!=NULL,"whole constructor normal boundary");CHECK(number(e,0x124)==((i>=2&&i<=4)?-1:0)&&number(e,0x128)==((i>=2&&i<=4)?0:1),"strict half thresholds and unordered fallback");}
 draw(e);CHECK(c.error==BH_HIT_MARKER_QUEUE&&q.count==9,"draw queue exhaustion preserved");
 n.y=0;memcpy((unsigned char*)e+0x108,&quiet,4);draw(e);CHECK(q.count==9,"unordered size skips queue");
 CHECK(bh_hit_marker_dispose(&c)&&bh_entities_valid(&pool),"dispose normal fixture");bh_draw_queue_dispose(&q);bh_entities_dispose(&pool);
}
static void fifo(void){BHEntityPool pool;BHDrawQueue q;BHHitMarkerContext c;BHEntity *owner[3],*e,*entries[200],*at;BHVec3 p={0,0,0},n={0,1,0};unsigned long i,j,count,generation;
 memset(&c,0,sizeof(c));CHECK(bh_entities_init(&pool,204,NULL)&&bh_draw_queue_init(&q,10,NULL),"FIFO real pool");for(i=0;i<3;++i)owner[i]=bh_entity_acquire(&pool);c.pool=&pool;c.queue=&q;
 for(i=0;i<200;++i){p.x=(float)i;entries[i]=bh_hit_marker_create(&c,&p,&n,1,0,owner[i%3]);CHECK(entries[i]!=NULL&&c.count==i+1,"global FIFO fills to exact200");}
 generation=entries[0]->generation;e=bh_hit_marker_create(&c,&p,&n,1,0,owner[2]);CHECK(e&&c.count==200&&pool.used==203&&c.oldest==entries[1]&&c.newest==e,"201st constructor evicts global oldest after acquiring new entity");
 CHECK(link(c.oldest,0xf8)==NULL&&link(e,0xf8)==entries[199]&&link(entries[199],0xfc)==e,"FIFO reciprocal links repaired");
 CHECK(entries[0]->generation==generation+1&&pool.free_slots==entries[0],"actual oldest entity returned to real pool");
 for(j=0;j<3;++j){count=0;at=link(owner[j],0x12c);while(at&&count<201){CHECK(link(at,0xf4)==owner[j],"owner identity retained across eviction");if(link(at,0xec))CHECK(link(link(at,0xec),0xf0)==at,"owner reciprocal links retained");at=link(at,0xec);++count;}CHECK(count==(j==0?66:j==1?67:67),"each distinct owner list retains exact membership after head/middle unlink");}
 for(i=0;i<401;++i){e=bh_hit_marker_create(&c,&p,&n,1,0,owner[1]);CHECK(e&&c.count==200&&pool.used==203,"recycled stale payload does not alter bounded FIFO");}
 CHECK(bh_entities_valid(&pool),"real pool/query membership survives repeated eviction");CHECK(bh_hit_marker_dispose(&c)&&pool.used==3,"all200 FIFO entities released once");for(i=0;i<3;++i)CHECK(link(owner[i],0x12c)==NULL,"all owner lists empty");bh_draw_queue_dispose(&q);bh_entities_dispose(&pool);
}
typedef struct Memory{int fail;unsigned long allocated,released;}Memory;
static void*__cdecl alloc(unsigned long n,void*u){Memory*m=u;void*p;if(m->fail)return NULL;p=malloc(n);if(p)++m->allocated;return p;}
static void __cdecl release(void*p,void*u){Memory*m=u;++m->released;free(p);}
static void failures_case(void){BHEntityPool pool;BHDrawQueue q;BHHitMarkerContext c;BHSceneAllocator a;Memory m;BHEntity*owner;BHVec3 p={0,0,0};
 memset(&c,0,sizeof(c));memset(&m,0,sizeof(m));a.allocate=alloc;a.release=release;a.user=&m;CHECK(bh_entities_init(&pool,1,&a)&&bh_draw_queue_init(&q,9,NULL),"actual controlled allocation fixture");owner=bh_entity_acquire(&pool);c.pool=&pool;c.queue=&q;m.fail=1;
 CHECK(!bh_hit_marker_create(&c,&p,&p,1,0,owner)&&c.error==BH_HIT_MARKER_POOL,"real pool growth failure");CHECK(c.count==0&&!c.oldest&&!c.newest&&!link(owner,0x12c)&&pool.used==1,"allocation failure leaves both lists unmodified");
 CHECK(!bh_hit_marker_create(&c,NULL,&p,1,0,owner)&&c.error==BH_HIT_MARKER_ARGUMENT,"explicit source boundary guard");bh_draw_queue_dispose(&q);bh_entities_dispose(&pool);CHECK(m.allocated==m.released,"actual failed fixture has balanced ownership");
}
int __cdecl bh_hit_marker_checkpoint(unsigned long*count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;construction();normal_boundaries();fifo();failures_case();bh_math_environment_leave(&fp);if(count)*count=checks;printf("{\"scenario\":\"hit-marker\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_HIT_MARKER_STANDALONE
int main(void){unsigned long count;return bh_hit_marker_checkpoint(&count);}
#endif
