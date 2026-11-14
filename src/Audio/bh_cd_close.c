#include "bh_cd_close.h"
int bh_cd_stop_close(BHCdClose *cd_close)
{
    if (!cd_close)
        return 0;
    cd_close->playing = 0;
    if (cd_close->device)
    {
        ++cd_close->native_calls;
        cd_close->stop_result = bh_audio_stop(cd_close->device);
        bh_audio_buffer_release(cd_close->device);
        cd_close->device = 0;
    }
    if (cd_close->output)
    {
        bh_audio_release(cd_close->output);
        cd_close->output = 0;
    }
    cd_close->local_file = 0;
    cd_close->local_length = 0;
    cd_close->last_track = -1;
    return 1;
}
