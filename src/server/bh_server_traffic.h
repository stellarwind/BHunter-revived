#ifndef BH_SERVER_TRAFFIC_H
#define BH_SERVER_TRAFFIC_H

#include "bh_server_session.h"

typedef struct BHServerLauncher BHServerLauncher;
typedef struct BHServerTraffic
{
    BHServerSession *session;
    BHServerLauncher *launchers;
    unsigned int count;
    uint32_t seed;
} BHServerTraffic;

int bh_server_traffic_load(BHServerTraffic *, BHServerSession *, const BHLevelIdentity *);
void bh_server_traffic_roster(unsigned int slot, void *);
void bh_server_traffic_close(BHServerTraffic *);

#endif
