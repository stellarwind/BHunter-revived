#ifndef BH_NETWORK_TRANSPORT_H
#define BH_NETWORK_TRANSPORT_H
#include "bh_network_codec.h"

typedef struct BHNetworkGame BHNetworkGame;
typedef struct BHNetworkTransport
{
    BHNetworkGame *game;
} BHNetworkTransport;

/* Recipient zero broadcasts. Recipient one is the listen host. */
int bh_network_send(BHNetworkTransport *, unsigned long recipient, const void *, unsigned long size, int reliable);
int bh_network_packet_send(BHNetworkTransport *, BHPacketWriter *, unsigned long recipient, int reliable);
#endif
