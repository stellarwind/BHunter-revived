#include "bh_sound_startup.h"
#include <string.h>
int __cdecl bh_sound_startup(BHSoundQueue *q, HWND window, BHSoundStartupStats *stats)
{
    BHSoundStartupStats actual;
    (void)window;
    if (!q || !q->allocator.allocate || q->device || q->resources || q->voices) return 0;
    memset(&actual, 0, sizeof(actual));
    q->listener_active = 0;
    actual.create_calls = 1;
    q->device = bh_audio_open();
    actual.create_result = q->device ? 0 : -1;
    q->last_hresult = actual.create_result;
    q->error = q->device ? BH_SOUND_OK : BH_SOUND_BACKEND;
    if (stats) *stats = actual;
    return q->device != NULL;
}
