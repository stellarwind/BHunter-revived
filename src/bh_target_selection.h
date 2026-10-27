#ifndef BH_TARGET_SELECTION_H
#define BH_TARGET_SELECTION_H
#include "bh_entities.h"

typedef struct BHTargetSelection
{
    BHEntity *target;
    float score;
    unsigned long visited;
} BHTargetSelection;

/* Complete 00421610 with explicit borrowed owners for original 004dfab8
   (player) and 004dfb28 (actor head). Neither owner is created or installed.
   Player +238 gates its unconditional initial selection, even when excluded;
   signed player +234 and actor +100 are divisors, including zero/negative.
   Actor +fc == 2 and actor != excluded gate event-3 position queries. Actor
   +f0 is read AFTER its callback. Actor/query/list/position lifetimes must
   survive callbacks; each selected query must fill the position packet. The
   query buffer has the original 40-byte capacity; unused words are unspecified.
   The original score rounds to float only after selection. Unordered scores
   select too (original FCOM C0), while equal ordered scores retain list order.
   Caller establishes the project's PC53/nearest/masked FP environment. PC24
   and original runtime FP transitions remain unvalidated.
   Returns 0 for null origin/result, exhausted traversal budget or a selected
   actor without a query; partial result/callback effects remain observable.
   These guards are source failure behavior outside the original valid domain.
   result must not alias origin or any actor/list storage. */
int __cdecl bh_target_select(BHEntity *player, BHEntity *head, const BHEntity *excluded, const BHVec3 *origin,
                             unsigned long limit, BHTargetSelection *result);
#endif
