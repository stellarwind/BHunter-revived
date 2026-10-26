#ifndef BH_APPLICATION_BOOTSTRAP_H
#define BH_APPLICATION_BOOTSTRAP_H
#include "bh_engine_bootstrap.h"
#include "bh_source_files.h"
#include "bh_paths.h"
#include "bh_display_enum.h"
#include "bh_render_startup.h"
#include "bh_window_dispatch.h"
#include "bh_frame_worker.h"
typedef struct BHStartupMode {unsigned long width,height,bits;int software;long resolution;} BHStartupMode;
typedef struct BHApplicationBootstrap {
 BHSourceFiles *source;BHPaths *paths;BHConfig *config;BHDisplayCatalog catalog;
 BHEngineBootstrap *engine;BHWindowDispatch *dispatch;BHFrameWorker *worker;BHFrameCallback *callback;
 BHRenderMode render;LPDIRECTDRAW draw;LPDIRECTDRAW2 draw2;
 HINSTANCE instance;HWND window;HANDLE thread;DWORD thread_id;
 void *startup_allocation; /* source-owned original004f9610 temporary, free once after thread startup */
 volatile LONG display_state,capture,thread_active,hide_cursor;
 int no_dialogs,bounded_startup;long installed,configured,selected,hardware,resolution;
 DWORD last_tick;double previous_time,frame_seconds;HRESULT last_hresult;
 unsigned long source_attempts,dialogs,mode_attempts,iterations,sleeps;char errors[8192];int error;
} BHApplicationBootstrap;
/* Whole004723c0. Actual CD checksum/retry dialog, class/window, path/config,
   driver selection, hardware/software fallback order, cooperative interfaces,
   real engine/render-worker startup, native message dispatch and pacing loop.
   All native interfaces/callbacks are real source owners. One application owns
   these references; source WNDPROC carries its context in GWL_USERDATA.
   no_dialogs is an explicit source failure boundary for unattended checks;
   bounded_startup returns before worker creation. Neither option drives input
   or gameplay. Tests stop at actual failed source-file verification, before
   windows/display/devices; positive native/application loop is unexecuted.
   The caller initializes all borrowed owners and source scene bindings first.
   No original executable is loaded or executed. */
DWORD __cdecl bh_application_bootstrap(BHApplicationBootstrap *,HINSTANCE,HINSTANCE,const char *,int);
/* Count-zero arithmetic/data adapters, shared with the actual owner. */
unsigned long __cdecl bh_application_mode_plan(int hardware,int low,BHStartupMode out[9]);
int __cdecl bh_application_pacing(double elapsed,double delay,DWORD *sleep_ms);
#endif
