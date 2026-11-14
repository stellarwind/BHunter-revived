#include "bh_cd_audio.h"
int bh_cd_audio_startup(BHCdAudio *cd_audio)
{
    BHCdClose *cd;
    if (!cd_audio || !cd_audio->notify || !(cd = cd_audio->notify->cd) || cd->device || !cd_audio->music_path)
        return 0;
    cd->output = bh_audio_open();
    if (!cd->output)
        return 0;
    ++cd_audio->opens;
    cd->device = bh_audio_load_music(cd_audio->music_path, &cd->local_length);
    if (!cd->device && cd_audio->music_fallback_path)
        cd->device = bh_audio_load_music(cd_audio->music_fallback_path, &cd->local_length);
    cd_audio->open_result = cd->device ? 0 : -1;
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
double bh_cd_audio_volume(BHCdAudio *cd_audio)
{
    return cd_audio && cd_audio->notify && cd_audio->notify->cd && cd_audio->notify->cd->device ? cd_audio->notify->cd->local_volume : 1;
}
double bh_wave_audio_volume(BHCdAudio *a)
{
    (void)a;
    return bh_audio_get_effects_gain();
}
