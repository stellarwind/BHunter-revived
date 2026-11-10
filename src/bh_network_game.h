#ifndef BH_NETWORK_GAME_H
#define BH_NETWORK_GAME_H
#include "bh_network_transport.h"
#include <stdint.h>

enum
{
    BH_NETWORK_PLAYERS = 8,
    BH_NETWORK_CARS = 3,
    BH_NETWORK_ROSTER_BYTES = 26,
    BH_NETWORK_POSE_BYTES = 22,
    BH_NETWORK_PACKET_CAPACITY = 264,
    BH_NETWORK_SCORE_HISTORY = 128,
    BH_LEVEL_NAME_CAPACITY = 49,
    BH_NETWORK_PORT = 27890,
    BH_NETWORK_DEATH_EFFECT = 13,
    BH_NETWORK_DEATH_FIRST = 0,
    BH_NETWORK_DEATH_FINAL = 1
};

typedef struct BHLevelIdentity
{
    char name[BH_LEVEL_NAME_CAPACITY];
    uint64_t fingerprint;
} BHLevelIdentity;

typedef struct BHNetworkScoreReport
{
    uint32_t received_at;
    uint16_t frame;
} BHNetworkScoreReport;

typedef struct BHNetworkPlayer
{
    int active;
    unsigned long id;
    unsigned char car, pose_propulsion;
    char name[32];
    struct _ENetPeer *peer;
    BHNetworkScoreReport score_history[BH_NETWORK_SCORE_HISTORY];
    unsigned int score_history_count, next_score_report;
} BHNetworkPlayer;

struct BHNetworkGame
{
    struct _ENetHost *host;
    struct _ENetPeer *server;
    BHNetworkPlayer players[8];
    uint32_t handshake_deadline[BH_NETWORK_PLAYERS - 1]; /* Indexed by ENet peer, before roster assignment. */
    unsigned char level_handshake[BH_NETWORK_PLAYERS - 1];
    BHLevelIdentity level;
    unsigned long local_player;
    unsigned char local_car, server_pose_propulsion;
    unsigned long sent[BH_NETWORK_DEATH_EFFECT + 1], received[BH_NETWORK_DEATH_EFFECT + 1], rejected;
    int initialized, hosting, disconnected, welcome_timed_out, level_ready;
};

typedef void (*BHNetworkMessage)(void *, unsigned long, void *);
typedef void (*BHNetworkRoster)(unsigned int slot, void *);

int bh_network_shot_pattern_valid(unsigned int type, unsigned int count, unsigned int spacing);
int bh_level_name_valid(const char *name);
int bh_network_open(BHNetworkGame *, const char *address, unsigned short port, const char *name, unsigned int car);
int bh_network_level_ready(BHNetworkGame *, uint64_t fingerprint);
void bh_network_close(BHNetworkGame *);
void bh_network_receive(BHNetworkGame *, BHNetworkMessage, BHNetworkRoster, void *);
#endif
