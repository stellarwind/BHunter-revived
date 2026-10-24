#ifndef BH_ROTATION_H
#define BH_ROTATION_H
#include "bh_math.h"
/* 004017e0: post-rotate the basis about local Y; translation is unchanged.
   Functional PC53 source uses CRT sin/cos with explicit binary32 coefficient
   stores. Quarter-turn construction angles are the currently exercised domain;
   huge-angle x87 FSIN/FCOS reduction and exceptional inputs remain unvalidated. */
void __cdecl bh_mat34_rotate_y(float radians,BHMat34 *matrix);
#endif
