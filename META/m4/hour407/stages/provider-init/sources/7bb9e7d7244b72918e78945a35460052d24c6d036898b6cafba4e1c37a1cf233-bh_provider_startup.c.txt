#include "bh_provider_startup.h"
#include <objbase.h>
#include <stddef.h>
typedef char native_peer_methods[offsetof(IDirectPlay3Vtbl,EnumConnections)==0x8c&&offsetof(IDirectPlay3Vtbl,InitializeConnection)==0x98&&offsetof(IDirectPlay3Vtbl,GetPlayerAddress)==0x48?1:-1];
typedef char native_lobby_method[offsetof(IDirectPlayLobby2Vtbl,EnumAddress)==0x14?1:-1];
int __cdecl bh_provider_startup(BHProviderStartup *state){
    static const GUID lobby_class={0x2fe8f810,0xb2a5,0x11d0,{0xa7,0x87,0,0,0xf8,3,0xab,0xfc}};
    static const GUID lobby_interface={0x1bb4af80,0xa303,0x11d0,{0x9c,0x4f,0,0xa0,0xc9,5,0x42,0x5e}};
    static const GUID application={0xfe8d1c00,0xabfa,0x11d2,{0x96,0x35,0,0x20,0x18,0x2a,0x83,0x3a}};
    BHProviderCatalog *catalog;DWORD size=0;long i;void *address;
    if(!state||!(catalog=state->catalog)||!state->modems||!catalog->allocator.allocate||!catalog->allocator.release||catalog->count||state->peer||state->lobby||state->com_initialized)return 0;
    catalog->count=0;state->modems->count=0;
    state->com_result=CoInitialize(NULL);state->com_initialized=SUCCEEDED(state->com_result);
    ++state->creations;state->peer_result=CoCreateInstance(&CLSID_DirectPlay,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectPlay3A,(void**)&state->peer);
    if(!state->peer)return 1;
    ++state->creations;state->lobby_result=CoCreateInstance(&lobby_class,NULL,CLSCTX_INPROC_SERVER,&lobby_interface,(void**)&state->lobby);
    ++state->enumerations;state->enumeration_result=IDirectPlay3_EnumConnections(state->peer,&application,bh_provider_catalog_callback,catalog,DPCONNECTION_DIRECTPLAY);
    for(i=0;i<catalog->count;++i)if(catalog->records[i].kind==2){
        ++state->initializations;state->initialization_result=IDirectPlay3_InitializeConnection(state->peer,catalog->records[i].connection,0);
        if(state->initialization_result==DP_OK)++state->successful_initializations;
    }
    ++state->address_queries;state->address_result=IDirectPlay3_GetPlayerAddress(state->peer,0,NULL,&size);
    if(state->address_result==DPERR_BUFFERTOOSMALL){
        address=catalog->allocator.allocate(size,catalog->allocator.user);
        /* Original assumes allocation and lobby creation succeed. These two
           failure-domain guards prevent NULL calls without inventing results. */
        if(address){
            ++state->address_queries;state->address_result=IDirectPlay3_GetPlayerAddress(state->peer,0,address,&size);
            if(state->address_result==DP_OK&&state->lobby){
                ++state->address_enumerations;state->address_enumeration_result=IDirectPlayLobby_EnumAddress(state->lobby,bh_modem_address_element,address,size,state->modems);
            }
            catalog->allocator.release(address,catalog->allocator.user);
        }
    }
    if(state->lobby){++state->releases;IDirectPlayLobby_Release(state->lobby);}state->lobby=NULL;
    if(state->peer){++state->releases;IDirectPlay3_Release(state->peer);}state->peer=NULL;
    return 1;
}
