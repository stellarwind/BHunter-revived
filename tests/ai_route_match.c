/* Isolated function comparison; no scene, renderer, input or game execution. */
#include "../src/bh_axis_snap.h"
#include "../src/bh_route_curve_length.h"
#include "../src/bh_camera_curve.h"
#include "../src/bh_entities.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../build/ai-route-match/original-route.h"

static unsigned long random_word(unsigned long *s)
{
    *s = *s * 1664525UL + 1013904223UL;
    return *s;
}
static float sample(unsigned long *s)
{
    long n = (long)(random_word(s) >> 8) - 0x800000L;
    return (float)n / 32;
}
int main(void)
{
    BHMathEnvironment environment;
    unsigned char *code;
    double (__cdecl *original_axis)(BHVec3 *);
    float (__cdecl *original_length)(const BHEntity *);
    static const unsigned long edge[] = {0,0x80000000UL,1,0x80000001UL,
        0x3f800000UL,0xbf800000UL,0x7f800000UL,0xff800000UL,
        0x7fc00000UL,0xffc00000UL,0x7f7fffffUL,0xff7fffffUL};
    unsigned long seed = 0x4355c0UL, i, j, axis_cases = 0, length_cases = 0;
    unsigned long axis_differences = 0, length_differences = 0;
    BHVec3 a, b, controls[4], saved[4];
    BHEntity actor;
    float source_length, reference_length;
    double source_axis, reference_axis;
    if (!bh_math_environment_enter(&environment)) return 2;
    code = (unsigned char *)VirtualAlloc(0, 2048, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!code) return 2;
    install_original_route(code);
    FlushInstructionCache(GetCurrentProcess(), code, 2048);
    original_axis = (double (__cdecl *)(BHVec3 *))code;
    original_length = (float (__cdecl *)(const BHEntity *))(code + 512);
    for (i = 0; i < 8192 + 1728; ++i)
    {
        if (i < 8192)
        {
            a.x = sample(&seed); a.y = sample(&seed); a.z = sample(&seed);
        }
        else
        {
            j = i - 8192;
            memcpy(&a.x, edge + j % 12, 4);
            memcpy(&a.y, edge + (j / 12) % 12, 4);
            memcpy(&a.z, edge + j / 144, 4);
        }
        b = a;
        source_axis = bh_vec3_snap_axis(&a);
        reference_axis = original_axis(&b);
        ++axis_cases;
        if (memcmp(&a, &b, sizeof(a)) || memcmp(&source_axis, &reference_axis, 8))
            ++axis_differences;
    }
    for (i = 0; i < 8192 + 144; ++i)
    {
        for (j = 0; j < 12; ++j) ((float *)controls)[j] = sample(&seed);
        if (i >= 8192)
        {
            j = i - 8192;
            memset(controls, 0, sizeof(controls));
            memcpy(&controls[0].x, edge + j % 12, 4);
            memcpy(&controls[3].x, edge + j / 12, 4);
        }
        if (i < 4) memset(controls, 0, sizeof(controls));
        if (i == 1) controls[3].x = 1;
        if (i == 2) controls[3].y = -1;
        if (i == 3) controls[3].z = 1;
        memcpy(saved, controls, sizeof(saved));
        memset(&actor, 0xa5, sizeof(actor));
        memcpy((char *)&actor + 0x120, controls, 12);
        memcpy((char *)&actor + 0x144, controls + 1, 12);
        memcpy((char *)&actor + 0x150, controls + 2, 12);
        memcpy((char *)&actor + 0x12c, controls + 3, 12);
        source_length = bh_route_curve_length(controls);
        reference_length = original_length(&actor);
        ++length_cases;
        if (memcmp(&source_length, &reference_length, 4) || memcmp(controls, saved, sizeof(saved)))
            ++length_differences;
    }
    printf("{\"axis_cases\":%lu,\"axis_differences\":%lu,\"length_cases\":%lu,\"length_differences\":%lu,"
           "\"curve_dependency\":\"same real source in both length owners\",\"precision\":\"PC53\"}\n",
           axis_cases, axis_differences, length_cases, length_differences);
    VirtualFree(code, 0, MEM_RELEASE);
    bh_math_environment_leave(&environment);
    return axis_differences || length_differences;
}
