#include "bh_network_shutdown.h"
void __cdecl bh_network_shutdown(BHNetworkShutdown *c)
{
    if (!c)
        return;
    if (c->first)
    {
        ++c->first_releases;
        c->first->lpVtbl->Release(c->first);
    }
    c->first = NULL;
    if (c->second)
    {
        ++c->second_releases;
        c->second->lpVtbl->Release(c->second);
    }
    c->second = NULL;
    ++c->uninitializes;
    CoUninitialize();
}
