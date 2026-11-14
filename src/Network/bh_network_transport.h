#ifndef BH_NETWORK_TRANSPORT_H
#define BH_NETWORK_TRANSPORT_H
#include "bh_network_codec.h"

typedef struct BHNetworkGame BHNetworkGame;
typedef struct BHNetworkTransport
{
    BHNetworkGame *game;
} BHNetworkTransport;

/* Recipient zero broadcasts. The server address maps to player one in listen games. */
enum { BH_NETWORK_SERVER = 9 };

int bh_network_send(BHNetworkTransport *, unsigned long recipient, const void *, unsigned long size, int reliable);
int bh_network_packet_send(BHNetworkTransport *, BHPacketWriter *, unsigned long recipient, int reliable);
#endif
