#ifndef BH_WINDOW_DISPATCH_H
#define BH_WINDOW_DISPATCH_H
#include "bh_frame_buffers.h"
#include <ddraw.h>
/* Borrowed slots mirror the live process globals, including the actual owning
   DirectDraw references cleared by CLOSE/DESTROY. The thread HANDLE is borrowed
   and is not closed here. It must identify this application's worker only. */
typedef struct BHWindowDispatch
{
    BHFrameBuffers *frames; /* live application message hook/user */
    volatile LONG *display_state, *thread_active;
    const volatile LONG *hide_cursor;
    HANDLE *thread;
    LPDIRECTDRAW *draw;
    LPDIRECTDRAWSURFACE *primary, *back;
    unsigned long calls, hooks, paints, restores;
    unsigned long terminated, releases, quits, cursor_hides, native_calls, failures;
    HRESULT last_hresult;
    BOOL last_terminate;
    int last_cursor;
} BHWindowDispatch;
/* Complete00471be0, the real default cdecl application hook. user is the
   explicit source ABI extension shared by BHFrameMessage; it is unused.
   Forwards all original arguments and DefWindowProcA's actual LRESULT. */
LRESULT __cdecl bh_window_default_message(void *, HWND, UINT, WPARAM, LPARAM);
/* Dispatcher derived from00471ff0; the legacy F12 BMP hook is removed. This context-taking source
   entry is called by an application's WNDPROC adapter; it does not install a
   window class or take user focus. Native errors record diagnostics while
   original later effects continue. TerminateThread and ShowCursor preserve the
   original semantics; tests must use only owned threads and restore cursor
   counts. Source NULL-binding guards avoid invalid original global accesses. */
LRESULT __cdecl bh_window_dispatch(BHWindowDispatch *, HWND, UINT, WPARAM, LPARAM);
#endif
