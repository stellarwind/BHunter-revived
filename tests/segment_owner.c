#include "../src/bh_segment_owner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL segment owner %s\n",n);}}while(0)
static void *allocate(unsigned long bytes,void *user){(void)user;return malloc(bytes);}
static void release(void *p,void *user){(void)user;free(p);}
int bh_segment_owner_checkpoint(unsigned long *count) {
    BHBsp bsp,other;BHBspVertex vertices[4];BHBspEdge edges[4];BHBspFace face;BHBspNode node;BHBspLeaf leaf;
    BHSegmentOwner owner;BHSegmentHit hit,saved;BHMat34 matrix,world,local;BHVec3 a={2,0,0},b={-2,0,0};unsigned long i;void *cache,*frames;
    checks=0;failures=0;memset(&owner,0,sizeof(owner));memset(&bsp,0,sizeof(bsp));memset(vertices,0,sizeof(vertices));memset(edges,0,sizeof(edges));memset(&face,0,sizeof(face));memset(&node,0,sizeof(node));
    memset(&matrix,0,sizeof(matrix));matrix.m[0][0]=matrix.m[1][1]=matrix.m[2][2]=1;
    bsp.allocator.allocate=allocate;bsp.allocator.release=release;bsp.vertices=vertices;bsp.edges=edges;bsp.faces=&face;bsp.nodes=&node;bsp.leaves=&leaf;
    bsp.counts[1]=bsp.counts[2]=4;bsp.counts[3]=bsp.counts[4]=bsp.counts[5]=1;
    for(i=0;i<4;++i){vertices[i].y=i<2?-1:1;vertices[i].z=(i==0||i==3)?-1:1;edges[i].vertex=i;edges[i].next=i==3?-1:i+1;}
    face.first_edge=0;node.normal[0]=1;node.front_leaf=0;node.back_leaf=-1;node.front_node=node.back_node=-1;
    for(i=0;i<3;++i){node.minimum[i]=-1;node.maximum[i]=1;}leaf.face=0;leaf.next=-1;
    memset(&hit,0xa5,sizeof(hit));saved=hit;
    CHECK(bh_segment_owner_query(&owner,NULL,NULL,NULL,NULL,NULL,&hit)==0&&!memcmp(&hit,&saved,sizeof(hit))&&!owner.cache.centers,"null model original no-op");
    CHECK(bh_segment_owner_query(&owner,&bsp,&a,&b,&matrix,&matrix,&hit)==1,"whole lazy cache prepare and segment hit");
    CHECK(owner.cache.centers&&owner.query.frames&&owner.geometry.face_flags==owner.cache.face_flags&&owner.query.geometry==&owner.geometry,"real complete owner dependency graph");
    CHECK(hit.face==0&&hit.fraction==0.5f&&hit.normal.x==1&&hit.point.x>0.0499f&&hit.point.x<0.0501f,"world result fraction normal and original offset");
    cache=owner.cache.centers;frames=owner.query.frames;owner.cache.face_flags[0]=1;saved=hit;
    CHECK(bh_segment_owner_query(&owner,&bsp,&a,&b,&matrix,&matrix,&hit)==0&&!memcmp(&hit,&saved,sizeof(hit)),"mutable face flag miss preserves result");
    CHECK(owner.cache.centers==cache&&owner.query.frames==frames&&owner.cache.face_flags[0]==1,"lazy prepared metadata not reset");
    owner.cache.face_flags[0]=0;world=local=matrix;world.m[0][3]=10;local.m[0][3]=-10;a.x=12;b.x=8;
    CHECK(bh_segment_owner_query(&owner,&bsp,&a,&b,&world,&local,&hit)==1&&hit.point.x>10.049f&&hit.point.x<10.051f,"actual explicit world/local transform pair");
    other=bsp;CHECK(bh_segment_owner_query(&owner,&other,&a,&b,&world,&local,&hit)==-1&&owner.cache.bsp==&bsp,"reject BSP replacement while cache live");
    owner.query.busy=1;CHECK(!bh_segment_owner_dispose(&owner)&&owner.cache.centers==cache,"busy lifetime guard");owner.query.busy=0;
    CHECK(bh_segment_owner_dispose(&owner)&&!owner.cache.centers&&!owner.query.frames&&!owner.geometry.bsp,"entire prepared owner disposed");
    CHECK(bh_segment_owner_dispose(&owner),"idempotent dispose");
    if(count)*count+=checks;printf("{\"scenario\":\"segment-owner\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_SEGMENT_OWNER_STANDALONE
int main(void){unsigned long n=0;return bh_segment_owner_checkpoint(&n);}
#endif
