/* Behavioral reconstruction with a verified 45-byte match at 00401460.
   Header and frontend context matter; see verify_translation.py and FINDINGS.md.
   This does not establish the original source or project-wide compiler flags. */
#include <math.h>

extern "C" void __cdecl bh_mat34_translate_world(
    float x, float y, float z, float *m)
{
    m[3] = (float)(m[3] + x);
    m[7] = (float)(y + m[7]);
    m[11] = (float)(z + m[11]);
}
