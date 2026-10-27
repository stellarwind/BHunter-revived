/* Explicit original arithmetic test adapter, excluded from all game targets. */
#include <windows.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../build/launch-review/original.h"
int main(void){
 unsigned char actor[864],output[28],*code;unsigned long offsets[]={12,36,48,24,72,640};
 unsigned long state=1,i,j,k,bits,expected[6];float v,base;double x;FILE*f;
 void (__cdecl *original)(void*,void*,unsigned long);
 _control87(_PC_53|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);
 code=VirtualAlloc(0,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 memcpy(code,original_bytes,sizeof(original_bytes));relocate(code);FlushInstructionCache(GetCurrentProcess(),code,4096);original=(void (__cdecl *)(void*,void*,unsigned long))code;
 /* Exactly representable positive control checks wrapper stack/output mapping. */
 memset(actor,0,sizeof(actor));v=1;memcpy(actor+24,&v,4);memcpy(actor+40,&v,4);memcpy(actor+56,&v,4);memcpy(actor+640,&v,4);
 original(actor,output,0);memcpy(&v,output,4);if(v!=-1)return 3;memcpy(&v,output+4,4);if(v!=.5f)return 3;memcpy(&v,output+8,4);if(v!=1.5f)return 3;memcpy(&x,output+12,8);if(x!=150)return 3;memcpy(&v,output+20,4);if(v!=0)return 3;memcpy(&v,output+24,4);if(v!=0)return 3;
 original(actor,output,2);memcpy(&v,output,4);if(v!=1)return 3;
 f=fopen("ballistic_launch_golden.h","w");if(!f)return 2;
 fprintf(f,"/* Original call-free firing block PC53; original CRT RNG spread. */\n");
 for(i=0;i<128;++i){
  memset(actor,0,sizeof(actor));fprintf(f,"{{");
  for(j=0;j<6;++j)for(k=0;k<3;++k){state=state*1664525UL+1013904223UL;v=(float)((long)(state>>8)-8388608)/131072.0f;memcpy(actor+offsets[j]+k*4,&v,4);memcpy(&bits,&v,4);fprintf(f,"0x%08lxUL%s",bits,j==5&&k==2?"},{":",");}
  original(actor,output,(i&1)*2);memcpy(expected,output,12);memcpy(&x,output+12,8);srand(i+17);
  v=(float)(x-((double)rand()-16384)*(-1.0/8192));memcpy(expected+3,&v,4);
  for(j=0;j<2;++j){memcpy(&base,output+20+j*4,4);v=(float)((double)base-((double)rand()-16384)*(-1.0/8192));memcpy(expected+4+j,&v,4);}
  for(j=0;j<6;++j)fprintf(f,"0x%08lxUL%s",expected[j],j==5?"}},\n":",");
 }
 fclose(f);VirtualFree(code,0,MEM_RELEASE);puts("Captured 128 original arithmetic cases; no full-function comparison claimed.");return 0;
}
