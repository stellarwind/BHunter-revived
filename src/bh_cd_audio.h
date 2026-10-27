#ifndef BH_CD_AUDIO_H
#define BH_CD_AUDIO_H
#include "bh_cd_notify.h"
/* Explicit borrowed media owner: notify/cd must outlive this context. Existing
   BHCdClose.last_track binds original004912d0, actually the AUX device index;
   this family documents that historical field-name mismatch without changing
   admitted layouts. wave_device binds004912d4 (original UINT index or a real
   caller-owned wave handle for read-only queries). Neither getter acquires it. */
typedef struct BHCdAudio
{
    BHCdNotify *notify;
    HWAVEOUT wave_device;
    MCIERROR open_result, set_result;
    MMRESULT auxiliary_result, wave_result;
    unsigned long opens, sets, auxiliary_caps, wave_caps, auxiliary_reads, wave_reads;
} BHCdAudio;
/* Whole00449a50. Requires no owned MCI device. Native cdaudio open is exact;
   callback stored before call, other state retained on failure. Success sets
   TMSF10, resets AUX index and scans endpoints descending; existing wave index
   is preserved. set/caps errors don't convert original return1 into failure. */
int __cdecl bh_cd_audio_startup(BHCdAudio *, DWORD callback);
/* Whole00449cf0/00449da0. Only low16 of native packed volume is normalized;
   missing endpoint returns1. Native errors return defined1 with diagnostics
   (original error output was unspecified). Caller establishes PC53 profile. */
double __cdecl bh_cd_audio_volume(BHCdAudio *);
double __cdecl bh_wave_audio_volume(BHCdAudio *);
#endif
