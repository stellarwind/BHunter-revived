#ifndef BH_AXIS_SNAP_H
#define BH_AXIS_SNAP_H
#include "bh_math.h"
/* Complete004343c0: retain original length on x87/double return while replacing
   v with a signed length on its dominant axis. Strict comparisons give z
   priority over y over x on magnitude ties. Nonpositive selected component
   uses negative length, including selected zero -> negative zero. Other
   coordinates become positive zero. PC53 nearest/masked profile required. */
double __cdecl bh_vec3_snap_axis(BHVec3 *v);
#endif
