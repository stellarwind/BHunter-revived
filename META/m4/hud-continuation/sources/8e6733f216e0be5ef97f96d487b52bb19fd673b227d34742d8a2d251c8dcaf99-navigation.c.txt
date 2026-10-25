#include "../src/bh_navigation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL navigation line %d: %s\n",line,label);}}
static void ring(BHWorldGeometry *p,unsigned long count) {
    unsigned long i;memset(p,0,count*sizeof(*p));
    for(i=0;i<count;++i){p[i].navigation.next=&p[(i+1)%count].navigation;p[i].minimum.x=p[i].maximum.x=(float)i;}
}
static void link(BHWorldGeometry *a,unsigned long slot,BHWorldGeometry *b){a->navigation.portals[slot].neighbor=&b->navigation;}
static void chain(BHWorldGeometry *p) {
    ring(p,4);p[3].minimum.x=p[3].maximum.x=4;
    link(p,0,p+1);link(p+1,0,p);link(p+1,1,p+2);link(p+2,0,p+1);link(p+2,1,p+3);link(p+3,0,p+2);
}
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long size,void *user){Memory *m=(Memory*)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(size);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory*)user;CHECK(m->live>0,"balanced navigation ownership");--m->live;free(p);}
static int cache_file(const char *path,unsigned long count,int truncated) {
    FILE *f=fopen(path,"wb");BHSceneRoute cell={5,123};int ok;
    if(!f)return 0;
    ok=fwrite(&count,1,truncated?2:4,f)==(unsigned long)(truncated?2:4);
    if(!truncated && count==2)ok=ok && fwrite(&cell,8,1,f)==1 && fwrite(&cell,8,1,f)==1 && fwrite(&cell,8,1,f)==1 && fwrite(&cell,8,1,f)==1;
    return fclose(f)==0 && ok;
}
static void basics(void) {
    BHWorldGeometry p[5];BHNavigation n;BHNavigationNode *old_nodes;BHSceneRoute *old_cells;
    Memory memory={0,0,0};BHSceneAllocator allocator;unsigned long allocation,live,word;BHVFile *cache;
    allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
    CHECK(bh_navigation_init(&n,&allocator),"navigation owner initialized");chain(p);
    CHECK(bh_navigation_build(&n,p,4,NULL) && n.node_count==4 && n.junction_count==2,"junction-first chain construction");
    CHECK(n.nodes[0].placement==p && n.nodes[1].placement==p+3 && n.nodes[2].placement==p+1 && n.nodes[3].placement==p+2,"two original scene-order passes");
    CHECK(bh_navigation_find(&n,p+2)==3 && bh_navigation_find(&n,p+4)==-1,"source index lookup");
    CHECK(n.routes.cells[2].exit_slot==0 && n.routes.cells[2].distance==4 && n.routes.cells[1].distance==4,"corridor distance unwind and local first exit");
    CHECK(n.nodes[2].junction_first_port==0 && n.nodes[2].junction_second_port==1 && n.nodes[2].endpoints_valid==3,"corridor endpoint port-order metadata");
    CHECK(n.routes.cells[0].exit_slot==-1 && n.routes.cells[0].distance==0,"untouched diagonal exit remains -1");
    old_nodes=n.nodes;old_cells=n.routes.cells;live=memory.live;
    for(allocation=1;allocation<=3;++allocation) {
        memory.fail_at=memory.calls+allocation;
        CHECK(!bh_navigation_build(&n,p,4,NULL) && n.error==BH_NAV_MEMORY && n.nodes==old_nodes && n.routes.cells==old_cells && memory.live==live,"every owned allocation failure rolls back");
    }
    memory.fail_at=0;link(p,0,p+4);
    CHECK(!bh_navigation_build(&n,p,4,NULL) && n.error==BH_NAV_TOPOLOGY && n.nodes==old_nodes && memory.live==live,"foreign portal target rejects without dereference/graph replacement");
    chain(p);p[3].navigation.next=&p[1].navigation;
    CHECK(!bh_navigation_build(&n,p,4,NULL) && n.nodes==old_nodes,"malformed ring rejected");
    ring(p,4);link(p,0,p+1);link(p+1,0,p+2);link(p+1,1,p+3);link(p+2,0,p+1);link(p+2,1,p+3);link(p+3,0,p+1);link(p+3,1,p+2);
    CHECK(!bh_navigation_build(&n,p,4,NULL) && n.error==BH_NAV_CYCLE && n.nodes==old_nodes && memory.live==live,"reachable directed corridor cycle rejects boundedly");
    ring(p,3);link(p,0,p+1);link(p,1,p+2);link(p+1,0,p);link(p+1,1,p+2);link(p+2,0,p+1);link(p+2,1,p);
    CHECK(bh_navigation_build(&n,p,3,NULL) && n.node_count==3 && n.junction_count==0 && !n.routes.cells,"pure degree-two ring retains nodes without junction table");
    ring(p,4);link(p,0,p+1);link(p+1,0,p);link(p+2,0,p+3);link(p+3,0,p+2);
    CHECK(bh_navigation_build(&n,p,4,NULL) && n.junction_count==4,"disconnected junction components");
    memcpy(&word,&n.routes.cells[2*4].distance,4);CHECK(word==0x7e876abdUL && n.routes.cells[2*4].exit_slot==-1,"unreachable sentinel retained exactly");
    p[3].owner=p+2;CHECK(!bh_navigation_build(&n,p,4,NULL) && n.error==BH_NAV_TOPOLOGY,"neighbors of excluded owned entities reject rather than guess an index");
    ring(p,4);p[0].minimum.x=p[0].maximum.x=0;p[1].minimum.x=p[1].maximum.x=1;
    p[2].minimum.x=p[2].maximum.x=0;p[2].minimum.y=p[2].maximum.y=1;
    p[3].minimum.x=p[3].maximum.x=10;
    link(p,0,p+1);link(p,1,p+2);link(p,2,p+3);link(p+1,0,p);link(p+1,1,p+2);link(p+2,0,p+1);link(p+2,1,p);link(p+3,0,p);
    CHECK(bh_navigation_build(&n,p,4,NULL) && n.junction_count==2 && n.routes.cells[0].exit_slot==1 && n.routes.cells[0].distance>3 && n.routes.cells[0].distance<4,"corridor loop preserves nonzero diagonal and last direct slot");
    ring(p,2);link(p,0,p+1);link(p,1,p+1);link(p+1,0,p);
    CHECK(bh_navigation_build(&n,p,2,NULL) && n.junction_count==1 && n.routes.cells[0].distance==0 && n.routes.cells[0].exit_slot==0 && !n.nodes[1].endpoints_valid,"duplicate return portals preserve no-forward-neighbor terminal contract");
    chain(p);CHECK(cache_file("bhnav-cache.tmp",2,0),"write explicit cache data fixture");cache=bh_vfile_open("bhnav-cache.tmp","rb");
    CHECK(cache && bh_navigation_build(&n,p,4,cache) && n.cache_used && n.routes.cells[0].exit_slot==5 && n.routes.cells[0].distance==123,"matching cache replaces computed cells without diagonal repair");
    if(cache){CHECK(bh_vfile_tell(cache)==36,"matching cache cursor consumed exactly");bh_vfile_close(cache);}
    cache=bh_vfile_open("bhnav-cache.tmp","rb");old_nodes=n.nodes;old_cells=n.routes.cells;live=memory.live;
    for(allocation=1;cache && allocation<=5;++allocation) {
        memory.fail_at=memory.calls+allocation;
        CHECK(!bh_navigation_build(&n,p,4,cache) && n.nodes==old_nodes && n.routes.cells==old_cells && memory.live==live && bh_vfile_tell(cache)==0,"graph/cache decoder allocation failures preserve graph/cursor/ownership");
    }
    memory.fail_at=0;if(cache)bh_vfile_close(cache);
    CHECK(cache_file("bhnav-cache.tmp",7,0),"write mismatching cache count");cache=bh_vfile_open("bhnav-cache.tmp","rb");
    CHECK(cache && bh_navigation_build(&n,p,4,cache) && !n.cache_used && n.routes.cells[2].distance==4,"count mismatch triggers real recomputation");
    if(cache){CHECK(bh_vfile_tell(cache)==4,"mismatching cache only consumes header");bh_vfile_close(cache);}
    CHECK(cache_file("bhnav-cache.tmp",2,1),"write truncated cache header");cache=bh_vfile_open("bhnav-cache.tmp","rb");old_nodes=n.nodes;
    CHECK(cache && !bh_navigation_build(&n,p,4,cache) && n.error==BH_NAV_CACHE && n.nodes==old_nodes,"short cache rejects instead of unchecked success");
    if(cache){CHECK(bh_vfile_tell(cache)==0,"cache failure restores borrowed cursor");bh_vfile_close(cache);}
    remove("bhnav-cache.tmp");bh_vfile_shutdown();bh_navigation_dispose(&n);CHECK(!memory.live,"complete navigation teardown");
}
int bh_navigation_checkpoint(unsigned long *count) {
    checks=0;failures=0;basics();*count=checks;
    printf("{\"scenario\":\"navigation\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}
