#include "../src/bh_provider_catalog.h"
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL provider-catalog %d: %s\n",__LINE__,s);}}while(0)
typedef struct Memory{int fail;unsigned long live;}Memory;
static void *__cdecl allocate(unsigned long n,void *u){Memory*m=(Memory*)u;void*p;if(m->fail)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void*p,void*u){Memory*m=(Memory*)u;if(p){--m->live;free(p);}}
static void memory_cases(void){
    BHProviderCatalog c,before;DPNAME name;unsigned char bytes[257];GUID guid;unsigned long i,n;char long_name[]="abcdefghijklmnopqrstEXTRA";Memory memory={0,0};BHSceneAllocator a={allocate,release,&memory};
    static const unsigned char ids[4][16]={{0x60,0xa7,0xea,0x44,0x68,0xcb,0xcf,0x11,0x9c,0x4e,0,0xa0,0xc9,5,0x42,0x5e},{0xe0,0x5e,0xe9,0x36,0x77,0x85,0xcf,0x11,0x96,0x0c,0,0x80,0xc7,0x53,0x4e,0x82},{0,0xc4,0x5b,0x68,0x2c,0x9d,0xcf,0x11,0xa9,0xcd,0,0xaa,0,0x68,0x86,0xe3},{0x60,0x68,0x1d,0x0f,0xd9,0x88,0xcf,0x11,0x9c,0x4e,0,0xa0,0xc9,5,0x42,0x5e}};
    static const long kinds[4]={2,1,0,3};static const char*labels[4]={"Modem-to-Modem","TCP/IP Internet","IPX Local Network","Serial Cable"};
    for(i=0;i<sizeof(bytes);++i)bytes[i]=(unsigned char)(i*19+7);
    memset(&name,0,sizeof(name));name.dwSize=sizeof(name);name.lpszShortNameA=long_name;
    CHECK(bh_provider_catalog_init(&c,&a),"real catalog owner");memset(c.records,0xa5,sizeof(c.records));
    for(n=0;n<8;++n){BHProviderRecord *record;unsigned char expected[188];BOOL result;
        if(n<4)memcpy(&guid,ids[n],16);else memset(&guid,(int)n,sizeof(guid));
        before=c;result=bh_provider_catalog_callback(&guid,bytes,n*31+1,&name,0x12345678,&c);record=&c.records[n];
        CHECK(result==(n<7)&&c.count==(long)n+1,"callback actual continue/stop at eight");
        CHECK(record->connection&&c.connection_bytes[n]==n*31+1&&!memcmp(record->connection,bytes,n*31+1),"owned exact byte copy across unaligned lengths");
        CHECK(!memcmp(&record->provider,&guid,16)&&!record->reserved&&record->kind==(n<4?kinds[n]:4),"exact GUID/type mapping and zero original reserved word");
        CHECK(!strncmp((char*)record+156,n<4?labels[n]:long_name,n<4?strlen(labels[n]):20)&&*((char*)record+176)==0,"original known labels and unknown20-byte termination");
        memcpy(expected,&before.records[n],188);memcpy(expected,&guid,16);memcpy(expected+16,&record->connection,4);memset(expected+20,0,4);memcpy(expected+24,&record->kind,4);strncpy((char*)expected+156,long_name,20);expected[176]=0;if(n<4)strcpy((char*)expected+156,labels[n]);
        CHECK(!memcmp(record,expected,188),"every other profile byte retained including overlapping string storage");
        for(i=0;i<8;++i)if(i!=n)CHECK(!memcmp(&c.records[i],&before.records[i],188),"other provider slots untouched");
    }
    before=c;CHECK(!bh_provider_catalog_callback(&guid,bytes,1,&name,0,&c)&&!memcmp(&c,&before,sizeof(c)),"full source guard does not leak/corrupt");
    bh_provider_catalog_dispose(&c);CHECK(!memory.live&&!c.allocator.allocate,"real complete blob ownership disposal");
    CHECK(bh_provider_catalog_init(&c,&a),"fresh failure owner");memory.fail=1;
    CHECK(!bh_provider_catalog_callback(&guid,bytes,1,&name,0,&c)&&!c.count&&c.allocation_failed&&!memory.live,"source failed allocation rejects without partial record");
    bh_provider_catalog_dispose(&c);
}
static void native_cases(void){BHProviderCatalog c;LPDIRECTPLAY3A peer=NULL;HRESULT hr;long i;
    hr=CoInitialize(NULL);CHECK(SUCCEEDED(hr),"real COM apartment");if(FAILED(hr))return;
    hr=CoCreateInstance(&CLSID_DirectPlay,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectPlay3A,(void**)&peer);
    CHECK(hr==DP_OK&&peer,"real unopened DirectPlay3A owner");if(!peer){CoUninitialize();return;}
    CHECK(bh_provider_catalog_init(&c,NULL),"actual native catalog context");
    hr=IDirectPlay3_EnumConnections(peer,NULL,bh_provider_catalog_callback,&c,DPCONNECTION_DIRECTPLAY);
    CHECK(hr==DP_OK&&c.count>0&&c.count<=8&&!c.allocation_failed,"actual native EnumConnections registers full WINAPI callback");
    for(i=0;i<c.count;++i)CHECK(c.records[i].connection&&c.connection_bytes[i]>0&&*((char*)&c.records[i]+176)==0,"native copied connection and bounded display label");
    bh_provider_catalog_dispose(&c);CHECK(IDirectPlay3_Release(peer)==0,"native root final release");CoUninitialize();
}
int bh_provider_catalog_checkpoint(unsigned long*count){checks=0;failures=0;memory_cases();native_cases();*count=checks;printf("{\"scenario\":\"provider-catalog\",\"checks\":%lu,\"failures\":%d,\"sessions_opened\":0,\"connections_initialized\":0,\"original_code_required\":false}\n",checks,failures);return failures;}
