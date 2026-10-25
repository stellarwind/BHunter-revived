#ifndef BH_LIGHT_CONFIG_H
#define BH_LIGHT_CONFIG_H
#include "bh_math.h"
/* Explicit bindings for scattered original globals, not original struct ABI.
   directional RGB =>004e32e4/004e32e0/004e32d8;
   ambient RGB =>004e2de0/004e2de8/004e2dd8. */
typedef struct BHLightConfig {
    long enabled;BHVec3 direction;
    float directional[3],ambient[3];
} BHLightConfig;
/* 00459400: raw enable stored first; zero retains all other values. Enabled
   direction is negated then normalized; zero vector remains signed zero.
   RGB bytes multiply strengths directly, without clamp or division255.
   Scalar arguments preserve original value semantics and permit callers to
   pass fields from config itself. Caller supplies admitted PC53 environment.
   Source NULL guard/success return extend the original valid-pointer API. */
int __cdecl bh_light_configure(BHLightConfig *,long enabled,float x,float y,float z,
    float directional_strength,float ambient_strength,
    unsigned long directional_color,unsigned long ambient_color);
#endif
