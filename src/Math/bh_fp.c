#include "bh_fp.h"
enum
{
    X87_CONTROL_MASK = 0x0f3f,
    X87_PC53_NEAREST_MASKED = 0x023f
};

static unsigned short read_control(void)
{
    unsigned short control;
    __asm__ volatile("fnstcw %0" : "=m"(control));
    return control;
}

static void write_control(unsigned short control)
{
    __asm__ volatile("fldcw %0" : : "m"(control));
}

int bh_math_environment_supported(void)
{
    return (read_control() & X87_CONTROL_MASK) == X87_PC53_NEAREST_MASKED;
}

int bh_math_environment_enter(BHMathEnvironment *environment)
{
    if (!environment)
        return 0;
    environment->control = read_control();
    write_control((environment->control & ~X87_CONTROL_MASK) | X87_PC53_NEAREST_MASKED);
    return bh_math_environment_supported();
}

void bh_math_environment_leave(const BHMathEnvironment *environment)
{
    if (environment)
        write_control((read_control() & ~X87_CONTROL_MASK) | (environment->control & X87_CONTROL_MASK));
}
