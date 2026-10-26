#include "bh_network_sessions.h"
#include <objbase.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
static const GUID app={0xfe8d1c00,0xabfa,0x11d2,{0x96,0x35,0,0x20,0x18,0x2a,0x83,0x3a}};
static const GUID lobby_class={0x2fe8f810,0xb2a5,0x11d0,{0xa7,0x87,0,0,0xf8,3,0xab,0xfc}};
static const GUID lobby_iid={0x1bb4af80,0xa303,0x11d0,{0x9c,0x4f,0,0xa0,0xc9,5,0x42,0x5e}};
static const GUID provider_id={0x07d916c0,0xe0af,0x11cf,{0x9c,0x4e,0,0xa0,0xc9,5,0x42,0x5e}};
static const GUID inet_id={0xc4a54da0,0xe0af,0x11cf,{0x9c,0x4e,0,0xa0,0xc9,5,0x42,0x5e}};
static const GUID modem_id={0xf6dcc200,0xa2fe,0x11d0,{0x9c,0x4f,0,0xa0,0xc9,5,0x42,0x5e}};
static const GUID phone_id={0x78ec89a0,0xe0af,0x11cf,{0x9c,0x4e,0,0xa0,0xc9,5,0x42,0x5e}};
static const GUID com_id={0xf2f0ce00,0xe0af,0x11cf,{0x9c,0x4e,0,0xa0,0xc9,5,0x42,0x5e}};
static const GUID providers[4]={
 {0x685bc400,0x9d2c,0x11cf,{0xa9,0xcd,0,0xaa,0,0x68,0x86,0xe3}},
 {0x36e95ee0,0x8577,0x11cf,{0x96,0x0c,0,0x80,0xc7,0x53,0x4e,0x82}},
 {0x44eaa760,0xcb68,0x11cf,{0x9c,0x4e,0,0xa0,0xc9,5,0x42,0x5e}},
 {0x0f1d6860,0x88d9,0x11cf,{0x9c,0x4e,0,0xa0,0xc9,5,0x42,0x5e}}};
typedef char session_native_layout[sizeof(DPSESSIONDESC2)==80&&sizeof(BHSessionRecord)==48&&offsetof(IDirectPlayLobby2Vtbl,CreateCompoundAddress)==0x38?1:-1];
typedef char provider_native_layout[sizeof(void*)==4&&offsetof(BHProviderRecord,reserved)==0x14&&offsetof(BHProviderRecord,data)==0x1c?1:-1];
void __cdecl bh_network_address_dispose(BHProviderRecord *p){if(p){free((void*)p->reserved);p->reserved=0;}}
static int text_ok(const char *p,unsigned long n){return memchr(p,0,n)!=NULL;}
static void element(DPCOMPOUNDADDRESSELEMENT *e,const GUID *id,DWORD size,void *data){e->guidDataType=*id;e->dwDataSize=size;e->lpData=data;}
int __cdecl bh_network_address(BHNetworkSessions *s,BHProviderRecord *p){DPCOMPOUNDADDRESSELEMENT e[4];DWORD count=0,size=0;void *address;
 if(!s||!p)return 0;bh_network_address_dispose(p);if(!s->lobby)return 1;
 if(p->kind<0||p->kind>4)return 0;
 if((p->kind==1||p->kind==2)&&!text_ok((char*)p->data,p->kind==1?64:96))return 0;
 if(p->kind==2&&!text_ok((char*)p->data+96,64))return 0;
 if(p->kind==0)element(e+count++,&provider_id,16,(void*)&providers[0]);
 if(p->kind==0||p->kind==3){element(e+count++,&provider_id,16,(void*)&providers[3]);element(e+count++,&com_id,20,p->data);}
 else if(p->kind==1){element(e+count++,&provider_id,16,(void*)&providers[1]);element(e+count++,&inet_id,lstrlenA((char*)p->data)+1,p->data);}
 else if(p->kind==2){element(e+count++,&provider_id,16,(void*)&providers[2]);element(e+count++,&modem_id,lstrlenA((char*)p->data)+1,p->data);element(e+count++,&phone_id,lstrlenA((char*)p->data+96)+1,p->data+96);}
 else if(p->kind==4)element(e+count++,&provider_id,16,&p->provider);
 ++s->address_calls;s->address_result=IDirectPlayLobby_CreateCompoundAddress(s->lobby,e,count,NULL,&size);
 if(s->address_result!=DPERR_BUFFERTOOSMALL)return 1;
 address=malloc(size);if(!address)return 0;
 ++s->address_calls;s->address_result=IDirectPlayLobby_CreateCompoundAddress(s->lobby,e,count,address,&size);
 if(s->address_result!=DP_OK)free(address);else p->reserved=(DWORD)address;return 1;
}
BOOL FAR PASCAL bh_network_session_callback(LPCDPSESSIONDESC2 d,LPDWORD timeout,DWORD flags,LPVOID user){BHNetworkSessions *s=(BHNetworkSessions*)user;BHSessionRecord *r;unsigned long i,n;(void)timeout;
 if(!d||flags==1)return FALSE;
 if(!s||s->count<0||s->count>=8||!d->lpszSessionNameA)return FALSE;
 n=lstrlenA(d->lpszSessionNameA);if(n>=32)return FALSE;r=s->records+s->count++;
 for(i=0;i<n;++i)r->name[i]=(char)toupper((signed char)d->lpszSessionNameA[i]);r->name[n]=0;r->instance=d->guidInstance;return s->count<8;
}
BOOL FAR PASCAL bh_network_session_continue(LPCDPSESSIONDESC2 d,LPDWORD timeout,DWORD flags,LPVOID user){(void)timeout;(void)user;return d&&flags!=1;}
static void description(DPSESSIONDESC2 *d){memset(d,0,sizeof(*d));d->dwSize=sizeof(*d);d->guidApplication=app;}
static void enumerate(BHNetworkSessions *s){DPSESSIONDESC2 d;long old=s->count;description(&d);s->count=0;s->enumerating=1;++s->enum_calls;
 s->enumeration_result=IDirectPlay3_EnumSessions(s->peer,&d,900,bh_network_session_callback,s,0x91);
 if(s->enumeration_result!=DP_OK)s->count=old;
}
int __cdecl bh_network_connection(BHNetworkSessions *s,BHProviderRecord *p){
 if(!s||!p||s->peer||s->lobby)return 0;s->count=0;
 s->peer_result=CoCreateInstance(&CLSID_DirectPlay,NULL,1,&IID_IDirectPlay3A,(void**)&s->peer);if(!s->peer)return 0;
 s->lobby_result=CoCreateInstance(&lobby_class,NULL,1,&lobby_iid,(void**)&s->lobby);
 if(!bh_network_address(s,p))return 0;
 s->initialization_result=IDirectPlay3_InitializeConnection(s->peer,p->reserved?(void*)p->reserved:p->connection,0);
 if(s->initialization_result!=DP_OK&&s->initialization_result!=DPERR_ALREADYINITIALIZED)return 0;
 if(p->kind==2&&!p->data[96])return 1;enumerate(s);return 1;
}
int __cdecl bh_network_sessions_refresh(BHNetworkSessions *s,const BHProviderRecord *p){if(!s||!p||!s->peer)return 0;if(p->kind==2&&!p->data[96])return 1;enumerate(s);return 1;}
void __cdecl bh_network_connection_dispose(BHNetworkSessions *s){DPSESSIONDESC2 d;if(!s||!s->peer)return;
 if(s->enumerating){description(&d);s->enumerating=0;++s->enum_calls;s->enumeration_result=IDirectPlay3_EnumSessions(s->peer,&d,10,bh_network_session_continue,s,0xa1);}
 if(s->lobby){++s->releases;IDirectPlayLobby_Release(s->lobby);}s->lobby=NULL;
 if(s->peer){++s->releases;IDirectPlay3_Release(s->peer);}s->peer=NULL;
}
