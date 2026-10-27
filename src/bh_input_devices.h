#ifndef BH_INPUT_DEVICES_H
#define BH_INPUT_DEVICES_H
#include "bh_input_collection.h"
typedef struct BHInputDeviceApi
{
    UINT(WINAPI *map_key)(UINT, UINT);
    MMRESULT(WINAPI *capture)(HWND, UINT, UINT, BOOL);
    MMRESULT(WINAPI *capabilities)(UINT, LPJOYCAPSA, UINT);
    MMRESULT(WINAPI *release)(UINT);
} BHInputDeviceApi;
typedef struct BHInputDevices
{
    BHInputCollection *collection;
    const char *names[255];
    unsigned char allocated[255];
    JOYCAPSA caps[2];
    BHInputDeviceApi api;
    BHSceneAllocator allocator;
    MMRESULT capture_result[2], caps_result[2], release_result[2];
    unsigned long allocation_failures;
    int started;
} BHInputDevices;
/* Source ownership adapter. Fresh owner; collector/snapshot/list storage are
   borrowed. Seeds exactly255 static original key-name entries; key255 has no
   initialized table slot. NULL allocator selects actual CRT malloc/free. */
int __cdecl bh_input_devices_bind(BHInputDevices *, BHInputCollection *, const BHSceneAllocator *);
/* Complete00473d60: localize A..Z when mapped to a different uppercase byte,
   allocate missing names, reset key/history/list globals, capture both WinMM
   joysticks at100ms, query their real caps, reset axes/mouse/calibration and
   recenter(160,100). Native default performs actual capture/recenter; source
   tests override APIs and never capture devices or move the desktop cursor.
   Requires empty list owner, as at original startup. Capture/caps/recenter
   failures keep original flow; capture success alone enables each joystick.
   Allocation failures leave NULL entries, continue initialization and report0.
   Fresh per-entry source ownership avoids leaking replaced localized strings.
   Startup/stop do not free input lists: caller must dispose them separately. */
int __cdecl bh_input_devices_start(BHInputDevices *, HWND);
/* Complete00473fe0. Release enabled0 then1 even after API failure, then clear
   both availability flags. Return reports API errors separately from cleanup. */
int __cdecl bh_input_devices_stop(BHInputDevices *);
/* Source lifetime adapter frees only owned generated names after stop. */
void __cdecl bh_input_devices_dispose(BHInputDevices *);
#endif
