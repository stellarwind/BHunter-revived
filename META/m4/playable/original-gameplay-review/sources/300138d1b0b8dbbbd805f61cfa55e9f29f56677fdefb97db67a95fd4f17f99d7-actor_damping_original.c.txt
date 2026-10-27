/* Optional isolated reference adapter. Never linked into the game. */
#define bh_actor_response_checkpoint included_response_checkpoint
#include "actor_response.c"
#undef bh_actor_response_checkpoint
#include <windows.h>
#include "../build/original-review/actor_damping_original.h"
int main(void){
 ResponseFixture r;BHMathEnvironment env;BHEntity *e;unsigned char reference[sizeof(BHEntity)];
 unsigned char *code;unsigned long state=1,i,j,bad=0,bits;float v;
 void (__cdecl *original)(void*);FILE *gold;
 remove("actor-damage-fixture/HUN_LOD1.BSP");_rmdir("actor-damage-fixture");
 if(!bh_math_environment_enter(&env)||!model_file()||!response_fixture(&r))return 2;
 code=VirtualAlloc(0,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 memcpy(code,original_bytes,sizeof(original_bytes));relocate(code);FlushInstructionCache(GetCurrentProcess(),code,4096);original=(void (__cdecl *)(void*))code;
 gold=fopen("actor-damping-golden.h","w");if(!gold)return 2;
 fprintf(gold,"/* Captured from original 00439a52..00439ba7 under PC53. */\n");
 e=actor(&r,2);
 for(i=0;i<256;++i){
  real(e,0x180,0);real(e,0x194,0);real(e,0x198,1);real(e,0x19c,1);
  fprintf(gold,"{{");
  for(j=0;j<6;++j){state=state*1664525UL+1013904223UL;v=(float)((long)(state>>8)-8388608)/524288.0f;real(e,0x15c+4*j,v);memcpy(&bits,&v,4);fprintf(gold,"0x%08lxUL%s",bits,j==5?"}, {":",");}
  memcpy(reference,e,sizeof(reference));original(reference);bh_actor_response_tick(e);
  if(r.response.error||memcmp(reference+0x15c,(char*)e+0x15c,24)){if(bad<4){printf("DIFF case=%lu expected=",i);for(j=0;j<6;++j)printf("%08lx,",word((BHEntity*)reference,0x15c+j*4));printf(" actual=");for(j=0;j<6;++j)printf("%08lx,",word(e,0x15c+j*4));puts("");}++bad;}
  for(j=0;j<6;++j)fprintf(gold,"0x%08lxUL%s",word((BHEntity*)reference,0x15c+j*4),j==5?"}},\n":",");
 }
 fclose(gold);VirtualFree(code,0,MEM_RELEASE);finish(&r.base);remove("actor-damage-fixture/HUN_LOD1.BSP");_rmdir("actor-damage-fixture");bh_math_environment_leave(&env);
 printf("{\"original_block_cases\":256,\"different_cases\":%lu,\"whole_function_match_claimed\":false}\n",bad);return bad?1:0;
}
