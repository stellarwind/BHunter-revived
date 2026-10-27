#include "bh_sound_startup.h"
#include <stddef.h>
#include <string.h>
typedef char original_description_prefix[sizeof(void *) == 4 && offsetof(DSBUFFERDESC, lpwfxFormat) == 16 ? 1 : -1];
int __cdecl bh_sound_startup(BHSoundQueue *queue, HWND window, BHSoundStartupStats *stats)
{
    BHSoundStartupStats actual;
    DSBUFFERDESC description;
    unsigned long attempt;
    if (!queue || !queue->allocator.allocate || !queue->allocator.release || queue->device || queue->primary ||
        queue->resources || queue->voices)
        return 0;
    memset(&actual, 0, sizeof(actual));
    memset(&description, 0, sizeof(description));
    description.dwSize = 20;
    description.dwFlags = DSBCAPS_PRIMARYBUFFER;
    /* Exactly the original one-listener active word; stored vectors persist. */
    queue->listener_active = 0;
    for (attempt = 0; attempt < 10; ++attempt)
    {
        ++actual.create_calls;
        actual.create_result = DirectSoundCreate(NULL, &queue->device, NULL);
        if (actual.create_result == DS_OK)
            break;
        if (attempt < 9)
        {
            ++actual.sleep_calls;
            Sleep(100);
        }
    }
    if (actual.create_result != DS_OK)
    {
        queue->device = NULL;
        queue->error = BH_SOUND_BACKEND;
        queue->last_hresult = actual.create_result;
        if (stats)
            *stats = actual;
        return 0;
    }
    ++actual.cooperative_calls;
    actual.cooperative_result = IDirectSound_SetCooperativeLevel(queue->device, window, DSSCL_NORMAL);
    ++actual.primary_calls;
    actual.primary_result = IDirectSound_CreateSoundBuffer(queue->device, &description, &queue->primary, NULL);
    if (actual.primary_result != DS_OK)
    {
        /* Native COM failure outputs no owned interface. The defensive release
           covers a nonconforming backend without hiding the original result. */
        if (queue->primary)
            IDirectSoundBuffer_Release(queue->primary);
        queue->primary = NULL;
        queue->last_hresult = actual.primary_result;
        queue->error = BH_SOUND_BACKEND;
    }
    else
    {
        ++actual.play_calls;
        actual.play_result = IDirectSoundBuffer_Play(queue->primary, 0, 0, DSBPLAY_LOOPING);
        queue->last_hresult = actual.play_result;
        queue->error = actual.play_result == DS_OK ? BH_SOUND_OK : BH_SOUND_BACKEND;
    }
    if (stats)
        *stats = actual;
    return 1;
}
