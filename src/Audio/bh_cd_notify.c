#include "bh_cd_notify.h"
int bh_cd_play_retained(BHCdNotify *cd_notify)
{
    unsigned long packed, end;
    BHCdClose *cd;
    if (!cd_notify || !(cd = cd_notify->cd) || !cd->device)
        return 0;
    /* track02.wav starts at track two with the pregap removed. Preserve the
       original packed minute/second/frame interpretation and endpoint clamp. */
    packed = (((cd_notify->frame & 255UL) << 8) | (cd_notify->minute_second & 65535UL)) << 8 | (cd_notify->end_track & 255UL);
    end = ((packed >> 8) & 255UL) * 60000UL + ((packed >> 16) & 255UL) * 1000UL +
          ((packed >> 24) & 255UL) * 1000UL / 75UL;
    if (end > cd->local_length)
        end = cd->local_length;
    cd_notify->last.dwFrom = 0;
    cd_notify->last.dwTo = end;
    ++cd_notify->plays;
    cd_notify->last_result = bh_audio_music_range(cd->device, 0, end, cd->playing != 0);
    if (cd_notify->last_result)
    {
        ++cd_notify->failures;
        return 0;
    }
    return 1;
}
