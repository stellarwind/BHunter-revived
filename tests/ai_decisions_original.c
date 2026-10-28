#include "../src/bh_route_choice.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../build/ai-decisions/original.h"
static void word(void *p,unsigned long at,unsigned long v){memcpy((char*)p+at,&v,4);}
int main(void){
 unsigned char *code,raw[1024],neighbors[6][1024];BHEntity actor,reference;
 BHWorldGeometry room,worlds[6];BHGeometryModel model;unsigned long i,j,state,bad=0;
 int (__cdecl *original)(BHEntity*,void*,void*,long);
 code=VirtualAlloc(0,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 install_original(code);original=(int(__cdecl*)(BHEntity*,void*,void*,long))code;
 memset(&room,0,sizeof(room));memset(&model,0,sizeof(model));room.model=&model;
 for(i=0;i<65536;++i){
  long selected,expected,excluded=(long)(i%8)-1;void *preferred=0;BHWorldGeometry *source_preferred=0;
  memset(raw,0,sizeof(raw));memset(&actor,0,sizeof(actor));memset(worlds,0,sizeof(worlds));
  model.portal_count=room.navigation.count=i%7;word(code,1024+24,model.portal_count);
  word(&actor,0xfc,(i%9)-1);state=i*2654435761UL;word(&actor,0x1e8,state);
  for(j=0;j<6;++j){
   unsigned long flags=((i>>(j+1))&3)<<4;memset(neighbors[j],0,1024);word(neighbors[j],0x114,flags);worlds[j].query_flags=flags;
   room.navigation.portals[j].neighbor=(i&(1UL<<(j+8)))?0:&worlds[j].navigation;
   word(raw,0x204+j*60,room.navigation.portals[j].neighbor?(unsigned long)neighbors[j]:0);
  }
  if(i%3){j=i%6;preferred=neighbors[j];source_preferred=worlds+j;}
  reference=actor;expected=original(&reference,raw,preferred,excluded);
  if(!bh_route_choose_exit(&actor,&room,source_preferred,excluded,&selected)||selected!=expected||memcmp((char*)&actor+0x1e8,(char*)&reference+0x1e8,4)){
   if(bad<4)printf("decision %lu original%ld source%ld\n",i,expected,selected);++bad;
  }
 }
 printf("route_decisions=65536 exit_or_rng_differences=%lu\n",bad);VirtualFree(code,0,MEM_RELEASE);return bad?1:0;
}
