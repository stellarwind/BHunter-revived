#include "bh_cd_close.h"
int __cdecl bh_cd_stop_close(BHCdClose *c)
{
    if (!c) return 0;
    c->playing = 0;
    if (c->device) {
        ++c->native_calls;
        c->stop_result = bh_audio_stop(c->device);
        bh_audio_buffer_release(c->device);
        c->device = 0;
    }
    if (c->output) { bh_audio_release(c->output); c->output = 0; }
    c->local_file = 0; c->local_length = 0; c->last_track = -1;
    return 1;
}
