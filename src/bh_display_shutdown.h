#ifndef BH_DISPLAY_SHUTDOWN_H
#define BH_DISPLAY_SHUTDOWN_H
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <stdio.h>
/* Owning references:0045d750 releases viewport, device, auxiliary in order and
   clears each slot. This owner deliberately does not release color/draw/d3d.
   Window/path strings are borrowed; optional FILE is owned until fclose. */
typedef struct BHDisplayShutdown
{
    LPDIRECT3DVIEWPORT2 viewport;
    LPDIRECT3DDEVICE2 device;
    LPDIRECTDRAWSURFACE auxiliary;
    HWND window;
    FILE *log;
    unsigned long requested, delete_temporary;
    const char *temporary_first, *temporary_second;
    unsigned long releases, messages, files_closed, delete_attempts, failures;
} BHDisplayShutdown;
void __cdecl bh_display_release(BHDisplayShutdown *); /*0045d750*/
int __cdecl bh_display_shutdown(BHDisplayShutdown *); /*004738a0*/
#endif
