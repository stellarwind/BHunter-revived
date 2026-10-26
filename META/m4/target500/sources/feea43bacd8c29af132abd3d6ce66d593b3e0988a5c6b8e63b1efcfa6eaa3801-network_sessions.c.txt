#include "../src/bh_network_sessions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <objbase.h>
static int checks,failures;
static void check(int v,const char *n){++checks;if(!v){++failures;printf("FAIL network sessions %s\n",n);}}
typedef struct Elements {int count;GUID types[8];DWORD sizes[8];unsigned char bytes[8][160];} Elements;
static BOOL CALLBACK inspect(const GUID *id,DWORD n,const void *bytes,void *user){Elements *e=(Elements*)user;int at=e->count++;if(at<8){e->types[at]=*id;e->sizes[at]=n;if(n<=160)memcpy(e->bytes[at],bytes,n);}return TRUE;}
int bh_test_network_sessions(void){BHNetworkSessions s;BHProviderRecord p;DPSESSIONDESC2 d;Elements e;HRESULT hr;int i,k;DWORD timeout=77;static const GUID cls={0x2fe8f810,0xb2a5,0x11d0,{0xa7,0x87,0,0,0xf8,3,0xab,0xfc}};static const GUID iid={0x1bb4af80,0xa303,0x11d0,{0x9c,0x4f,0,0xa0,0xc9,5,0x42,0x5e}};
 checks=failures=0;memset(&s,0,sizeof(s));memset(&p,0,sizeof(p));memset(&d,0,sizeof(d));d.dwSize=sizeof(d);d.lpszSessionNameA="aBc Session";d.guidInstance.Data1=0x12345678;
 check(!bh_network_session_callback(NULL,&timeout,0,&s)&&s.count==0,"NULL callback stop");
 check(!bh_network_session_callback(&d,&timeout,1,&s)&&s.count==0,"timeout stop");
 for(i=0;i<8;++i){d.guidInstance.Data2=(WORD)i;check(bh_network_session_callback(&d,&timeout,0,&s)==(i<7),"eight callback continuation");check(s.count==i+1&&!strcmp(s.records[i].name,"ABC SESSION")&&s.records[i].instance.Data2==i,"name and GUID");}
 check(!bh_network_session_callback(&d,&timeout,0,&s)&&s.count==8&&timeout==77,"full guard no timeout write");
 check(bh_network_session_continue(&d,&timeout,0,NULL)&&!bh_network_session_continue(NULL,&timeout,0,NULL)&&!bh_network_session_continue(&d,&timeout,1,NULL)&&bh_network_session_continue(&d,&timeout,3,NULL),"continuation exact flag equality");
 memset(&s,0,sizeof(s));check(bh_network_address(&s,&p)&&!p.reserved,"absent lobby clears address");check(!bh_network_sessions_refresh(&s,&p),"absent peer refresh");bh_network_connection_dispose(&s);check(!s.releases,"absent peer dispose");
 hr=CoInitialize(NULL);check(SUCCEEDED(hr),"real COM initialization");
 if(SUCCEEDED(hr)){
  s.lobby_result=CoCreateInstance(&cls,NULL,1,&iid,(void**)&s.lobby);check(s.lobby_result==S_OK&&s.lobby!=NULL,"native lobby creation");
  if(s.lobby){
   for(k=0;k<5;++k){p.kind=k;memset(p.data,0,sizeof(p.data));if(k==1)strcpy((char*)p.data,"127.0.0.1");if(k==2){strcpy((char*)p.data,"MODEM");strcpy((char*)p.data+96,"123");}if(k==0||k==3){DWORD serial[5]={1,9600,0,0,0};memcpy(p.data,serial,20);}p.provider.Data1=0x12345678;
    check(bh_network_address(&s,&p),"compound construction invoked");check(s.address_result==S_OK&&p.reserved,"native compound allocation");
    if(p.reserved){DWORD size;memcpy(&size,(unsigned char*)p.reserved+20,4);memset(&e,0,sizeof(e));hr=IDirectPlayLobby_EnumAddress(s.lobby,inspect,(void*)p.reserved,size,&e);
     check(hr==S_OK,"native address decode");check(e.count==(k==0||k==2?4:k==4?2:3),"exact original element populations including kind0 fallthrough");
     if(k==1||k==2){int found=0;for(i=0;i<e.count&&i<8;++i)if(e.sizes[i]==(DWORD)(lstrlenA((char*)p.data)+1)&&!memcmp(e.bytes[i],p.data,e.sizes[i]))found=1;check(found,"native address retains requested string");}
    }
    bh_network_address_dispose(&p);check(!p.reserved,"owned compound dispose");
   }
   IDirectPlayLobby_Release(s.lobby);s.lobby=NULL;
  }
  memset(&s,0,sizeof(s));memset(&p,0,sizeof(p));check(!bh_network_connection(&s,&p)&&s.peer!=NULL&&FAILED(s.initialization_result),"connection creates actual peer and retains native provider error");
  if(s.peer){s.count=3;p.kind=2;check(bh_network_sessions_refresh(&s,&p)&&s.count==3&&!s.enum_calls,"empty modem skips enumeration");p.kind=1;check(bh_network_sessions_refresh(&s,&p)&&s.enum_calls==1&&s.enumerating&&s.count==3&&FAILED(s.enumeration_result),"uninitialized native enumeration failure preserves count");bh_network_connection_dispose(&s);check(!s.peer&&!s.lobby&&!s.enumerating&&s.releases==2&&s.enum_calls==2,"real stop/release ownership");}
  CoUninitialize();
 }
 printf("{\"scenario\":\"m4-network-sessions\",\"checks\":%d,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_NETWORK_SESSIONS_STANDALONE
int main(void){return bh_test_network_sessions();}
#endif
int bh_network_sessions_checkpoint(unsigned long *count){int result=bh_test_network_sessions();*count+=(unsigned long)checks;return result;}
