#include "bh_cd_notify.h"
typedef char notify_play_layout[sizeof(MCI_PLAY_PARMS) == 12 ? 1 : -1];
int __cdecl bh_cd_notify(BHCdNotify *c, short event, MCIDEVICEID device)
{
    BHCdClose *cd;
    if (!c || !(cd = c->cd))
        return 0;
    if (!cd->device || device != cd->device || event != 1 || !cd->playing)
        return 1;
    c->last.dwCallback = c->callback;
    c->last.dwFrom = c->from_track & 255UL;
    c->last.dwTo = (((c->frame & 255UL) << 8) | (c->minute_second & 65535UL)) << 8 | (c->end_track & 255UL);
    ++c->plays;
    c->last_result = mciSendCommand(cd->device, MCI_PLAY, 0x0d, (DWORD)&c->last);
    if (c->last_result)
    {
        ++c->failures;
        return 0;
    }
    return 1;
}
