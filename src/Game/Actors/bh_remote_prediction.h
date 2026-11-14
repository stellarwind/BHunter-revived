#ifndef BH_REMOTE_PREDICTION_H
#define BH_REMOTE_PREDICTION_H
#include "../Entities/bh_entities.h"

/* Flags immediately after the angles in a player pose packet. */
enum
{
    BH_REMOTE_POSE_SHIELD = 1,
    BH_REMOTE_POSE_GROUND_RING = 2,
    BH_REMOTE_POSE_SMOKE = 4,
    BH_REMOTE_POSE_WRECK = 8,
    BH_REMOTE_POSE_HIDDEN = 16
};

/* Complete00423bb0: signed16 modular tick difference, negative ->0. Original
   anchor+180, anchor timestamp+17c, velocity+48 and acceleration+60 belong to
   the real entity payload. First binary32 stores v*t+anchor, then a*.5*t*t
   adds to those rounded components. Out must not overlap entity storage.
   Returns0 on invalid/nonfinite source input or unsupported FP environment. */
int bh_remote_predict_position(const BHEntity *, unsigned short tick, BHVec3 *out);
/* Complete00423400:32-word rows, slots0..30 and original count word31.
   Exactly one linked VC5 CRT rand() call for two valid preferences or for
   random fallback; none for a single valid preference. Raw count remains
   authoritative even if inconsistent; no ordinal found returns0 as original.
   rows/count are borrowed explicit bindings for original004cdca8 matrix.
   Invalid source row/slot bounds return0 without consuming random state. */
int bh_remote_choose_slot(const unsigned long (*rows)[32], unsigned long row_count, long row, long first, long second,
                          long *selected);
#endif
