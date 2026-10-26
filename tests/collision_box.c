#include "../src/bh_collision_box.h"
#include "../src/bh_geometry_radius.h"
#include "../src/bh_segment_query.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<20)printf("FAIL collision-box %d: %s\n",__LINE__,s);}}while(0)
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long bytes,void *user){Memory *m=(Memory*)user;void *p;if(++m->calls==m->fail_at)return NULL;p=malloc(bytes);if(p){++m->live;memset(p,0xa5,bytes);}return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory*)user;CHECK(m->live>0,"real allocator balanced");--m->live;free(p);}
static BHVec3 vector(float x,float y,float z){BHVec3 v;v.x=x;v.y=y;v.z=z;return v;}
static int close_value(double a,double b){return fabs(a-b)<0.00001*(fabs(b)+1);}
static void check_model(BHCollisionModel *model,const BHVec3 *lo,const BHVec3 *hi){
 /* Independent original populated-record transcription. Vertex indices below
    are read in actual face-edge order, not derived from renderer normals. */
 static const long order[24]={0,4,6,2,3,7,5,1,1,5,4,0,2,6,7,3,2,3,1,0,4,5,7,6};
 static const float uv[8]={0,0,1,0,1,1,0,1};
 static const float normals[18]={-1,0,0,1,0,0,0,-1,0,0,1,0,0,0,-1,0,0,1};
 BHBsp *b=&model->bsp;BHBspVertex expected;BHFaceInfo face;BHVec3 vertex;unsigned long i,j,stamp=0xffffffffUL,flag;const float *l=&lo->x,*h=&hi->x;
 CHECK(b->counts[0]==2&&b->counts[1]==8&&b->counts[2]==24&&b->counts[3]==6&&b->counts[4]==6&&b->counts[5]==6,"all six whole record arrays owned");
 CHECK(model->geometry.bsp==b&&b->storage&&b->bounds&&model->geometry.face_flags,"actual geometry bound to actual owned model");
 CHECK(!strcmp(b->materials[0].texture,"default")&&!b->materials[1].texture[0],"original default material0 and defined unspecified material1");
 for(i=0;i<8;++i){memset(&expected,0,sizeof(expected));expected.x=i&1?h[0]:l[0];expected.y=i&2?h[1]:l[1];expected.z=i&4?h[2]:l[2];memcpy((unsigned char*)&expected+0x34,&stamp,4);CHECK(!memcmp(b->vertices+i,&expected,60),"all vertex coordinates, original frame sentinel and defined cache bytes");}
 for(i=0;i<24;++i){CHECK(b->edges[i].vertex==order[i]&&b->edges[i].next==(i%4==3?-1:(long)i+1),"exact four-edge winding and terminating next");CHECK(b->edges[i].u==uv[(i%4)*2]&&b->edges[i].v==uv[(i%4)*2+1],"all original UV corners");}
 for(i=0;i<6;++i){const BHBspNode *n=b->nodes+i;const BHBspBound *bound=b->bounds+i;BHVec3 mid;double dx,dy,dz;volatile float sum,rx,rz,squared,radius;
  CHECK(b->faces[i].material==1&&b->faces[i].first_edge==(long)i*4&&b->faces[i].runtime==0,"original completed face records");
  CHECK(b->leaves[i].face==(long)i&&b->leaves[i].next==-1,"independent face leaves");
  CHECK(!memcmp(n->minimum,lo,12)&&!memcmp(n->maximum,hi,12)&&!memcmp(n->normal,normals+i*3,12),"each full box bound and axis normal");
  CHECK(n->front_leaf==(long)i&&n->back_leaf==-1&&n->front_node==-1&&n->back_node==(i==5?-1:(long)i+1),"whole ordered BSP chain");
  CHECK(n->plane==-(i%2?h[i/2]:l[i/2]),"negated selected bound planes preserved literally");
  sum=(float)((double)l[0]+h[0]);bh_round_float(&mid.x,(double)sum/2);sum=(float)((double)l[1]+h[1]);bh_round_float(&mid.y,(double)sum/2);sum=(float)((double)l[2]+h[2]);bh_round_float(&mid.z,(double)sum/2);
  dx=(double)h[0]-mid.x;dy=(double)h[1]-mid.y;dz=(double)h[2]-mid.z;rx=(float)dx;rz=(float)dz;squared=(float)((dx*rx+dy*dy)+dz*rz);
  radius=(float)sqrt(squared);CHECK(!memcmp(&bound->center,&mid,12)&&bound->radius_squared==squared&&bound->radius==radius,"retained midpoint float spill and original x/z mixed precision radii");
  memset(&face,0,sizeof(face));CHECK(bh_geometry_face(&model->geometry,i,&face)&&face.vertices==4&&!face.texture[0]&&!memcmp(&face.normal,normals+i*3,12),"actual source face descriptor traverses new box");
  for(j=0;j<4;++j){const BHBspVertex *v=b->vertices+order[i*4+j];CHECK(bh_geometry_vertex(&model->geometry,i,j,&vertex)&&!memcmp(&vertex,v,12),"real vertex owner follows new edge chains");}
  CHECK(bh_geometry_get_flags(&model->geometry,i,&flag)&&!flag,"actual separate original zero face flags");
 }
}
static void data_cases(void){
 BHCollisionModel m;BHVec3 e,p,lo,hi;unsigned long i;BHBspFace *faces;const unsigned char *old;
 CHECK(bh_collision_model_init(&m,NULL),"actual model lifecycle initialization");
 e=vector(10,20,30);p=vector(1,2,3);lo=vector(-1,-2,-3);hi=vector(9,18,27);
 CHECK(bh_collision_box_build(&m,&e,&p),"whole asymmetric box construction");check_model(&m,&lo,&hi);
 CHECK(m.bsp.bounds[0].center.x==4&&m.bsp.bounds[0].center.y==8&&m.bsp.bounds[0].center.z==12&&m.bsp.bounds[0].radius_squared==350,"independent numeric populated-record anchor");
 {volatile float radius=(float)sqrt(1134);CHECK(bh_geometry_radius(&m.bsp)==(double)radius,"real actor-radius consumer uses generated box rather than fabricated radius");}
 e=vector(1,1,1);p=vector(0.0000000298023223876953125f,0.0000000298023223876953125f,0.0000000298023223876953125f);
 CHECK(bh_collision_box_build(&m,&e,&p)&&m.bsp.nodes[0].maximum[0]==1&&m.bsp.bounds[0].center.x==0.5f&&m.bsp.bounds[0].radius_squared==0.75f,"positive discriminator: upper extent subtraction and midpoint must store binary32 before later arithmetic");
 for(i=0;i<36;++i){e=vector((float)(i+1)/7,(float)(i%5)/3,(float)(i%7)/11);p=vector((float)(i%3)/9,(float)(i%4)/-7,(float)(i%6)/13);lo=vector(-p.x,-p.y,-p.z);hi=vector((float)((double)e.x-p.x),(float)((double)e.y-p.y),(float)((double)e.z-p.z));CHECK(bh_collision_box_build(&m,&e,&p),"replacing with varied or degenerate original extents");check_model(&m,&lo,&hi);}
 old=m.bsp.storage;faces=(BHBspFace*)m.bsp.faces;faces[0].runtime=0x12345678;
 e.x=-1;CHECK(!bh_collision_box_build(&m,&e,&p)&&m.bsp.storage==old&&faces[0].runtime==0x12345678,"source negative extent rejection retains existing whole model");
 e.x=1;m.geometry.enumerating=1;CHECK(!bh_collision_box_build(&m,&e,&p)&&m.bsp.storage==old,"source active enumeration lifetime guard");m.geometry.enumerating=0;
 CHECK(!bh_collision_box_build(&m,NULL,&p)&&m.bsp.storage==old,"source NULL guard retains live model");
 {unsigned long bits=0x7fc00000;memcpy(&e.x,&bits,4);CHECK(!bh_collision_box_build(&m,&e,&p)&&m.bsp.storage==old,"nonfinite source input rejection");}
 CHECK(bh_collision_model_dispose(&m),"real source model disposal");
}
static void allocation_cases(void){
 Memory memory={0,0,0};BHSceneAllocator a={allocate,release,&memory};BHCollisionModel m;BHVec3 e= {4,6,8},p={1,2,3};unsigned long i,calls,live;unsigned char *old;BHGeometry geometry;
 CHECK(bh_collision_model_init(&m,&a)&&bh_collision_box_build(&m,&e,&p),"real custom allocator model construction");calls=memory.calls;CHECK(calls==4&&memory.live==3,"actual validation work plus owned storage/bounds/flags allocations");
 old=m.bsp.storage;geometry=m.geometry;live=memory.live;
 for(i=1;i<=calls;++i){memory.fail_at=memory.calls+i;CHECK(!bh_collision_box_build(&m,&e,&p)&&m.bsp.error==BH_BSP_MEMORY,"each actual allocation failure reported");CHECK(m.bsp.storage==old&&m.geometry.face_flags==geometry.face_flags&&m.geometry.bsp==&m.bsp&&memory.live==live,"failed build retains all ownership and frees partial transaction");memory.fail_at=0;}
 CHECK(bh_collision_box_build(&m,&e,&p)&&m.bsp.storage!=old&&memory.live==live,"success replaces actual storage and releases prior ownership");
 CHECK(bh_collision_model_dispose(&m)&&!memory.live,"all allocator-managed resources released");
}
static void query_cases(void){
 BHCollisionModel m;BHSegmentQuery q;BHSegmentHit hit,before;BHMat34 identity;BHVec3 e={2,2,2},p={1,1,1},a={-3,0,0},b={3,0,0};int result;unsigned long i;
 memset(&identity,0,sizeof(identity));identity.m[0][0]=identity.m[1][1]=identity.m[2][2]=1;
 CHECK(bh_collision_model_init(&m,NULL)&&bh_collision_box_build(&m,&e,&p)&&bh_segment_query_init(&q,&m.geometry),"actual segment workspace borrows real constructed geometry");
 memset(&hit,0,sizeof(hit));result=bh_segment_query_run(&q,&a,&b,&identity,&identity,&hit);
 CHECK(result==1&&hit.face==0&&close_value(hit.fraction,2.0/3)&&close_value(hit.point.x,0.95)&&hit.normal.x==-1,"source segment owner consumes retained low-face plane expression without geometric correction");
 for(i=0;i<6;++i)CHECK(bh_geometry_set_flags(&m.geometry,i,1),"real mutable face suppression owner");
 before=hit;CHECK(bh_segment_query_run(&q,&a,&b,&identity,&identity,&hit)==0&&!memcmp(&hit,&before,sizeof(hit)),"actual face flags suppress all new-box contacts without replacing output");
 CHECK(bh_segment_query_dispose(&q)&&bh_collision_model_dispose(&m),"borrower-before-owner teardown");
}
int bh_collision_box_checkpoint(unsigned long *count){BHMathEnvironment env;checks=failures=0;CHECK(bh_math_environment_enter(&env),"PC53 numerical contract");data_cases();allocation_cases();query_cases();bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"collision-box\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;}

