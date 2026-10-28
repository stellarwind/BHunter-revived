#ifndef BH_MISSION_PROFILE_H
#define BH_MISSION_PROFILE_H
/* Optional bounded diagnostics; never used to schedule simulation. */
#ifdef BH_MISSION_PROFILE
void bh_mp_start(void);
void bh_mp_frame(unsigned long frame);
__int64 bh_mp_begin(void);
void bh_mp_end(__int64 start, const char *kind, const char *detail, unsigned long value);
void bh_mp_save(void);
#define BH_MP_BEGIN(name) __int64 name = bh_mp_begin()
#define BH_MP_END(name, kind, detail, value) bh_mp_end(name, kind, detail, value)
#else
#define bh_mp_start() ((void)0)
#define bh_mp_frame(frame) ((void)0)
#define bh_mp_save() ((void)0)
#define BH_MP_BEGIN(name)
#define BH_MP_END(name, kind, detail, value) ((void)0)
#endif
#endif
