#ifndef BH_NETWORK_SCORES_H
#define BH_NETWORK_SCORES_H
#include "bh_network_transport.h"
#include "bh_hud_scores.h"
typedef struct BHNetworkScores {
 BHHudScores *scores;BHHudScoreText *text;BHNetworkTransport *transport;
 BHEntity *local_player;long mode;unsigned short frame;
 unsigned char packet[24];BHPacketWriter writer;HRESULT send_result;
 unsigned long broadcasts,reports;int error;
} BHNetworkScores;
/* Whole00426eb0/00426f90. Borrow live original eight-slot score roster, actual
   text queue owner and actual DirectPlay transport. Last duplicate remote
   identity wins; local player victim identity overrides slot lookup. Arithmetic
   wraps32-bit local and16-bit remote. Mode2 mutates and broadcasts then queues
   scores even after Send failure; other modes emit type11 report to recipient1.
   Type12 snapshot sends low16 local score in local slot, absent remotes zero.
   Native HRESULT is diagnostic only as original callers ignored it. No session,
   provider or peer initialization is performed here. All bindings outlive call.
   Packet storage retains final unpadded wire bytes for evidence/readback. */
int __cdecl bh_network_scores_broadcast(BHNetworkScores *);
int __cdecl bh_network_scores_death(BHNetworkScores *,long attacker,BHEntity *victim);
#endif
