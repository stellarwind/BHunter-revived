#include "bh_media_control.h"
#include <float.h>
#include <math.h>
#include <string.h>
static BHCdNotify *notify(BHMediaControl *media_control)
{
    return media_control && media_control->audio ? media_control->audio->notify : 0;
}
int bh_cd_play_range(BHMediaControl *c, unsigned long from, unsigned long to, unsigned long ms, unsigned long frame,
                     long repeat)
{
    BHCdNotify *cd_notify = notify(c);
    if (!cd_notify || !cd_notify->cd)
        return 0;
    if (!cd_notify->cd->device)
        return 1;
    cd_notify->from_track = from;
    cd_notify->cd->playing = repeat;
    cd_notify->minute_second = ms;
    cd_notify->frame = frame;
    cd_notify->end_track = to;
    return bh_cd_play_retained(cd_notify);
}
void bh_cd_stop(BHMediaControl *media_control)
{
    BHCdNotify *cd_notify = notify(media_control);
    if (!cd_notify || !cd_notify->cd)
        return;
    cd_notify->cd->playing = 0;
    if (cd_notify->cd->device)
    {
        ++media_control->stops;
        media_control->stop_result = bh_audio_stop(cd_notify->cd->device);
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
void bh_cd_set_volume(BHMediaControl *media_control, float v)
{
    BHCdNotify *cd_notify = notify(media_control);
    if (!cd_notify || !cd_notify->cd)
        return;
    media_control->aux_packed = packed(v);
    ++media_control->aux_writes;
    cd_notify->cd->local_volume = (float)((media_control->aux_packed & 65535UL) * (1.0 / 65535.0));
    /* The public volume slider is linear; translate to mixer hundredths dB. */
    if (cd_notify->cd->device)
    {
        float gain = cd_notify->cd->local_volume;
        media_control->aux_result = bh_audio_volume(cd_notify->cd->device, gain > 0 ? (long)(2000.0 * log10(gain)) : -10000);
    }
}
void bh_wave_set_volume(BHMediaControl *media_control, float v)
{
    if (!media_control || !media_control->audio)
        return;
    media_control->wave_packed = packed(v);
    ++media_control->wave_writes;
    bh_audio_effects_gain((float)((media_control->wave_packed & 65535UL) * (1.0 / 65535.0)));
    media_control->wave_result = 0;
}
