#ifndef BH_NETWORK_SESSIONS_H
#define BH_NETWORK_SESSIONS_H
#include "bh_provider_catalog.h"
#include <dplobby.h>
typedef struct BHSessionRecord {GUID instance;char name[32];} BHSessionRecord;
typedef struct BHNetworkSessions {
 LPDIRECTPLAY3A peer;LPDIRECTPLAYLOBBY2A lobby;
 BHSessionRecord records[8];long count;int enumerating;
 HRESULT peer_result,lobby_result,address_result,initialization_result,enumeration_result;
 unsigned long address_calls,enum_calls,releases;
} BHNetworkSessions;
/* Explicit original global ownership. Initialize zeroed; caller owns COM.
   Provider reserved word is the original owned compound-address pointer.
   Kind0..4 and bounded NUL-terminated strings required. Native COM methods
   remain real; failures are recorded. No sessions/players are created here. */
int __cdecl bh_network_address(BHNetworkSessions *,BHProviderRecord *); /*476070*/
BOOL FAR PASCAL bh_network_session_callback(LPCDPSESSIONDESC2,LPDWORD,DWORD,LPVOID); /*476360*/
BOOL FAR PASCAL bh_network_session_continue(LPCDPSESSIONDESC2,LPDWORD,DWORD,LPVOID); /*4763f0*/
int __cdecl bh_network_connection(BHNetworkSessions *,BHProviderRecord *); /*476410*/
int __cdecl bh_network_sessions_refresh(BHNetworkSessions *,const BHProviderRecord *); /*476560*/
void __cdecl bh_network_connection_dispose(BHNetworkSessions *); /*476620*/
void __cdecl bh_network_address_dispose(BHProviderRecord *); /*count-zero ownership*/
#endif
