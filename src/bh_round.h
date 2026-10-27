#ifndef BH_ROUND_H
#define BH_ROUND_H
/* Explicit binary32 storage boundary. This is a separate translation unit
   because pinned VC5 can reuse x87 values across volatile stores/reads in one
   TU. Requires a writable float destination and the caller's FP profile. */
void __cdecl bh_round_float(float *destination, double value);
#endif
