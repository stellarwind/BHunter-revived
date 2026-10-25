#ifndef BH_NETWORK_TRANSPORT_H
#define BH_NETWORK_TRANSPORT_H
#include <windows.h>
#include <dplay.h>
#include "bh_network_codec.h"
typedef struct BHNetworkTransport {
    LPDIRECTPLAY3A peer;
    DPID local_player;
    HRESULT last_hresult;
    unsigned long native_calls;
    DPID last_recipient;
    DWORD last_flags,last_size;
} BHNetworkTransport;
/* Fresh source context owns one AddRef on the supplied real DirectPlay3A
   interface. This does not initialize a provider, open a session, or create a
   player. The caller supplies the original source DPID binding explicitly. */
int __cdecl bh_network_transport_init(BHNetworkTransport *transport,LPDIRECTPLAY3A peer,DPID local_player);
int __cdecl bh_network_transport_dispose(BHNetworkTransport *transport);
/* Complete 00476d50. Forward from/to/data/size to real SDK Send; any nonzero
   guaranteed argument maps to flags 1, otherwise 0. NULL peer performs no call.
   HRESULT/last-call diagnostics are source extensions: S_FALSE means no peer,
   E_INVALIDARG means invalid context; original callers ignore Send HRESULT.
   Bytes are borrowed for the synchronous call only; no queue/retry is added. */
HRESULT __cdecl bh_network_send(BHNetworkTransport *transport,DPID recipient,
    void *bytes,DWORD size,int guaranteed);
/* Complete 00423710 over an explicit already-begun writer: finish payload
   length/padding/XOR, then send the UNPADDED length. Writer cursor includes pad.
   Uses real bh_network_send; no transport implementation is substituted.
   Source framing guards (<=255 payload, capacity, no repeated finish) apply. */
HRESULT __cdecl bh_network_packet_send(BHNetworkTransport *transport,BHPacketWriter *writer,
    DPID recipient,int guaranteed);
#endif
