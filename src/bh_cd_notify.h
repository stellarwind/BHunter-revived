#ifndef BH_CD_NOTIFY_H
#define BH_CD_NOTIFY_H
#include "bh_cd_close.h"
/* Retained original CD track/range policy, played from the extracted track. */
typedef struct BHCdNotify {
    BHCdClose *cd;
    unsigned long from_track, end_track, minute_second, frame;
    struct { unsigned long dwFrom, dwTo; } last;
    int last_result;
    unsigned long plays, failures;
} BHCdNotify;
int __cdecl bh_cd_play_retained(BHCdNotify *);
#endif
