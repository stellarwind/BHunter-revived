#include "bh_cd_notify.h"
int __cdecl bh_cd_play_retained(BHCdNotify *c)
{
    unsigned long packed, end;
    BHCdClose *cd;
    if (!c || !(cd = c->cd) || !cd->device)
        return 0;
    /* track02.wav starts at track two with the pregap removed. Preserve the
       original packed minute/second/frame interpretation and endpoint clamp. */
    packed = (((c->frame & 255UL) << 8) | (c->minute_second & 65535UL)) << 8 | (c->end_track & 255UL);
    end = ((packed >> 8) & 255UL) * 60000UL + ((packed >> 16) & 255UL) * 1000UL +
          ((packed >> 24) & 255UL) * 1000UL / 75UL;
    if (end > cd->local_length)
        end = cd->local_length;
    c->last.dwFrom = 0;
    c->last.dwTo = end;
    ++c->plays;
    c->last_result = bh_audio_music_range(cd->device, 0, end, cd->playing != 0);
    if (c->last_result)
    {
        ++c->failures;
        return 0;
    }
    return 1;
}
