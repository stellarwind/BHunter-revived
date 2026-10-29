#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include "../src/bh_rotation.h"
#include "original-rotation.h"

typedef void (__cdecl *Rotation)(float, BHMat34 *);
static unsigned long sequence = 0x19510728UL;
static unsigned long random_word(void)
{
    sequence = sequence * 1664525UL + 1013904223UL;
    return sequence;
}
int main(void)
{
    unsigned char *code = VirtualAlloc(NULL, 8192, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    unsigned int precisions[3] = {_PC_24, _PC_53, _PC_64};
    unsigned int saved = _control87(0, 0), pc, axis, i, j, differences = 0, checks = 0;
    BHMat34 actual, expected;
    Rotation source[2] = {bh_mat34_rotate_world_x, bh_mat34_rotate_world_y};
    float angle;
    if (!code)
        return 2;
    install_original_rotation(code);
    FlushInstructionCache(GetCurrentProcess(), code, 8192);
    for (pc = 0; pc < 3; ++pc)
    {
        _control87(precisions[pc] | _RC_NEAR | _MCW_EM, _MCW_PC | _MCW_RC | _MCW_EM);
        for (axis = 0; axis < 2; ++axis)
            for (i = 0; i < 16384; ++i)
            {
                for (j = 0; j < 12; ++j)
                    ((float *)&expected)[j] = (float)(long)(random_word() & 0x1fffff) / 4096.0f - 256.0f;
                angle = (float)(long)(random_word() & 0x1fffff) / 32768.0f - 32.0f;
                if (i < 4)
                    angle = i == 0 ? 0.0f : i == 1 ? -0.0f : i == 2 ? 1.5707963705062866f : -3.1415927410125732f;
                actual = expected;
                ((Rotation)(code + axis * 4096))(angle, &expected);
                source[axis](angle, &actual);
                ++checks;
                if (memcmp(&actual, &expected, sizeof(actual)))
                {
                    ++differences;
                    if (differences <= 4)
                        printf("difference pc=%u axis=%u case=%u\n", pc, axis, i);
                }
            }
    }
    _control87(saved, _MCW_PC | _MCW_RC | _MCW_EM);
    VirtualFree(code, 0, MEM_RELEASE);
    printf("rotation checks=%u differences=%u precisions=24,53,64\n", checks, differences);
    return differences ? 1 : 0;
}
