#ifndef BH_LIGHT_EVAL_H
#define BH_LIGHT_EVAL_H
#include "bh_light.h"
/* Complete 00450cd0/00451140 with original global bindings supplied by c.
   Packed result is 00RRGGBB, including zero for an empty list. Modes 1/2 use
   effective_intensity; 3/4 use intensity and range/cone attenuation. Prepared
   mode reads prepared_type/local fields and local_reference when override is
   active. World mode reads type/world fields and the override itself.
   Borrowed live acyclic list; PC53/nearest/masked. Inputs need not be unit
   length (the originals do not normalize them). Source guards reject invalid
   consumed inputs or scaled channel sums outside signed32 conversion range;
   result remains unchanged on failure. FP sticky flags are not a contract.
   Result may alias input storage: all reads precede the single output store. */
int __cdecl bh_light_evaluate_prepared(const BHLightContext *c, const BHVec3 *point, const BHVec3 *normal, float scale,
                                       unsigned long *result);
int __cdecl bh_light_evaluate_world(const BHLightContext *c, const BHVec3 *point, const BHVec3 *normal, float scale,
                                    unsigned long *result);
#endif
