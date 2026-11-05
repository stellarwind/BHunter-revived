#ifndef BH_CD_CLOSE_H
#define BH_CD_CLOSE_H
#include <windows.h>
#include "bh_sdl_audio.h"
typedef struct BHCdClose
{
    BHAudioBuffer *device;
    BHAudioDevice *output;
    long playing, last_track;
    int stop_result, close_result;
    unsigned long native_calls;
    int local_file;
    DWORD local_length;
    float local_volume;
} BHCdClose;
int __cdecl bh_cd_stop_close(BHCdClose *);
#endif
