#include <windows.h>
#include <initguid.h>
#include "bh_network_transport.h"
#include <stddef.h>
#include <string.h>

typedef char directplay_send_slot[offsetof(IDirectPlay3Vtbl, Send) == 0x68 ? 1 : -1];
typedef char directplay_guaranteed[DPSEND_GUARANTEED == 1 && sizeof(void *) == 4 ? 1 : -1];
int __cdecl bh_network_transport_init(BHNetworkTransport *t, LPDIRECTPLAY3A peer, DPID local)
{
    if (!t)
        return 0;
    memset(t, 0, sizeof(*t));
    t->peer = peer;
    t->local_player = local;
    if (peer)
        IDirectPlay3_AddRef(peer);
    return 1;
}
int __cdecl bh_network_transport_dispose(BHNetworkTransport *t)
{
    if (!t)
        return 0;
    if (t->peer)
        IDirectPlay3_Release(t->peer);
    memset(t, 0, sizeof(*t));
    return 1;
}
HRESULT __cdecl bh_network_send(BHNetworkTransport *t, DPID recipient, void *bytes, DWORD size, int guaranteed)
{
    if (!t)
        return E_INVALIDARG;
    if (!t->peer)
    {
        t->last_hresult = S_FALSE;
        return S_FALSE;
    }
    t->last_recipient = recipient;
    t->last_flags = guaranteed ? DPSEND_GUARANTEED : 0;
    t->last_size = size;
    ++t->native_calls;
    t->last_hresult = IDirectPlay3_Send(t->peer, t->local_player, recipient, t->last_flags, bytes, size);
    return t->last_hresult;
}
HRESULT __cdecl bh_network_packet_send(BHNetworkTransport *t, BHPacketWriter *writer, DPID recipient, int guaranteed)
{
    unsigned long wire_size;
    if (!t || !writer)
        return E_INVALIDARG;
    if (!bh_packet_finish(writer, &wire_size))
    {
        t->last_hresult = E_INVALIDARG;
        return E_INVALIDARG;
    }
    return bh_network_send(t, recipient, writer->data, wire_size, guaranteed);
}
