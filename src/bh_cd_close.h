#ifndef BH_CD_CLOSE_H
#define BH_CD_CLOSE_H
#include <windows.h>
#include <mmsystem.h>
/* Borrow actual MCI device4dfb6c, playback flag4dfb70 and track4912d0.
   This complete stop/close owner does not open a CD drive or recover playback. */
typedef struct BHCdClose
{
    MCIDEVICEID device;
    long playing, last_track;
    MCIERROR stop_result, close_result;
    unsigned long native_calls;
    int local_file;
    DWORD local_length;
    float local_volume;
} BHCdClose;
/* Complete00449b40: device0 retains all state. Otherwise playing0 before
   real STOP, reload current device for real CLOSE, then device0/track-1.
   API errors are reported after all original effects; no Play occurs. */
int __cdecl bh_cd_stop_close(BHCdClose *);
#endif
