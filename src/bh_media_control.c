#include "bh_media_control.h"
#include <float.h>
#include <math.h>
#include <string.h>
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
        c->stop_result = bh_audio_stop(n->cd->device);
    }
}
/* Original FNSTSW C0 first comparison sends unordered to zero. The product
   remains extended until truncating CRT __ftol; no intermediate float store. */
static unsigned long packed(float volume)
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
    c->aux_packed = packed(v);
    ++c->aux_writes;
    n->cd->local_volume = (float)((c->aux_packed & 65535UL) * (1.0 / 65535.0));
    /* The public volume slider is linear; translate to mixer hundredths dB. */
    if (n->cd->device)
    {
        float gain = n->cd->local_volume;
        c->aux_result = bh_audio_volume(n->cd->device, gain > 0 ? (long)(2000.0 * log10(gain)) : -10000);
    }
}
void __cdecl bh_wave_set_volume(BHMediaControl *c, float v)
{
    if (!c || !c->audio)
        return;
    c->wave_packed = packed(v);
    ++c->wave_writes;
    bh_audio_effects_gain((float)((c->wave_packed & 65535UL) * (1.0 / 65535.0)));
    c->wave_result = 0;
}
