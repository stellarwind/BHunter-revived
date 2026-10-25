#include "../src/bh_collision_cache.h"
#include "../src/bh_collision_model.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<12)printf("FAIL collision cache %d: %s\n",line,label);}}
typedef struct Fault {unsigned long calls,fail,live,sizes[5];} Fault;
static void *__cdecl allocate(unsigned long bytes,void *user){Fault *f=user;void *p;f->sizes[f->calls++]=bytes;if(f->calls==f->fail)return 0;p=malloc(bytes);if(p)++f->live;return p;}
static void __cdecl release(void *p,void *user){Fault *f=user;if(p){--f->live;free(p);}}
static int model_fixture(const char *name) {
    BHBspMaterial material;BHBspVertex vertices[3];BHBspEdge edges[3];BHBspFace face;BHBspNode node;BHBspLeaf leaf;
    unsigned long counts[6]={1,3,3,1,1,1},sizes[6],i;const void *sections[6];FILE *file=fopen(name,"wb");
    if(!file)return 0;
    memset(&material,0,sizeof(material));strcpy(material.texture,"STONE.PCX");
    memset(vertices,0,sizeof(vertices));memset(edges,0,sizeof(edges));
    for(i=0;i<3;++i){edges[i].vertex=i;edges[i].next=i<2?(long)i+1:-1;}
    vertices[0].y=vertices[0].z=-1;vertices[1].y=1;vertices[1].z=-1;vertices[2].z=1;
    face.material=face.first_edge=0;face.runtime=0x12345678;
    memset(&node,0,sizeof(node));for(i=0;i<3;++i){node.minimum[i]=-1;node.maximum[i]=1;}
    node.normal[0]=1;node.front_node=node.back_node=node.back_leaf=-1;leaf.face=0;leaf.next=-1;
    sections[0]=&material;sections[1]=vertices;sections[2]=edges;sections[3]=&face;sections[4]=&node;sections[5]=&leaf;
    sizes[0]=sizeof(material);sizes[1]=sizeof(vertices);sizes[2]=sizeof(edges);sizes[3]=sizeof(face);sizes[4]=sizeof(node);sizes[5]=sizeof(leaf);
    for(i=0;i<6;++i)if(fwrite(counts+i,1,4,file)!=4 || fwrite(sections[i],1,sizes[i],file)!=sizes[i]){fclose(file);return 0;}
    return !fclose(file);
}

static void verify(BHCollisionModel *m) {
 BHCollisionCache c,before;BHSceneAllocator a;Fault f;unsigned long i,k;BHBspFace face;
 memset(&c,0,sizeof(c));memset(&f,0,sizeof(f));a.allocate=allocate;a.release=release;a.user=&f;
 for(k=1;k<=5;++k){memset(&f,0,sizeof(f));f.fail=k;CHECK(!bh_collision_cache_prepare(&c,&m->bsp,&a) && c.error==BH_BSP_MEMORY && !c.centers && !f.live && f.calls==k,"each real allocator failure rolls back every allocation");}
 memset(&f,0,sizeof(f));CHECK(bh_collision_cache_prepare(&c,&m->bsp,&a) && f.calls==5 && f.live==5,"all five original owned arrays allocated");
 CHECK(f.sizes[0]==m->bsp.counts[3]*4 && f.sizes[1]==m->bsp.counts[4]*12 && f.sizes[2]==m->bsp.counts[4]*4 && f.sizes[3]==m->bsp.counts[4]*4 && f.sizes[4]==m->bsp.counts[3]*12,"original allocation order and sizes");
 for(i=0;i<c.face_count;++i){face=m->bsp.faces[i];face.runtime=0;CHECK(!memcmp(c.faces+i,&face,sizeof(face)) && !c.face_flags[i],"independent face copy retains topology and clears live words");}
 for(i=0;i<c.node_count;++i){CHECK(!memcmp(c.centers+i,&m->bsp.bounds[i].center,12) && c.radii[i]==m->bsp.bounds[i].radius && c.squared_radii[i]==m->bsp.bounds[i].radius_squared,"separate real decoder bound owner agrees for every loaded node");}
 c.face_flags[0]=0xaabbccdd;c.faces[0].runtime=173;c.centers[0].x=17;before=c;
 CHECK(bh_collision_cache_prepare(&c,&m->bsp,0) && !memcmp(&before,&c,sizeof(c)) && f.calls==5 && c.face_flags[0]==0xaabbccdd && c.faces[0].runtime==173 && c.centers[0].x==17,"lazy guard retains mutable cache and never recomputes");
 CHECK(m->bsp.faces[0].runtime==0 && m->bsp.bounds[0].center.x!=17,"cache mutable copies never alter borrowed source geometry");
 bh_collision_cache_dispose(&c);CHECK(!f.live && !c.centers && !c.faces,"dispose returns all five resources");bh_collision_cache_dispose(&c);
}
static void load(const char *archive) {
 BHCollisionModel m;BHCollisionCache c;BHBsp b;BHBspFace face={7,9,0x12345678};BHBspNode node;BHSceneAllocator a;Fault f;
 memset(&c,0,sizeof(c));memset(&b,0,sizeof(b));memset(&node,0,sizeof(node));memset(&f,0,sizeof(f));a.allocate=allocate;a.release=release;a.user=&f;
 b.faces=&face;b.nodes=&node;b.counts[3]=b.counts[4]=1;node.minimum[0]=2;node.maximum[0]=4;node.minimum[1]=-8;node.maximum[1]=0;node.minimum[2]=10;node.maximum[2]=14;
 CHECK(bh_collision_cache_prepare(&c,&b,&a) && c.centers[0].x==3 && c.centers[0].y==-4 && c.centers[0].z==12 && c.squared_radii[0]==21 && c.radii[0]==4.582575798034668f,"independent asymmetric known geometry center and 1+16+4 radius oracle");
 CHECK(c.faces[0].material==7 && c.faces[0].first_edge==9 && !c.faces[0].runtime && face.runtime==0x12345678,"nonzero serialized runtime cleared only in owned face copy");bh_collision_cache_dispose(&c);
 CHECK(!bh_collision_cache_prepare(0,&b,0) && !bh_collision_cache_prepare(&c,0,0),"null argument guards");b.counts[4]=65536;CHECK(!bh_collision_cache_prepare(&c,&b,&a) && !f.live,"bounded count rejects before allocation");
 CHECK(model_fixture("COLLISION_CACHE.BSP"),"real serialized triangle model created");CHECK(bh_collision_model_init(&m,0),"actual model owner initialize");
 if(bh_collision_model_load(&m,"COLLISION_CACHE.BSP","",BH_BSP_BARE_ONLY)){CHECK(1,"actual source VFILE and model decoder");verify(&m);}else CHECK(0,"actual source fixture load");
 if(archive){CHECK(bh_vfile_mount(archive),"mount immutable actual archive");if(bh_collision_model_load(&m,"PU_9A.BSP","BSP\\",BH_BSP_PREFIX_ONLY)){CHECK(1,"actual archive model load");verify(&m);}else CHECK(0,"archive model load");}
 CHECK(bh_collision_model_dispose(&m),"real model ownership release");bh_vfile_shutdown();remove("COLLISION_CACHE.BSP");
}
int bh_collision_cache_checkpoint(const char *archive,unsigned long *count){BHMathEnvironment e;checks=0;failures=0;if(!bh_math_environment_enter(&e))return 1;load(archive);bh_math_environment_leave(&e);if(count)*count=checks;printf("{\"scenario\":\"collision-cache\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;}
#ifdef BH_COLLISION_CACHE_STANDALONE
int main(int argc,char **argv){unsigned long count;return bh_collision_cache_checkpoint(argc>1?argv[1]:0,&count);}
#endif

