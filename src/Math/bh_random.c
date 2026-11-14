#include <stdint.h>

static uint32_t state = 1;

int bh_rand(void)
{
    state = state * UINT32_C(214013) + UINT32_C(2531011);
    return (int)((state >> 16) & 0x7fff);
}

void bh_srand(unsigned int seed)
{
    state = seed;
}
