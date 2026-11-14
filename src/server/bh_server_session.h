#ifndef BH_SERVER_SESSION_H
#define BH_SERVER_SESSION_H

#include "../Network/bh_network_game.h"

typedef struct BHServerSession
{
    BHNetworkGame game;
    BHNetworkMessage world_message;
    BHNetworkRoster world_roster;
    void *world;
    uint16_t tick, scores[BH_NETWORK_PLAYERS];
    float latency[BH_NETWORK_PLAYERS];
    int failed;
} BHServerSession;

/* The world owner supplies the identity of its loaded level and receives gameplay and roster events. */
int bh_server_session_open(BHServerSession *, unsigned short port, const BHLevelIdentity *,
                           BHNetworkMessage, BHNetworkRoster, void *world);
int bh_server_session_poll(BHServerSession *);
/* Advance the session clock once per 30 Hz simulation step. */
int bh_server_session_tick(BHServerSession *);
void bh_server_session_close(BHServerSession *);

#endif
