/* Isolated original segment owner. Never part of the game link. */
#include "../src/bh_segment_query.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../build/ai-collision/original.h"
static unsigned long rng=1979;
static float sample(void){rng=rng*1664525UL+1013904223UL;return (float)((long)(rng>>8)-8388608)/32768.0f;}
static void pointer(void *p,unsigned long offset,const void *v){memcpy((char*)p+offset,&v,4);}
int main(void){
 const char *names[]={"aicenter","aiwest","aieast","ainorth","aisouth","boxpost"};
 BHMathEnvironment env;BHMat34 matrix;unsigned char *code;unsigned long i,j,k,total=0,bad=0,hits=0;
 int (__cdecl *original)(long);
 if(!bh_math_environment_enter(&env))return 2;
 code=VirtualAlloc(0,8192,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 memset(code,0,8192);install_original(code);FlushInstructionCache(GetCurrentProcess(),code,8192);original=(int(__cdecl*)(long))code;
 memset(&matrix,0,sizeof(matrix));matrix.m[0][0]=matrix.m[1][1]=matrix.m[2][2]=1;
 for(i=0;i<6;++i){
  BHBsp bsp;BHGeometry geometry;BHSegmentQuery query;unsigned char raw[128],*bytes;long size;FILE *f;char path[160];
  sprintf(path,"build/box-test/bsp/%s.bsp",names[i]);f=fopen(path,"rb");if(!f)return 2;
  fseek(f,0,SEEK_END);size=ftell(f);rewind(f);bytes=malloc(size);if(!bytes||fread(bytes,1,size,f)!=(unsigned long)size)return 2;fclose(f);
  if(!bh_bsp_init(&bsp,0)||!bh_bsp_decode(&bsp,bytes,size)||!bh_geometry_init(&geometry,&bsp,0)||!bh_segment_query_init(&query,&geometry))return 2;
  free(bytes);memset(raw,0,sizeof(raw));pointer(raw,0x44,bsp.vertices);pointer(raw,0x4c,bsp.edges);pointer(raw,0x5c,bsp.nodes);pointer(raw,0x64,bsp.leaves);pointer(raw,0x78,geometry.face_flags);pointer(raw,0x7c,bsp.faces);
  pointer(code,4096,raw);pointer(code,4096+0x24,&matrix);
  for(j=0;j<8192;++j){
   BHVec3 a,b;BHSegmentHit got;unsigned long reference[12];int x,y;
   if(j==4096){
    /* Both face-list orientations, using the same decoded test geometry. */
    for(k=0;k<bsp.counts[4];++k){BHBspNode *n=(BHBspNode*)bsp.nodes+k;long leaf=n->front_leaf;n->front_leaf=n->back_leaf;n->back_leaf=leaf;}
   }
   for(k=0;k<3;++k){((float*)&a)[k]=sample();((float*)&b)[k]=sample();}
   /* Include starts in rooms and strict plane/edge boundaries. */
   if(j%4==0){a=bsp.bounds[0].center;}
   if(j%8==1){a=bsp.bounds[0].center; a.y=20; b.y=20;}
   memcpy(code+4096+8,&a,12);memcpy(code+4096+24,&b,12);memset(reference,0,sizeof(reference));pointer(code,4096+0x7c,reference);
   x=original(0);y=bh_segment_query_run(&query,&a,&b,&matrix,&matrix,&got);++total;if(x)++hits;
   if(x!=y||(x&&(memcmp(reference,&got,28)||reference[10]!=(unsigned long)got.face||reference[11]!=got.flags))){
    if(bad<8)printf("difference %s case%lu result%d/%d face%lu/%ld fraction %.9g/%.9g\n",names[i],j,x,y,reference[10],got.face,*(float*)reference,got.fraction);++bad;
   }
  }
  bh_segment_query_dispose(&query);bh_geometry_dispose(&geometry);bh_bsp_dispose(&bsp);
 }
 printf("segment_cases=%lu hits=%lu differences=%lu PC53\n",total,hits,bad);
 VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&env);return bad?1:0;
}
