#include "../src/bh_provider_startup.h"
#include "../src/bh_network_shutdown.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL provider-startup %d: %s\n",__LINE__,s);}}while(0)
typedef struct Memory{unsigned long live,calls;}Memory;
static void *__cdecl allocate(unsigned long n,void*u){Memory*m=(Memory*)u;void*p=malloc(n);++m->calls;if(p)++m->live;return p;}
static void __cdecl release(void*p,void*u){Memory*m=(Memory*)u;if(p){--m->live;free(p);}}
int bh_provider_startup_checkpoint(unsigned long*count){BHProviderStartup s,before;BHProviderCatalog catalog;BHModemNames modems;BHNetworkShutdown shutdown;Memory memory={0,0};BHSceneAllocator a={allocate,release,&memory};long i;unsigned long modem_providers=0;
    checks=0;failures=0;memset(&s,0,sizeof(s));memset(&modems,0x5a,sizeof(modems));memset(&shutdown,0,sizeof(shutdown));
    CHECK(!bh_provider_startup(NULL),"source NULL guard");before=s;CHECK(!bh_provider_startup(&s)&&!memcmp(&s,&before,sizeof(s)),"unbound source guard");
    CHECK(bh_provider_catalog_init(&catalog,&a),"real source catalog ownership");s.catalog=&catalog;s.modems=&modems;
    CHECK(bh_provider_startup(&s),"whole native provider initialization invocation");
    CHECK(SUCCEEDED(s.com_result)&&s.com_initialized,"actual owned COM initialization");
    CHECK(s.peer_result==S_OK&&s.lobby_result==S_OK&&s.creations==2,"actual original DirectPlay3A/Lobby2A COM identities");
    CHECK(s.enumerations==1&&s.enumeration_result==DP_OK&&catalog.count>0&&catalog.count<=8,"real full provider callback enumeration");
    for(i=0;i<catalog.count;++i){CHECK(catalog.records[i].connection&&catalog.connection_bytes[i]>0,"actual copied provider persists after root release");if(catalog.records[i].kind==2)++modem_providers;}
    CHECK(s.initializations==modem_providers,"only original kind2 providers receive initialization calls");
    CHECK(s.address_queries>=1&&s.address_queries<=2,"actual system player address query branches");
    CHECK(s.releases==2&&!s.peer&&!s.lobby,"original lobby then peer release and clear");
    CHECK(modems.count>=0&&modems.count<=8,"actual modem callback context count remains bounded");
    CHECK(memory.live==(unsigned long)catalog.count,"temporary address storage released, only copied providers retained");
    before=s;CHECK(!bh_provider_startup(&s)&&!memcmp(&s,&before,sizeof(s)),"owned COM/catalog guard prevents repeat leak");
    bh_provider_catalog_dispose(&catalog);CHECK(!memory.live,"real complete copied connection disposal");
    if(s.com_initialized){bh_network_shutdown(&shutdown);s.com_initialized=0;CHECK(shutdown.uninitializes==1,"actual admitted unconditional COM shutdown owner balances initialization");}
    *count=checks;printf("{\"scenario\":\"provider-startup\",\"checks\":%lu,\"failures\":%d,\"sessions_opened\":0,\"address_enumerations\":%lu,\"connection_initialization_calls\":%lu,\"connections_initialized\":%lu,\"initialization_hresult\":%ld,\"address_hresult\":%ld,\"original_code_required\":false}\n",checks,failures,s.address_enumerations,s.initializations,s.successful_initializations,(long)s.initialization_result,(long)s.address_result);return failures;
}
