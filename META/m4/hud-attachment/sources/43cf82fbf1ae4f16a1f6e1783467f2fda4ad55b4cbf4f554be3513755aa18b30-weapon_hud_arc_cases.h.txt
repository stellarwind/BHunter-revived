/* Static instruction-derived PC53 curve fixtures, not original-runtime output. */
typedef struct ArcProbe {unsigned long point,axis,expected;} ArcProbe;
typedef struct ArcCase {const char *name;unsigned long vectors[4][3],strength,seed,count;ArcProbe probes[4];} ArcCase;
static const ArcCase arc_cases[]={
{"retained_x",{{0x45cc2b10UL,0x44abd365UL,0x4633a0fdUL},{0xc6d5f0acUL,0x47016e8dUL,0x468623ceUL},{0xc70a1bf9UL,0x470204f0UL,0x4652dfb9UL},{0x46a17befUL,0x455cb0f3UL,0xc7110348UL}},0x3f001d7eUL,53189,4,{{2,0,0x457a3978UL},{4,0,0x4472d7e2UL},{5,0,0xc415e7d9UL},{15,0,0xc6105d21UL}}},
{"retained_yz",{{0x470d606bUL,0x4635694aUL,0xc715a41dUL},{0xc61cb62bUL,0x4637880eUL,0x46ca17f9UL},{0xc5be9e46UL,0x464753f8UL,0x46a3e822UL},{0xc70bbc4fUL,0xc599f86dUL,0x46fb504eUL}},0x3f5e40eeUL,32245,4,{{1,1,0x46358d05UL},{3,2,0xc6bd9e31UL},{4,1,0x4630bbb5UL},{5,2,0xc64793a9UL}}},
{"retained_blend",{{0x4716843eUL,0x46954cb3UL,0xc6e01b7fUL},{0x4709974eUL,0xc62e29f3UL,0xc6d83d41UL},{0x4695bfa6UL,0xc708f28cUL,0x468517b7UL},{0x4711deb9UL,0x468d2099UL,0xc71aae6eUL}},0x3e9e3458UL,63398,4,{{1,2,0xc6df920aUL},{2,1,0x468a6f01UL},{4,0,0x471457e8UL},{5,1,0x466a16b6UL}}},
};
