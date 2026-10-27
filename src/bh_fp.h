#ifndef BH_FP_H
#define BH_FP_H

/* Source integration support, not a recovered original gameplay function.
   One scope per thread/boundary; pairs must be nested and restored in order.
   The supported current kernel profile is x87 PC53, nearest, all traps masked.
   Raw math functions require this profile; they do not silently change it.
   CRT control APIs may clear sticky status on writes; this scope preserves
   control settings, not a full x87 register/status environment. */
typedef struct BHMathEnvironment
{
    unsigned int control;
} BHMathEnvironment;
int __cdecl bh_math_environment_supported(void);
int __cdecl bh_math_environment_enter(BHMathEnvironment *environment);
void __cdecl bh_math_environment_leave(const BHMathEnvironment *environment);

#endif
