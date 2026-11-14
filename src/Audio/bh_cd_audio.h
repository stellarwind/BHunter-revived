#ifndef BH_CD_AUDIO_H
#define BH_CD_AUDIO_H
#include "bh_cd_notify.h"
typedef struct BHCdAudio
{
    BHCdNotify *notify;
    int open_result, set_result;
    unsigned long opens;
    const char *music_path, *music_fallback_path;
} BHCdAudio;
/* Loads the extracted CD track through SDL; callback retained at call sites
   for the transition ABI, but no native window notification is installed. */
int bh_cd_audio_startup(BHCdAudio *);
double bh_cd_audio_volume(BHCdAudio *);
double bh_wave_audio_volume(BHCdAudio *);
#endif
