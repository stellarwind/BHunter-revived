#include "../src/bh_entities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<=12)printf("FAIL entity line %d: %s\n",line,label);}}
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long size,void *user){Memory *m=(Memory*)user;unsigned long *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=(unsigned long*)malloc(size+20);if(!p)return NULL;p[0]=size;p[1]=0x7812abcdUL;memset((unsigned char*)p+16,0xa5,size);memcpy((unsigned char*)p+16+size,&p[1],4);++m->live;return (unsigned char*)p+16;}
static void __cdecl release(void *pointer,void *user){Memory *m=(Memory*)user;unsigned long *p=(unsigned long*)((unsigned char*)pointer-16),guard;memcpy(&guard,(unsigned char*)pointer+p[0],4);CHECK(m->live && p[1]==0x7812abcdUL && guard==p[1],"entity chunk allocation guards/release");--m->live;free(p);}
static BHSceneAllocator hooks(Memory *m){BHSceneAllocator a;a.allocate=allocate;a.release=release;a.user=m;return a;}
static BHEntityPool *current;
static BHEntity *trace[32],*future,*newborn;
static unsigned long calls,event3;static int mutation_mode,busy_rejected;
static long user_packet=7;
static void __cdecl refresh(BHEntity *entity,void *packet);
static void __cdecl callback(BHEntity *entity,void *packet){
    if(packet!=&user_packet && packet){BHEntityPositionPacket *p=(BHEntityPositionPacket*)packet;if(p->event==3){++event3;p->position.x=0;p->position.y=0;p->position.z=0;return;}}
    if(calls<32)trace[calls]=entity;++calls;
}
static void __cdecl other_callback(BHEntity *entity,void *packet){callback(entity,packet);}
static void __cdecl step(BHEntity *entity){
    if(calls<32)trace[calls]=entity;++calls;
    if(mutation_mode==1)CHECK(bh_entity_release(current,entity),"step self release");
    if(mutation_mode==2 && !newborn){newborn=bh_entity_acquire(current);CHECK(newborn!=NULL,"step creates new head");if(newborn)newborn->step=step;}
    if(mutation_mode==3){mutation_mode=0;CHECK(bh_entity_release(current,future),"step releases cached future");}
    if(mutation_mode==4){busy_rejected=!bh_entities_dispose(current) && current->error==BH_ENTITY_BUSY;}
    if(mutation_mode==5){mutation_mode=0;CHECK(bh_entity_release(current,future),"release successor for reuse");newborn=bh_entity_acquire(current);CHECK(newborn==future,"real pool reuses cached successor address");newborn->step=step;newborn->refresh=refresh;}
}
static void __cdecl refresh(BHEntity *entity,void *packet){CHECK(packet==NULL,"original refresh second argument zero");step(entity);}
static void __cdecl nesting(BHEntity *entity,void *packet){++calls;if(calls<18 && !bh_entities_query(current,0,0,NULL,0,&user_packet))busy_rejected=current->error==BH_ENTITY_BUSY;}

static void allocation(void){
    BHEntityPool pool,other;BHEntity *a,*b,*g,*r;Memory memory={0,0,0};BHSceneAllocator allocator=hooks(&memory);unsigned long generation;unsigned char *base;
    CHECK(bh_entities_init(&pool,2,&allocator) && bh_entities_valid(&pool) && pool.capacity==2,"initial arena/free list");
    pool.origin.x=11;pool.origin.y=-3;pool.origin.z=7;
    a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);CHECK(a && b && (unsigned char*)b-(unsigned char*)a==864,"initial slot address stride/order");
    CHECK(a->generation==1 && b->generation==1 && a->flags==0 && a->kind==0 && !a->step && !a->refresh && !a->query && a->lifetime==-1 && a->scale==1,"initial zeroing and exact acquire defaults");
    CHECK(a->origin.x==11 && a->origin.y==-3 && a->origin.z==7,"shared origin copied from explicit owner state");
    memory.fail_at=memory.calls+1;CHECK(!bh_entity_acquire(&pool) && pool.error==BH_ENTITY_MEMORY && pool.capacity==2 && pool.used==2 && bh_entities_valid(&pool),"growth allocation failure leaves original arena intact");memory.fail_at=0;
    g=bh_entity_acquire(&pool);CHECK(g && pool.capacity==10 && pool.used==3 && g->generation==0,"growth adds floor(capacity/8)+8 and returns last slot without generation increment");
    CHECK(g->payload[0]==0xa5 && g->flags==0xa5a5a5a5UL && g->kind==0,"growth payload selectively initialized, never silently zeroed");
    base=(unsigned char*)g-7*864;r=bh_entity_acquire(&pool);CHECK((unsigned char*)r==base+6*864 && r->generation==1,"remaining growth slots queued in reverse order");
    a->payload[5]=0x71;a->flags=0x4321;generation=a->generation;
    CHECK(bh_entity_release(&pool,a) && a->generation==generation+1 && pool.free_slots==a,"release generation and LIFO recycle");
    CHECK(bh_entity_acquire(&pool)==a && a->generation==generation+2 && a->payload[5]==0x71 && a->flags==0x4321,"recycled slot retains payload/flags and increments generation again");
    CHECK(bh_entities_init(&other,1,NULL),"other owner init");
    CHECK(!bh_entity_release(&other,a) && other.error==BH_ENTITY_ARGUMENT,"cross-pool release rejected");
    CHECK(bh_entity_release(&pool,b) && !bh_entity_release(&pool,b) && pool.error==BH_ENTITY_ARGUMENT,"double release rejected");
    CHECK(!bh_entity_release(&pool,(BHEntity*)((unsigned char*)a+1)),"interior pointer rejected");
    CHECK(bh_entities_valid(&pool),"active/free ownership remains valid");
    bh_entities_dispose(&other);CHECK(bh_entities_dispose(&pool) && !memory.live && bh_entities_valid(&pool),"all growth chunks released");
    CHECK(!bh_entities_init(&pool,0,&allocator) && pool.error==BH_ENTITY_LIMIT,"zero-capacity unsafe original case rejected");bh_entities_dispose(&pool);
}

static void lists_and_dispatch(void){
    BHEntityPool pool;BHEntity *a,*b,*c;current=&pool;CHECK(bh_entities_init(&pool,8,NULL),"dispatch arena");
    a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);c=bh_entity_acquire(&pool);a->flags=b->flags=c->flags=3;
    CHECK(bh_entity_set_query(&pool,c,callback) && bh_entity_set_query(&pool,a,callback),"query registration order");
    calls=0;CHECK(bh_entities_query(&pool,3,3,NULL,0,&user_packet) && calls==1 && trace[0]==a,"query starts registered head but walks active chain, not registration chain");
    CHECK(bh_entity_set_query(&pool,a,other_callback) && pool.query_head==a && a->query_next==c,"replacement callback preserves list position");
    CHECK(bh_entity_set_query(&pool,a,NULL) && bh_entity_set_query(&pool,c,NULL) && !pool.query_head && bh_entities_valid(&pool),"detach query head and tail");
    bh_entity_set_query(&pool,a,callback);bh_entity_set_query(&pool,b,callback);bh_entity_set_query(&pool,c,callback);
    calls=0;CHECK(bh_entities_query(&pool,3,3,NULL,-1,&user_packet) && calls==3 && trace[0]==c && trace[1]==b && trace[2]==a,"unbounded masked query dispatch order");
    calls=0;CHECK(bh_entities_query(&pool,4,4,NULL,0,&user_packet) && !calls,"mask/value filtering");
    bh_entity_set_query(&pool,b,NULL);bh_entity_set_query(&pool,b,callback);CHECK(bh_entities_valid(&pool) && pool.query_head==b && !b->query_previous,"re-enable detached nonhead with consistent backlinks");
    a->step=b->step=c->step=step;a->refresh=b->refresh=c->refresh=refresh;
    calls=0;mutation_mode=0;CHECK(bh_entities_step(&pool) && calls==3 && trace[0]==c && trace[2]==a,"active newest-first step traversal");
    calls=0;CHECK(bh_entities_focus(&pool,b) && bh_entities_step(&pool) && calls==1 && trace[0]==b,"focused step does not run other slots");bh_entities_focus(&pool,NULL);
    calls=0;CHECK(bh_entities_refresh(&pool) && calls==3,"all refresh callbacks and zero packet");
    calls=0;newborn=NULL;mutation_mode=2;CHECK(bh_entities_step(&pool) && calls==3 && newborn && pool.active==newborn,"new head created during traversal waits until next pass");
    mutation_mode=4;busy_rejected=0;calls=0;CHECK(bh_entities_step(&pool) && busy_rejected && pool.capacity==8,"callback cannot dispose live arena");
    mutation_mode=1;calls=0;CHECK(bh_entities_step(&pool) && calls==4 && !pool.used && bh_entities_valid(&pool),"self release preserves cached active successors");
    mutation_mode=0;a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);c=bh_entity_acquire(&pool);a->step=b->step=c->step=step;
    future=b;mutation_mode=3;calls=0;CHECK(!bh_entities_step(&pool) && pool.error==BH_ENTITY_MUTATION && calls==1 && bh_entities_valid(&pool),"future-slot lifetime change stops traversal explicitly");
    CHECK(pool.failure_count==1 && pool.failure_current==c && pool.failure_next==b && pool.failure_callback==(unsigned long)step && !pool.failure_refresh && !pool.failure_next_active && pool.failure_actual_generation==pool.failure_expected_generation+1,"mutation diagnostic identifies callback and released successor");
    CHECK(bh_entities_refresh(&pool) && pool.failure_count==1 && pool.failure_next==b,"refresh preserves earlier step failure evidence");
    mutation_mode=0;bh_entity_set_query(&pool,c,nesting);calls=0;busy_rejected=0;
    CHECK(bh_entities_query(&pool,0,0,NULL,0,&user_packet) && busy_rejected && pool.dispatch_depth==0,"bounded nested queries unwind safely");
    bh_entities_dispose(&pool);
}
static void recycled_successor(void){
    BHEntityPool pool;BHEntity *a,*b,*c;unsigned long pass;
    for(pass=0;pass<2;++pass){
        current=&pool;CHECK(bh_entities_init(&pool,8,NULL),"recycle traversal arena");
        a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);c=bh_entity_acquire(&pool);
        a->step=b->step=c->step=step;a->refresh=b->refresh=c->refresh=refresh;
        future=b;newborn=NULL;mutation_mode=5;calls=0;
        CHECK(pass?bh_entities_refresh(&pool):bh_entities_step(&pool),"active recycled successor continues original saved-pointer traversal");
        CHECK(calls==4&&trace[0]==c&&trace[1]==b&&trace[2]==c&&trace[3]==a,"original +e0 loads visit replacement then relinked current then tail");
        CHECK(b->generation==3&&pool.error==BH_ENTITY_OK&&bh_entities_valid(&pool),"recycled generation is valid and pool remains consistent");
        bh_entities_dispose(&pool);
    }
}
typedef struct QueryCase {unsigned long words[7];int hit;} QueryCase;
static const QueryCase query_cases[]={
#include "entity_query_cases.h"
};
static float from_word(unsigned long word){float value;memcpy(&value,&word,4);return value;}
static void spatial(void){
    BHEntityPool pool;BHEntity *e;BHVec3 center={0,0,0};unsigned long i,j,control;float values[7];
    current=&pool;bh_entities_init(&pool,1,NULL);e=bh_entity_acquire(&pool);e->flags=1;bh_entity_set_query(&pool,e,callback);
    for(i=0;i<sizeof(query_cases)/sizeof(query_cases[0]);i++){
        for(j=0;j<7;j++)values[j]=from_word(query_cases[i].words[j]);
        memcpy(&center,values,12);memcpy(&e->position,values+3,12);calls=0;
        CHECK(bh_entities_query(&pool,1,1,&center,values[6],&user_packet) && calls==(unsigned long)query_cases[i].hit,"independent PC53 radius boundary control");
    }
    center.x=center.y=center.z=0;e->position.x=3;e->position.y=4;e->position.z=0;calls=0;
    CHECK(bh_entities_query(&pool,1,1,&center,5,&user_packet) && calls==1,"inclusive 3-4-5 boundary");
    e->position.x=e->position.y=e->position.z=100;e->flags=0x101;event3=0;calls=0;
    CHECK(bh_entities_query(&pool,1,1,&center,1,&user_packet) && calls==1 && event3==1 && e->position.x==0,"event-3 callback position writeback occurs before spatial test");
    calls=0;CHECK(bh_entities_query(&pool,0,0,NULL,from_word(0x7fc12345UL),&user_packet) && calls==1,"unordered radius takes observed nonspatial branch");
    CHECK(!bh_entities_query(&pool,0,0,NULL,1,&user_packet) && pool.error==BH_ENTITY_ARGUMENT,"positive radius requires center");
    control=_control87(0,0);_control87(_PC_24,_MCW_PC);CHECK(!bh_entities_query(&pool,0,0,&center,1,&user_packet) && pool.error==BH_ENTITY_NUMERICAL,"spatial precision precondition");_control87(control,_MCW_PC);
    bh_entities_dispose(&pool);
}
static void capacity_and_real(const char *archive){
    BHEntityPool pool;BHEntity *entity;unsigned long i;BHSceneLevel level;BHVFile *file;
    CHECK(bh_entities_init(&pool,65535,NULL),"maximum arena capacity");
    for(i=0;i<65535;i++)if(!bh_entity_acquire(&pool))break;
    CHECK(i==65535 && pool.used==65535 && !bh_entity_acquire(&pool) && pool.error==BH_ENTITY_LIMIT && bh_entities_valid(&pool),"maximum occupancy and bounded growth rejection");bh_entities_dispose(&pool);
    if(!archive)return;
    bh_scene_level_init(&level,NULL);CHECK(bh_vfile_mount(archive),"mount real placement data for pool scenario");file=bh_vfile_open("TEST.LEV","rb");
    CHECK(file && bh_scene_level_read(&level,file),"decode real placements for ownership scenario");if(file)bh_vfile_close(file);
    CHECK(bh_entities_init(&pool,9600,NULL),"observed bootstrap 9600-slot initial arena");
    for(i=0;i<level.placement_count;i++){
        entity=bh_entity_acquire(&pool);CHECK(entity!=NULL,"slot acquired for each real placement record");if(!entity)break;
        entity->position.x=(float)level.placements[i].x;entity->position.y=(float)level.placements[i].y;entity->position.z=(float)level.placements[i].z;entity->flags=level.placements[i].flags;
    }
    CHECK(i==5815 && pool.used==5815 && pool.capacity==9600 && bh_entities_valid(&pool),"all 5815 real records fit observed arena without invented gameplay callbacks");
    while(pool.active)CHECK(bh_entity_release(&pool,pool.active),"recycle every real placement slot");
    CHECK(!pool.used && bh_entities_valid(&pool),"real-data release leaves complete valid free arena");
    printf("{\"scenario\":\"entities-real-data\",\"initial_capacity\":9600,\"placement_slots\":%lu,\"gameplay_callbacks_installed\":false}\n",i);
    bh_entities_dispose(&pool);bh_scene_level_dispose(&level);bh_vfile_shutdown();
}
int bh_entities_checkpoint(const char *archive,unsigned long *total){checks=0;failures=0;allocation();lists_and_dispatch();recycled_successor();spatial();capacity_and_real(archive);*total=checks;printf("{\"scenario\":\"entity-pool\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;}
