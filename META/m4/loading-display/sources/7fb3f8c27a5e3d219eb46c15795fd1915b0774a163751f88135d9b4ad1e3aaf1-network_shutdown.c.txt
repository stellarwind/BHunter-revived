#include "../src/bh_network_shutdown.h"
#include "../src/bh_network_transport.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL network-shutdown: %s\n",s);}}while(0)
int bh_network_shutdown_checkpoint(unsigned long *count){
    BHNetworkShutdown state;LPDIRECTPLAY3A first=NULL,second=NULL;HRESULT hr;unsigned long mask;
    checks=failures=0;memset(&state,0,sizeof(state));
    for(mask=0;mask<4;++mask){
        hr=CoInitialize(NULL);CHECK(SUCCEEDED(hr),"successful same-thread COM initialization matched by owner");if(FAILED(hr))break;
        hr=CoInitialize(NULL);CHECK(hr==S_FALSE,"retained native control keeps a second balanced COM initialization during source cleanup");
        if(mask&1){hr=CoCreateInstance(&CLSID_DirectPlay,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectPlay3A,(void**)&first);
            CHECK(SUCCEEDED(hr)&&first,"actual first unopened DirectPlay interface");
            if(first){state.first=(IUnknown*)first;IDirectPlay3_AddRef(first);}}
        if(mask&2){hr=CoCreateInstance(&CLSID_DirectPlay,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectPlay3A,(void**)&second);
            CHECK(SUCCEEDED(hr)&&second,"actual second unopened DirectPlay interface");
            if(second){state.second=(IUnknown*)second;IDirectPlay3_AddRef(second);}}
        bh_network_shutdown(&state);
        CHECK(!state.first&&!state.second&&state.uninitializes==mask+1,"both slots clear before unconditional same-thread COM cleanup");
        if(first){CHECK(IDirectPlay3_Release(first)==0,"first retained native control observes exactly one source Release");first=NULL;}
        if(second){CHECK(IDirectPlay3_Release(second)==0,"second retained native control observes exactly one source Release");second=NULL;}
        CoUninitialize();
    }
    CHECK(state.first_releases==2&&state.second_releases==2&&state.uninitializes==4,
          "all four optional-interface combinations preserve release and unconditional uninitialize counts");
    if(count)*count=checks;
    printf("{\"scenario\":\"network-shutdown\",\"checks\":%lu,\"failures\":%lu,\"sessions_opened\":0,\"connections_initialized\":0,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_NETWORK_SHUTDOWN_STANDALONE
int main(void){return bh_network_shutdown_checkpoint(NULL);}
#endif
