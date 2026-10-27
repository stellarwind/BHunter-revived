#ifndef BH_PROVIDER_STARTUP_H
#define BH_PROVIDER_STARTUP_H
#include "bh_provider_catalog.h"
#include "bh_serial_options.h"
#include <dplobby.h>
typedef struct BHProviderStartup
{
    BHProviderCatalog *catalog;
    BHModemNames *modems;
    LPDIRECTPLAY3A peer;
    LPDIRECTPLAYLOBBY2A lobby;
    int com_initialized;
    HRESULT com_result, peer_result, lobby_result, enumeration_result;
    HRESULT initialization_result, address_result, address_enumeration_result;
    unsigned long creations, enumerations, initializations, successful_initializations, address_queries,
        address_enumerations, releases;
} BHProviderStartup;
/* Whole00475ef0. Fresh initialized catalog, modem context and zeroed startup
   state required. Enumerates actual providers, initializes original kind2
   modem connection(s), queries system player address and uses actual admitted
   modem-address callback when available. No sessions/player/game creation.
   Both created interfaces are released/cleared before return, as original.
   Successful CoInitialize remains owned by caller until actual source network
   shutdown on the same thread. Native errors remain recorded and are ignored
   by original sequencing; return1 means valid invocation, not network success.
   Caller disposes provider blobs independently of native COM teardown. */
int __cdecl bh_provider_startup(BHProviderStartup *);
#endif
