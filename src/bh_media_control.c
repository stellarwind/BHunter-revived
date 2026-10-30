#include "bh_media_control.h"
#include <float.h>
#include <string.h>
#include <digitalv.h>
static BHCdNotify *notify(BHMediaControl *c)
{
    return c && c->audio ? c->audio->notify : 0;
}
int __cdecl bh_cd_play_range(BHMediaControl *c, unsigned long from, unsigned long to, unsigned long ms,
                             unsigned long frame, long repeat)
{
    BHCdNotify *n = notify(c);
    if (!n || !n->cd)
        return 0;
    if (!n->cd->device)
        return 1;
    n->from_track = from;
    n->cd->playing = repeat;
    n->minute_second = ms;
    n->frame = frame;
    n->end_track = to;
    return bh_cd_play_retained(n);
}
void __cdecl bh_cd_stop(BHMediaControl *c)
{
    BHCdNotify *n = notify(c);
    if (!n || !n->cd)
        return;
    n->cd->playing = 0;
    if (n->cd->device)
    {
        ++c->stops;
        c->stop_result = mciSendCommandA(n->cd->device, MCI_STOP, 0, 0);
    }
}
/* Original FNSTSW C0 first comparison sends unordered to zero. The product
   remains extended until truncating CRT __ftol; no intermediate float store. */
static DWORD packed(float volume)
{
    unsigned long word;
    double v = volume;
    if (_isnan(v) || v < 0)
        v = 0;
    else if (v > 1)
        v = 1;
    word = (unsigned long)(v * 65535.0);
    return (word << 16) + word;
}
void __cdecl bh_cd_set_volume(BHMediaControl *c, float v)
{
    BHCdNotify *n = notify(c);
    if (!n || !n->cd)
        return;
    if (n->cd->local_file)
    {
        MCI_DGV_SETAUDIO_PARMSA settings;
        DWORD value = packed(v) & 65535UL;
        memset(&settings, 0, sizeof(settings));
        settings.dwItem = MCI_DGV_SETAUDIO_VOLUME;
        settings.dwValue = value * 1000UL / 65535UL;
        c->aux_result = mciSendCommandA(n->cd->device, MCI_SETAUDIO,
            MCI_DGV_SETAUDIO_ITEM | MCI_DGV_SETAUDIO_VALUE, (DWORD)&settings);
        ++c->aux_writes;
        if (!c->aux_result)
            n->cd->local_volume = (float)(value * (1.0 / 65535.0));
        return;
    }
    if (n->cd->last_track == -1)
        return;
    c->aux_packed = packed(v);
    ++c->aux_writes;
    c->aux_result = auxSetVolume((UINT)n->cd->last_track, c->aux_packed);
}
void __cdecl bh_wave_set_volume(BHMediaControl *c, float v)
{
    if (!c || !c->audio || c->audio->wave_device == (HWAVEOUT)-1)
        return;
    c->wave_packed = packed(v);
    ++c->wave_writes;
    c->wave_result = waveOutSetVolume(c->audio->wave_device, c->wave_packed);
}
