#ifndef BH_NETWORK_SHUTDOWN_H
#define BH_NETWORK_SHUTDOWN_H
#include <windows.h>
#include <objbase.h>
/* Explicit owning reference slots. Only IUnknown::Release is required by this
   owner; no speculative derived interface/vtable layout is imposed. COM must
   have a matching successful initialization on the calling thread for EACH
   call, including the all-NULL path. */
typedef struct BHNetworkShutdown {
    IUnknown *first;  /* original004effec */
    IUnknown *second; /* original004effe8 */
    unsigned long first_releases,second_releases,uninitializes;
} BHNetworkShutdown;
/* Complete00476030: Release first if non-NULL, clear first, Release second if
   non-NULL, clear second, then unconditional CoUninitialize. This owner does
   not close sessions or destroy players; those belong to other functions. */
void __cdecl bh_network_shutdown(BHNetworkShutdown *);
#endif
