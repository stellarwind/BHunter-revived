#ifndef BH_WORLD_EDIT_H
#define BH_WORLD_EDIT_H
#include "bh_world_geometry.h"
/* Source semantic editor component, not an overlay of the original entity.
   Geometry must be first. Every member of its scene ring uses this component;
   integer coordinates map original+e8/+ec/+f0, angles map+3c/+40/+44. */
typedef struct BHWorldEditPlacement {
    BHWorldGeometry geometry;
    long coordinates[3];BHVec3 angles;
} BHWorldEditPlacement;
/* Whole00408b10: real construction followed by first-match portal resolution.
   Other scene components must already have valid constructed portal records.
   Only this component is rebuilt/reconnected; no reciprocal repair is added. */
int __cdecl bh_world_edit_rebuild(BHWorldGeometry *,unsigned long ring_count);
/* Whole0040e200: modulo32 translation, signed grid truncation, real rebuild;
   then attached following nodes only, unless single is nonzero. */
int __cdecl bh_world_edit_move(BHWorldEditPlacement *,unsigned long ring_count,
    long x,long y,long z,int single);
/* Whole0040e300: rotate attached nodes while owner is nonnull. Raw turn adds
   to orientation modulo4, while its SIGN selects coordinate quarter-turn.
   Thus turn0 still performs the original positive coordinate rotation. Upper
   orientation bits are cleared, Y/Z angles become0 and each node snaps/rebuilds.
   Borrowed models, geometry and ring storage stay live; PC53 finite profile. */
int __cdecl bh_world_edit_rotate(BHWorldEditPlacement *,unsigned long ring_count,
    long turn,long pivot_x,long pivot_z);
#endif
