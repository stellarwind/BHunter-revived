#include "bh_cd_notify.h"
typedef char notify_play_layout[sizeof(MCI_PLAY_PARMS) == 12 ? 1 : -1];
int __cdecl bh_cd_notify(BHCdNotify *c, short event, MCIDEVICEID device)
{
    BHCdClose *cd;
    if (!c || !(cd = c->cd))
        return 0;
    if (!cd->device || device != cd->device || event != 1 || !cd->playing)
        return 1;
    return bh_cd_play_retained(c);
}
int __cdecl bh_cd_play_retained(BHCdNotify *c)
{
    BHCdClose *cd;
    MCI_PLAY_PARMS play;
    MCIDEVICEID device;
    if (!c || !(cd = c->cd) || !cd->device)
        return 0;
    device = cd->device;
    /* 00449c20/00449b90 pass a 12-byte stack parameter block. Keep the
       original TMSF bit packing, including the OR of the middle bytes. */
    play.dwFrom = c->from_track & 255UL;
    play.dwTo = (((c->frame & 255UL) << 8) | (c->minute_second & 65535UL)) << 8 | (c->end_track & 255UL);
    play.dwCallback = c->callback;
    if (cd->local_file)
    {
        /* The local WAV is CD track two, with its pregap removed. */
        play.dwFrom = 0;
        play.dwTo = MCI_TMSF_MINUTE(play.dwTo) * 60000UL +
            MCI_TMSF_SECOND(play.dwTo) * 1000UL +
            MCI_TMSF_FRAME(play.dwTo) * 1000UL / 75UL;
        if (play.dwTo > cd->local_length)
            play.dwTo = cd->local_length;
    }
    ++c->plays;
    c->last_result = mciSendCommandA(device, MCI_PLAY, 0x0d, (DWORD)&play);
    c->last = play;
    if (c->last_result)
    {
        ++c->failures;
        return 0;
    }
    return 1;
}
