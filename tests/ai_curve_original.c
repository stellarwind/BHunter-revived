/* Pure complete-owner differential check. No game, scene or renderer runs. */
#include "../src/bh_camera_curve.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../build/ai-curve-match/original-curve.h"

#ifdef BH_CURVE_CANDIDATE
void __cdecl BH_CURVE_CANDIDATE(float, BHVec3 *, const BHVec3 *, const BHVec3 *, const BHVec3 *, const BHVec3 *);
#endif
typedef void (__cdecl *Curve)(float, BHVec3 *, const BHVec3 *, const BHVec3 *, const BHVec3 *, const BHVec3 *);
static unsigned long word(unsigned long *state)
{
    *state = *state * 1664525UL + 1013904223UL;
    return *state;
}
static float sample(unsigned long *state)
{
    return (float)((long)(word(state) >> 8) - 0x800000L) / 32;
}
static void run(Curve original, Curve source, const char *name, unsigned long *differences)
{
    static const unsigned long phases[] = {0,0x80000000UL,1,0x80000001UL,
        0x3f800000UL,0x3f7fffffUL,0x3f800001UL,0xbf800000UL,
        0x3f000000UL,0x40000000UL,0xc0000000UL,0x33800000UL};
    BHVec3 controls[4], a[4], b[4], expected, actual, *ap, *bp;
    unsigned long seed = 0x4341a0UL, i, j, alias, first = 0;
    float t;
    for (i = 0; i < 32768; ++i)
    {
        for (j = 0; j < 12; ++j) ((float *)controls)[j] = sample(&seed);
        if (i < 12288) memcpy(&t, phases + i % 12, 4);
        else t = (float)((long)(word(&seed) >> 8) - 0x800000L) / 2097152;
        if (i < 12) memset(controls, 0, sizeof(controls));
        for (alias = 0; alias < 5; ++alias)
        {
            memcpy(a, controls, sizeof(a)); memcpy(b, controls, sizeof(b));
            memset(&expected, 0xa5, sizeof(expected)); memset(&actual, 0xa5, sizeof(actual));
            ap = alias < 4 ? a + alias : &expected;
            bp = alias < 4 ? b + alias : &actual;
            original(t, ap, a, a+1, a+2, a+3);
            source(t, bp, b, b+1, b+2, b+3);
            if (memcmp(ap, bp, sizeof(*ap)) || memcmp(a, b, sizeof(a)))
            {
                if (!first)
                {
                    printf("%s first difference case=%lu alias=%lu t=%08lx expected=%08lx/%08lx/%08lx actual=%08lx/%08lx/%08lx\n",
                        name,i,alias,*(unsigned long *)&t,((unsigned long *)ap)[0],((unsigned long *)ap)[1],((unsigned long *)ap)[2],
                        ((unsigned long *)bp)[0],((unsigned long *)bp)[1],((unsigned long *)bp)[2]);
                    first=1;
                }
                ++*differences;
            }
        }
    }
}
int main(void)
{
    BHMathEnvironment environment;
    unsigned char *code;
    unsigned long differences = 0, candidate_differences = 0;
    if (!bh_math_environment_enter(&environment)) return 2;
    code = (unsigned char *)VirtualAlloc(0,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if (!code) return 2;
    install_original_curve(code);
    FlushInstructionCache(GetCurrentProcess(),code,4096);
    run((Curve)code,bh_camera_curve,"production",&differences);
#ifdef BH_CURVE_CANDIDATE
    run((Curve)code,BH_CURVE_CANDIDATE,"candidate",&candidate_differences);
#endif
    printf("{\"cases\":163840,\"production_differences\":%lu,\"candidate_differences\":%lu,\"pc\":53}\n",differences,candidate_differences);
    VirtualFree(code,0,MEM_RELEASE);
    bh_math_environment_leave(&environment);
    return differences || candidate_differences ? 1 : 0;
}
