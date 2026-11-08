#ifndef BH_NETWORK_GAME_H
#define BH_NETWORK_GAME_H
#include "bh_network_transport.h"

enum
{
    BH_NETWORK_PLAYERS = 8,
    BH_NETWORK_CARS = 3,
    BH_NETWORK_ROSTER_BYTES = 26,
    BH_NETWORK_PACKET_CAPACITY = 264,
    BH_NETWORK_PORT = 27890,
    BH_NETWORK_DEATH_EFFECT = 13,
    BH_NETWORK_DEATH_FIRST = 0,
    BH_NETWORK_DEATH_FINAL = 1
};

typedef struct BHNetworkPlayer
{
    int active;
    unsigned long id;
    unsigned char car;
    char name[32];
    struct _ENetPeer *peer;
} BHNetworkPlayer;

struct BHNetworkGame
{
    struct _ENetHost *host;
    struct _ENetPeer *server;
    BHNetworkPlayer players[8];
    unsigned long local_player;
    unsigned char local_car;
    unsigned long sent[BH_NETWORK_DEATH_EFFECT + 1], received[BH_NETWORK_DEATH_EFFECT + 1], rejected;
    int initialized, hosting, disconnected;
};

typedef void (*BHNetworkMessage)(void *, unsigned long, void *);
typedef void (*BHNetworkRoster)(unsigned int slot, void *);

int bh_network_open(BHNetworkGame *, const char *address, unsigned short port, const char *name, unsigned int car);
void bh_network_close(BHNetworkGame *);
void bh_network_receive(BHNetworkGame *, BHNetworkMessage, BHNetworkRoster, void *);
#endif
