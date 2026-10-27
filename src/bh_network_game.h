#ifndef BH_NETWORK_GAME_H
#define BH_NETWORK_GAME_H
#include "bh_network_sessions.h"
typedef struct BHNetworkPlayer
{
    long active;
    DPID id;
    char name[32];
} BHNetworkPlayer;
typedef void(__cdecl *BHNetworkMessage)(void *, DWORD, void *);
typedef struct BHNetworkGame
{
    BHNetworkSessions *sessions;
    BHSceneAllocator allocator;
    BHNetworkPlayer players[8];
    long player_count;
    DPID local_player;
    void *buffer;
    DWORD capacity;
    HRESULT open_result, player_result, close_result, receive_result;
    unsigned long opens, creations, closes, receives, callbacks;
    int error;
} BHNetworkGame;
/* Source ownership adapter; context owns only buffer, borrows actual peer.
   COM/session interfaces remain owned by BHNetworkSessions. No Send or input.
   Names <=31bytes plus NUL; callback may read/mutate current packet but cannot
   dispose/rebind peer or allocator during a receive drain. */
int __cdecl bh_network_game_init(BHNetworkGame *, BHNetworkSessions *, const BHSceneAllocator *);
void __cdecl bh_network_game_dispose(BHNetworkGame *);
int __cdecl bh_network_host(BHNetworkGame *, const char *);                /*4766e0*/
int __cdecl bh_network_join(BHNetworkGame *, const GUID *, const char *);  /*4768c0*/
int __cdecl bh_network_receive(BHNetworkGame *, BHNetworkMessage, void *); /*476a20*/
#endif
