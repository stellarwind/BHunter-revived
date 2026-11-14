#ifndef BH_LINUX_COMPAT_H
#define BH_LINUX_COMPAT_H
#include <math.h>
#include <stdlib.h>
#include <strings.h>

#define __cdecl
#define __int64 long long
#define _finite isfinite
#define _isnan isnan
#define _stricmp strcasecmp
#define _strnicmp strncasecmp

/* Gameplay depends on the original shared 15-bit random stream. */
int bh_rand(void);
void bh_srand(unsigned int seed);
#undef RAND_MAX
#define RAND_MAX 32767
#define rand bh_rand
#define srand bh_srand
#endif
