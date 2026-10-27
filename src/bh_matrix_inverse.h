#ifndef BH_MATRIX_INVERSE_H
#define BH_MATRIX_INVERSE_H
#include "bh_math.h"
/* Required caller profile: PC53/nearest, masked exceptions, like bh_math.h.
   Complete fixed-size owners; algorithm provenance remains unresolved.
   LU input must be finite; output buffers must not overlap one another.
   Return1 preserves original semantics: a zero pivot arising from dependent
   nonzero rows is not rejected/repaired and may produce nonfinite solutions. */
int __cdecl bh_mat4_lu(float matrix[4][4], long pivot[4], float *parity);                      /* 00401c10 */
void __cdecl bh_mat4_lu_solve(const float matrix[4][4], const long pivot[4], float vector[4]); /* 00401b30 */
/* 00401e50. Complete affine inverse, including scale/shear/translation.
   Source and destination may alias. Failure retains destination unchanged. */
int __cdecl bh_mat34_inverse_general(const BHMat34 *matrix, BHMat34 *inverse);
#endif
