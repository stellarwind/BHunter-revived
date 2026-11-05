#include "bh_cd_audio.h"
int __cdecl bh_cd_audio_startup(BHCdAudio *a, DWORD unused)
{
    BHCdClose *cd;
    (void)unused;
    if (!a || !a->notify || !(cd = a->notify->cd) || cd->device || !a->music_path)
        return 0;
    cd->output = bh_audio_open();
    if (!cd->output)
        return 0;
    ++a->opens;
    cd->device = bh_audio_load_music(a->music_path, &cd->local_length);
    a->open_result = cd->device ? 0 : -1;
    if (!cd->device)
    {
        bh_audio_release(cd->output);
        cd->output = 0;
        return 0;
    }
    cd->local_file = 1;
    cd->local_volume = 1;
    cd->last_track = -1;
    return 1;
}
double __cdecl bh_cd_audio_volume(BHCdAudio *a)
{
    return a && a->notify && a->notify->cd && a->notify->cd->device ? a->notify->cd->local_volume : 1;
}
double __cdecl bh_wave_audio_volume(BHCdAudio *a)
{
    (void)a;
    return bh_audio_get_effects_gain();
}
