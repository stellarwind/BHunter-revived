#ifndef BH_MODEL_PATH_H
#define BH_MODEL_PATH_H
#include "bh_model.h"
/* 0044e910: append one missing backslash, empty path forces bare-only;
   policy outside0..2 also selects bare-only. Bounded input guards retain
   context on invalid/overlong input. Explicit context replaces two globals. */
int __cdecl bh_model_path_set(BHModelContext *context, const char *prefix, long policy);
#endif
