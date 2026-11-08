#ifndef BH_FP_H
#define BH_FP_H

/* Source integration support, not a recovered original gameplay function.
   One scope per thread/boundary; pairs must be nested and restored in order.
   The supported current kernel profile is x87 PC53, nearest, all traps masked.
   Raw math functions require this profile; they do not silently change it.
   The scope saves and restores the x87 precision, rounding and exception masks. */
typedef struct BHMathEnvironment
{
    unsigned int control;
} BHMathEnvironment;
int bh_math_environment_supported(void);
int bh_math_environment_enter(BHMathEnvironment *environment);
void bh_math_environment_leave(const BHMathEnvironment *environment);

#endif
