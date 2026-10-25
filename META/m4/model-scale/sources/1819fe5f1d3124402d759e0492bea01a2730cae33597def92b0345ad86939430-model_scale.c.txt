#include "../src/bh_model_scale.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include "../src/bh_scene_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<15)printf("FAIL model-scale %lu: %s\n",checks,s);}}while(0)
typedef struct Memory {unsigned long allocations,releases,live;} Memory;
static void *__cdecl allocate(unsigned long bytes,void *user){Memory *m=(Memory*)user;void *p=malloc(bytes);if(p){++m->allocations;++m->live;}return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory*)user;++m->releases;--m->live;free(p);}
static unsigned long bits(float x){unsigned long u;memcpy(&u,&x,4);return u;}
static float scalar(unsigned long x){float n;memcpy(&n,&x,4);return n;}
typedef struct Fixture {BHModel m;BHBspVertex v[5];BHBspEdge e[8];BHBspFace f[2];BHBspNode n[2];} Fixture;
static void fixture(Fixture *f){
 unsigned long i;memset(f,0,sizeof(*f));f->m.vertex_count=5;f->m.vertices=f->v;f->m.edge_count=8;f->m.edges=f->e;
 f->m.face_count=2;f->m.faces=f->f;f->m.node_count=2;f->m.nodes=f->n;
 f->m.transform.m[0][0]=f->m.transform.m[1][1]=f->m.transform.m[2][2]=1;
 for(i=0;i<5;++i){f->v[i].x=(float)i-2;f->v[i].y=(float)i-1;f->v[i].z=(float)i+1;memset(f->v[i].opaque,0xab,48);f->m.runtime[i]=0xdead0001UL+i*4;}
 for(i=0;i<8;++i){f->e[i].vertex=i%5;f->e[i].u=(float)i*.125f;f->e[i].v=(float)i*.25f;f->e[i].next=i==4||i==7?-1:(long)i+1;}
 f->f[0].first_edge=0;f->f[1].first_edge=5;
 for(i=0;i<2;++i){f->n[i].minimum[0]=-2;f->n[i].maximum[0]=4;f->n[i].minimum[1]=-3;f->n[i].maximum[1]=5;
  f->n[i].minimum[2]=-6;f->n[i].maximum[2]=7;f->n[i].normal[0]=1;f->n[i].plane=-2;}
}
static void winding(void){
 Fixture f;BHBspEdge old[8];long count,i,j,next;unsigned long nan=0x7f800001UL;
 for(count=0;count<=8;++count){fixture(&f);for(i=0;i<8;++i)f.e[i].next=i==7?-99:i+1;
  memcpy(&f.e[0].u,&nan,4);memcpy(old,f.e,sizeof(old));
  next=bh_model_reverse_edges(&f.m,0,count);CHECK(next==(count==8?-99:count),"winding returns edge after requested segment");
  for(i=0;i<8;++i){j=i<count?count-1-i:i;
   CHECK(f.e[i].vertex==old[j].vertex,"whole odd/even payload reversal");
   CHECK(!memcmp(&f.e[i].u,&old[j].u,8),"UV payload bit preservation including signaling NaN");
   CHECK(f.e[i].next==old[i].next,"all edge next links retained");}
 }
 fixture(&f);memcpy(old,f.e,sizeof(old));CHECK(bh_model_reverse_edges(&f.m,-7,0)==-7,"zero-count arbitrary sentinel return");
 CHECK(bh_model_reverse_edges(&f.m,0,-1)==LONG_MIN,"negative count source guard");
 CHECK(bh_model_reverse_edges(&f.m,8,1)==LONG_MIN,"out of bounds start source guard");
 CHECK(bh_model_reverse_edges(&f.m,0,6)==LONG_MIN,"unterminated requested prefix source guard");
 CHECK(!memcmp(old,f.e,sizeof(old)),"invalid winding rejects before payload mutation");
}
static void scaling(void){
 Fixture f,before;BHModelDrawCache a,b,*cache[2];BHSceneAllocator owner;Memory memory={0,0,0};unsigned long i,j,releases;int axis;
 owner.allocate=allocate;owner.release=release;owner.user=&memory;
 for(axis=0;axis<3;++axis){
  float factors[3]={2,3,4};fixture(&f);before=f;memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));
  cache[0]=&a;cache[1]=&b;CHECK(bh_model_draw_cache_init(&a,&f.m,&owner)&&bh_model_draw_cache_init(&b,&f.m,&owner),"two actual independently allocated associated caches");
  CHECK(memory.live==6,"all six real cache allocations owned");factors[axis]=-factors[axis];releases=memory.releases;
  CHECK(bh_model_scale(&f.m,factors[0],factors[1],factors[2],cache,2),"reflected nonuniform scale succeeds");
  CHECK(memory.live==0&&memory.releases==releases+6&&!a.model&&!b.model&&!a.vertices&&!b.tasks,"every real cache disposed once through its stored allocator");
  for(i=0;i<5;++i){CHECK(!f.m.runtime[i],"opaque poison runtime words zeroed without dereference");
   CHECK(f.v[i].x==before.v[i].x*factors[0]&&f.v[i].y==before.v[i].y*factors[1]&&f.v[i].z==before.v[i].z*factors[2],"all vertex component products");
   CHECK(!memcmp(f.v[i].opaque,before.v[i].opaque,48),"serialized vertex opaque payload retained");}
  for(i=0;i<2;++i){
   for(j=0;j<3;++j){float low=before.n[i].minimum[j]*factors[j],high=before.n[i].maximum[j]*factors[j];
    CHECK(f.n[i].minimum[j]==(factors[j]<0?high:low)&&f.n[i].maximum[j]==(factors[j]<0?low:high),"reflected bounds ordered independently on every axis");}
   CHECK(f.n[i].normal[0]==(axis==0?-1.0f:1.0f)&&f.n[i].normal[1]==0&&f.n[i].normal[2]==0,"actual normalization of inverse-transpose normal");
   CHECK(f.n[i].plane==-4.0f,"plane through scaled old normal point");
  }
  CHECK(bits(f.m.radius)==bits((float)sqrt(64.0+225.0+784.0)),"root farthest-corner radius from scaled AABB");
  CHECK(f.e[0].vertex==before.e[4].vertex&&f.e[4].vertex==before.e[0].vertex&&f.e[5].u==before.e[7].u,"odd determinant reverses both face payload chains");
  for(i=0;i<8;++i)CHECK(f.e[i].next==before.e[i].next,"reflection retains face edge topology");
  CHECK(!memcmp(&f.m.transform,&before.m.transform,sizeof(f.m.transform))&&!f.m.frame,"model transform and zero frame retained");
  CHECK(bh_model_draw_cache_init(&a,&f.m,&owner),"cache must be recreated through real owner before draw");
  CHECK(a.vertices[2].local.x==f.v[2].x&&a.vertices[2].local.y==f.v[2].y&&a.vertices[2].local.z==f.v[2].z,"recreated cache copies scaled local coordinates");
  bh_model_draw_cache_dispose(&a);CHECK(!memory.live,"recreated cache lifetime balanced");
 }
 fixture(&f);f.e[4].next=LONG_MIN;before=f;
 CHECK(bh_model_reverse_edges(&f.m,0,5)==LONG_MIN,"LONG_MIN is a legitimate following-edge terminator");
 fixture(&f);f.e[4].next=LONG_MIN;before=f;
 CHECK(bh_model_scale(&f.m,-1,1,1,NULL,0),"reflected scale ignores helper return after valid LONG_MIN topology preflight");
 CHECK(f.e[4].next==LONG_MIN&&f.e[0].u==before.e[4].u,"LONG_MIN link preserved while payload reverses");
 fixture(&f);before=f;CHECK(bh_model_scale(&f.m,-2,-3,4,NULL,0),"two-axis reflection scale");
 CHECK(!memcmp(f.e,before.e,sizeof(f.e)),"positive determinant retains winding");
 fixture(&f);f.m.frame=7;before=f;CHECK(bh_model_scale(&f.m,scalar(0x7fc00000UL),0,0,NULL,99),"nonzero frame bypasses even invalid cache list/scales");
 CHECK(!memcmp(&f,&before,sizeof(f)),"nonzero-frame complete no-op");
 fixture(&f);before=f;CHECK(!bh_model_scale(&f.m,2,3,4,NULL,1),"missing list rejects before mutation");
 CHECK(!memcmp(&f,&before,sizeof(f)),"bad source binding retains all geometry/runtime words");
 memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));cache[0]=&a;cache[1]=&a;
 CHECK(!bh_model_scale(&f.m,2,3,4,cache,2),"duplicate cache rejected");
 a.model=&before.m;cache[0]=&a;CHECK(!bh_model_scale(&f.m,2,3,4,cache,1),"foreign model cache rejected");
 f.e[4].next=0;before=f;CHECK(!bh_model_scale(&f.m,-1,1,1,NULL,0),"cyclic reflected edge list rejected");
 CHECK(!memcmp(&f,&before,sizeof(f)),"topology preflight leaves all state unchanged");
 fixture(&f);f.m.node_count=0;CHECK(!bh_model_scale(&f.m,1,1,1,NULL,0),"root node required by radius owner");
 fixture(&f);before=f;CHECK(bh_model_scale(&f.m,scalar(0x7fc00001UL),1,1,NULL,0),"unordered scale follows raw C0 path");
 CHECK(f.e[0].u==before.e[4].u&&_isnan(f.n[0].minimum[0])&&_isnan(f.m.radius),"unordered determinant reverses winding and retains NaN geometry class");
 fixture(&f);before=f;CHECK(bh_model_scale(&f.m,0,1,1,NULL,0),"zero scale retains masked IEEE normalization behavior");
 CHECK(!memcmp(f.e,before.e,sizeof(f.e))&&_isnan(f.n[0].plane),"zero determinant retains winding and nonfinite plane");
}
static void plane_fixtures(void){
 static const unsigned long cases[][11]={
  {0x3ecb64e8UL,0xbf096f4eUL,0x3f3e8a2bUL,0xc566236eUL,0x3f5545caUL,0x3e27f408UL,0x3fd325d9UL,0x3e1243edUL,0xbf7b0060UL,0x3e0a6677UL,0xc489e021UL},
  {0xbe6e9040UL,0xbee7497dUL,0x3f5c7776UL,0x45a5d48eUL,0xc08c1d63UL,0x40f1e8e9UL,0x3f49ccabUL,0x3d46f362UL,0xbd5f6fa7UL,0x3f7f50f4UL,0x45976242UL},
  {0x3d941376UL,0x3f5217d1UL,0xbf1119dfUL,0xc6173ef2UL,0x3f90110bUL,0xc08ea2a2UL,0x409df2d3UL,0x3e9156eeUL,0xbf50479eUL,0xbf01e707UL,0xc727159dUL},
  {0x3e83813bUL,0xbe270e49UL,0xbf73dc86UL,0xc5bc4afdUL,0xc0105043UL,0xc0687ea4UL,0x40a8ee35UL,0xbf05b8beUL,0x3e52e28dUL,0xbf53d651UL,0xc6d7de93UL},
  {0x3f0a0a6bUL,0x3efaac35UL,0x3f2f6b13UL,0xc522c3f8UL,0x40385802UL,0x40b3a57bUL,0xc0f1a399UL,0x3f547386UL,0x3ec5f14aUL,0xbecdf66cUL,0xc63462d7UL},
  {0xbddd0dedUL,0xbf6a58f8UL,0xbec68822UL,0xc58d26a4UL,0xc03582eaUL,0x3fe83f15UL,0xc06022a9UL,0x3d967d23UL,0xbf795f9dUL,0x3e5ae80cUL,0xc60843b6UL},
  {0x3f79e546UL,0x3e41a77dUL,0x3dda41d0UL,0x45b81e09UL,0x3e0af636UL,0xbfc644e6UL,0xc05d0139UL,0x3f7ff5f6UL,0xbc8b0583UL,0xbb8c909bUL,0x444cbc7dUL},
  {0xbee96481UL,0x3f41a9f7UL,0xbef01a27UL,0x45e2c98cUL,0xc0b3834eUL,0xc0d9f7f6UL,0xc0d1e367UL,0x3f0623cbUL,0xbf375660UL,0x3eec0cd1UL,0x4736ccadUL},
  {0xbf3f37ddUL,0x3f003a24UL,0xbedfdea0UL,0xc5df5613UL,0x3d9c72ccUL,0x40259c65UL,0x400f638dUL,0xbf7fe61fUL,0x3ca21b58UL,0xbca370d0UL,0xc436a777UL},
  {0x3f591c85UL,0xbed91f36UL,0xbea2a483UL,0x4576288dUL,0xc04bc1a9UL,0x3f8b300eUL,0x402f8355UL,0xbf0c3d33UL,0xbf4d4e36UL,0xbe73ec4eUL,0x45fd1b25UL},
  {0x3f44f840UL,0x3f02727eUL,0xbec53477UL,0xc562d39aUL,0x40d4dfcdUL,0xc0938305UL,0x406d4fb9UL,0x3f1b392cUL,0xbf1459b4UL,0xbf0b67cbUL,0xc694a3beUL},
  {0xbef3b60bUL,0xbe32a4a7UL,0xbf5caa01UL,0xc5586e66UL,0xc0e54c15UL,0xc02c1015UL,0x40b6db5fUL,0x3ec001b0UL,0x3ebb8f39UL,0xbf5a006eUL,0xc698ba73UL},
 };Fixture f;unsigned long i,j;
 for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i){fixture(&f);
  for(j=0;j<3;++j)f.n[0].normal[j]=scalar(cases[i][j]);f.n[0].plane=scalar(cases[i][3]);
  CHECK(bh_model_scale(&f.m,scalar(cases[i][4]),scalar(cases[i][5]),scalar(cases[i][6]),NULL,0),"nontrivial plane fixture scale");
  for(j=0;j<3;++j)CHECK(bits(f.n[0].normal[j])==cases[i][7+j],"independent PC53 normalized normal golden");
  CHECK(bits(f.n[0].plane)==cases[i][10],"independent binary32 old-point spills and Z+X+Y plane golden");
 }
}
static void real_asset(const char *archive){
 BHSceneView view;BHTextureContext texture;BHModelContext context;BHModel *model;BHModelDrawCache cache,*list[1];float old,expected;int mounted;
 memset(&view,0,sizeof(view));memset(&texture,0,sizeof(texture));memset(&context,0,sizeof(context));memset(&cache,0,sizeof(cache));
 mounted=bh_vfile_mount(archive);CHECK(mounted,"real archive mounted as immutable data");if(!mounted)return;
 CHECK(bh_scene_view_init(&view,32,32),"actual private offscreen native image context");if(!view.device)goto finish;
 CHECK(bh_texture_context_init(&texture,view.draw,view.device,NULL)&&bh_texture_formats(&texture)&&bh_texture_set_path(&texture,"TEXTURES",2),"real texture owners for model material dependencies");
 CHECK(bh_model_context_init(&context,&texture,NULL)&&bh_model_set_path(&context,"BSP\\",BH_BSP_PREFIX_ONLY),"real model allocation and archive path context");
 model=bh_model_load(&context,"CAPTAIN.BSP");CHECK(model!=NULL,"actual startup captain model loaded");
 if(model){old=model->vertices[0].x;list[0]=&cache;
  CHECK(bh_model_draw_cache_init(&cache,model,NULL),"real asset associated cache allocated");
  CHECK(bh_model_scale(model,3,3,3,list,1)&&!cache.model,"actual startup uniform captain scale disposes cache");
  bh_round_float(&expected,(double)old*3);
  CHECK(bits(model->vertices[0].x)==bits(expected)&&model->radius>0,"actual asset vertex and radius updated");
  CHECK(bh_model_draw_cache_init(&cache,model,NULL)&&cache.vertices[0].local.x==model->vertices[0].x,"real asset cache safely reinitialized with scaled local data");
  bh_model_draw_cache_dispose(&cache);bh_model_delete(&context,model);
 }
 bh_texture_context_dispose(&texture);
finish:bh_scene_view_dispose(&view);bh_vfile_shutdown();
}
int __cdecl bh_model_scale_checkpoint(const char *archive,unsigned long *count){
 BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
 winding();scaling();plane_fixtures();if(archive)real_asset(archive);bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"model-scale\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_MODEL_SCALE_STANDALONE
int main(int argc,char **argv){return bh_model_scale_checkpoint(argc>1?argv[1]:NULL,NULL);}
#endif
