#include "bh_sound_startup.h"
#include <string.h>
int __cdecl bh_sound_startup(BHSoundQueue *sound_queue, HWND window, BHSoundStartupStats *stats)
{
    BHSoundStartupStats actual;
    (void)window;
    if (!sound_queue || !sound_queue->allocator.allocate || sound_queue->device || sound_queue->resources ||
        sound_queue->voices)
        return 0;
    memset(&actual, 0, sizeof(actual));
    sound_queue->listener_active = 0;
    actual.create_calls = 1;
    sound_queue->device = bh_audio_open();
    actual.create_result = sound_queue->device ? 0 : -1;
    sound_queue->last_hresult = actual.create_result;
    sound_queue->error = sound_queue->device ? BH_SOUND_OK : BH_SOUND_BACKEND;
    if (stats)
        *stats = actual;
    return sound_queue->device != NULL;
}
