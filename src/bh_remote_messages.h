#ifndef BH_REMOTE_MESSAGES_H
#define BH_REMOTE_MESSAGES_H
#include "bh_remote_position.h"
#include "bh_network_scores.h"
#include "bh_network_effect_receive.h"
#include "bh_actor_launch.h"
#include "bh_messages.h"
typedef struct BHRemoteMessages
{
    BHRemotePackets *packets;
    BHNetworkScores *scores;
    BHMessageState *messages;
    BHNetworkTransport *transport;
    BHSoundQueue *sound;
    BHContactEffects *contacts;
    BHNetworkEffectReceive *effects;
    BHActorLaunchContext *launch;
    float *latency;
    unsigned long *peers;
    unsigned char outgoing[64];
    BHPacketWriter writer;
    int send_result;
    unsigned long sends, sounds, sparks, departures;
    int error;
} BHRemoteMessages;
/* Whole00427d30 host and00428750 client. Session roster notifications, actual
   linked remote allocation/removal, reliable announcements, clock/latency,
   lag-gated sound/sparks/damage, shot/snapshot/effect/actor source receivers
   and score deltas/snapshots retain original routing and counter quirks.
   Output packet storage is diagnostic; Send uses the actual source transport.
   All borrowed contexts, actor tables, rooms and roster names survive calls.
   Bounds guards replace native event/packet overreads and occupied-slot leaks.
   The portable clock packet includes all eight slots. PC53 finite moderate state. */
int bh_remote_roster(BHRemoteMessages *, unsigned int slot, int host);
int bh_remote_clear(BHRemoteMessages *);
int __cdecl bh_remote_host_message(BHRemoteMessages *, void *, unsigned long size, unsigned long capacity);
int __cdecl bh_remote_client_message(BHRemoteMessages *, void *, unsigned long size, unsigned long capacity);
#endif
