#include "../src/bh_window_dispatch.h"
#include "../src/bh_force_feedback_start.h"
#include "../src/bh_cd_notify.h"
#include "../src/bh_input_messages.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#ifndef HWND_MESSAGE
#define HWND_MESSAGE ((HWND)-3)
#endif
static unsigned long checks,failures;
static int observe_nested;static unsigned long nested_destroys;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL window-dispatch: %s\n",s);}}while(0)
static LRESULT __cdecl input_message(void *user,HWND window,UINT message,WPARAM key,LPARAM bits){
    return bh_input_message((BHInputSnapshot*)user,window,message,key,bits);
}
typedef struct Waiting {HANDLE ready,wait;} Waiting;
static DWORD WINAPI waiting(void *user){Waiting *w=(Waiting*)user;SetEvent(w->ready);return WaitForSingleObject(w->wait,INFINITE);}
/* Test-only OS adapter, not recovery of original startup/class registration. */
static LRESULT CALLBACK private_proc(HWND window,UINT message,WPARAM key,LPARAM bits){
    BHWindowDispatch *c=(BHWindowDispatch*)GetWindowLongA(window,GWL_USERDATA);
    if(message==WM_NCCREATE){c=(BHWindowDispatch*)((CREATESTRUCTA*)bits)->lpCreateParams;
        SetWindowLongA(window,GWL_USERDATA,(LONG)c);}
    if(!c)return DefWindowProcA(window,message,key,bits);
    if(message==WM_DESTROY&&observe_nested){++nested_destroys;
        CHECK(c->quits==1&&c->releases==3&&!*c->draw&&!*c->primary&&!*c->back&&!*c->thread_active,
              "nested real WM_DESTROY enters only after outer CLOSE releases references, clears active flag and posts quit");}
    return bh_window_dispatch(c,window,message,key,bits);
}
static void native_window_bridge(BHFrameBuffers *frames,BHSceneView *scene,BHForceFeedbackStart *start,BHCdNotify *notify){
    const char *name="BHunterRestorationDispatchFixture";WNDCLASSA wc;ATOM atom;HWND window=NULL;
    BHWindowDispatch c;BHInputSnapshot snapshot;volatile LONG state=0,capture=0,active=0,hide=0;
    LPDIRECTDRAW draw=scene->draw;LPDIRECTDRAWSURFACE primary=scene->color,back=scene->depth;
    HANDLE thread=NULL;Waiting waiter;MSG message;DWORD exit_code;unsigned long before,removed=0;
    memset(&wc,0,sizeof(wc));memset(&c,0,sizeof(c));memset(&snapshot,0,sizeof(snapshot));memset(&waiter,0,sizeof(waiter));
    IDirectDraw_AddRef(draw);IDirectDrawSurface_AddRef(primary);IDirectDrawSurface_AddRef(back);
    c.frames=frames;c.display_state=&state;c.capture_request=&capture;c.thread_active=&active;
    c.hide_cursor=&hide;c.thread=&thread;c.draw=&draw;c.primary=&primary;c.back=&back;c.force=start;c.cd=notify;
    frames->message=input_message;frames->message_user=&snapshot;
    wc.lpfnWndProc=private_proc;wc.hInstance=GetModuleHandleA(NULL);wc.lpszClassName=name;
    atom=RegisterClassA(&wc);CHECK(atom!=0,"test-only native WNDPROC class registered with actual OS");if(!atom)goto done;
    window=CreateWindowExA(0,name,"BHunter message adapter",0,0,0,0,0,HWND_MESSAGE,NULL,wc.hInstance,&c);
    CHECK(window&&GetWindowLongA(window,GWL_USERDATA)==(LONG)&c,"WM_NCCREATE binds borrowed source context into actual private HWND");
    if(!window)goto done;
    SendMessageA(window,WM_SETFOCUS,0,0);CHECK((state&1)&&c.focus_starts>=1,"actual WNDPROC SendMessage reaches complete focus owner without SetFocus");
    SendMessageA(window,WM_KILLFOCUS,0,0);CHECK(!(state&1),"native private kill-focus message clears source bit");
    before=c.hooks;CHECK(!SendMessageA(window,WM_SYSCOMMAND,SC_SCREENSAVE|7,0)&&c.hooks==before,"native WNDPROC retains suppressed-system-command early return");
    SendMessageA(window,WM_KEYDOWN,'B',0);
    CHECK(snapshot.current['B']==1&&!capture,"actual installed source input-message callback receives synthetic private-window key");
    SendMessageA(window,WM_KEYDOWN,VK_F12,0);
    CHECK(capture==1,"actual WNDPROC F12 only publishes request without executing capture");capture=0;
    CHECK(SendMessageA(window,WM_GETTEXTLENGTH,0,0)==(LRESULT)strlen("BHunter message adapter"),"live hook delegates through actual default native message result");
    waiter.ready=CreateEventA(NULL,TRUE,FALSE,NULL);waiter.wait=CreateEventA(NULL,TRUE,FALSE,NULL);
    if(waiter.ready&&waiter.wait)thread=CreateThread(NULL,0,waiting,&waiter,0,NULL);
    CHECK(thread&&WaitForSingleObject(waiter.ready,1000)==WAIT_OBJECT_0,"second owned waiter is ready for real nested-close integration");
    active=thread?1:0;observe_nested=1;nested_destroys=0;
    SendMessageA(window,WM_CLOSE,0,0);observe_nested=0;
    CHECK(!IsWindow(window)&&nested_destroys==1&&c.quits==2&&c.releases==3&&c.terminated==(thread?1UL:0UL),
          "actual CLOSE default hook destroys own HWND, nested DESTROY repeats null gates without double Release or Terminate");window=NULL;
    if(thread)CHECK(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0&&GetExitCodeThread(thread,&exit_code)&&!exit_code,
          "nested cleanup terminates owned native waiter exactly once");
    while(PeekMessageA(&message,NULL,WM_QUIT,WM_QUIT,PM_REMOVE))++removed;
    CHECK(removed>0,"all native quit indications from nested close/destroy are consumed before fixture exit");
done:
    observe_nested=0;if(window&&IsWindow(window))DestroyWindow(window);
    if(thread){SetEvent(waiter.wait);WaitForSingleObject(thread,1000);CloseHandle(thread);}
    if(waiter.ready)CloseHandle(waiter.ready);if(waiter.wait)CloseHandle(waiter.wait);
    if(back)IDirectDrawSurface_Release(back);if(primary)IDirectDrawSurface_Release(primary);if(draw)IDirectDraw_Release(draw);
    if(atom)UnregisterClassA(name,wc.hInstance);
    while(PeekMessageA(&message,NULL,WM_QUIT,WM_QUIT,PM_REMOVE)){}
    frames->message=bh_window_default_message;frames->message_user=NULL;
}
int bh_window_dispatch_checkpoint(unsigned long *count){
    BHWindowDispatch context;BHFrameBuffers frames;BHForceFeedback force;BHForceFeedbackStart start;
    BHCdClose cd;BHCdNotify notify;BHInputSnapshot snapshot;BHSceneView scene;BHMathEnvironment fp;
    volatile LONG state=0,capture=0,active=0,hide=0;HANDLE thread=NULL;LPDIRECTDRAW draw=NULL;
    LPDIRECTDRAWSURFACE primary=NULL,back=NULL;HWND window=NULL,second=NULL;Waiting waiter;MSG msg;
    unsigned long before,native;int cursor_count,restored;DWORD exit_code;LRESULT direct,forwarded;
    checks=failures=0;memset(&context,0,sizeof(context));memset(&frames,0,sizeof(frames));
    memset(&force,0,sizeof(force));memset(&start,0,sizeof(start));memset(&cd,0,sizeof(cd));
    memset(&notify,0,sizeof(notify));memset(&snapshot,0,sizeof(snapshot));memset(&scene,0,sizeof(scene));memset(&waiter,0,sizeof(waiter));
    if(!bh_math_environment_enter(&fp))return 2;
    window=CreateWindowExA(0,"STATIC","BHunter private dispatch",0,0,0,0,0,HWND_MESSAGE,NULL,GetModuleHandleA(NULL),NULL);
    CHECK(window!=NULL,"private message-only native window cannot display or take user focus");if(!window)goto done;
    CHECK(bh_frame_buffers_bind(&frames,NULL),"actual frame owner provides live application hook slot");if(!frames.mutex)goto done;
    frames.message=bh_window_default_message;
    context.frames=&frames;context.display_state=&state;context.capture_request=&capture;
    context.thread_active=&active;context.hide_cursor=&hide;context.thread=&thread;
    context.draw=&draw;context.primary=&primary;context.back=&back;context.force=&start;context.cd=&notify;
    start.force=&force;notify.cd=&cd;
    direct=DefWindowProcA(window,WM_GETTEXTLENGTH,0,0);
    forwarded=bh_window_default_message(NULL,window,WM_GETTEXTLENGTH,0,0);
    CHECK(direct>0&&forwarded==direct&&bh_window_dispatch(&context,window,WM_GETTEXTLENGTH,0,0)==direct,
          "complete cdecl default adapter and dispatcher preserve actual native DefWindowProc result");
    force.enabled=1;state=(LONG)0x80000000UL;
    bh_window_dispatch(&context,window,WM_SETFOCUS,0,0);
    CHECK((unsigned long)state==0x80000001UL&&start.started==1&&context.focus_starts==1&&!start.acquires&&!start.starts,
          "focus preserves high bits and calls real optional force-start owner without device acquisition");
    bh_window_dispatch(&context,window,WM_KILLFOCUS,0,0);
    CHECK((unsigned long)state==0x80000000UL,"kill-focus clears only bit0");
    bh_window_dispatch(&context,window,WM_ACTIVATEAPP,FALSE,0);
    CHECK((unsigned long)state==0x80000002UL&&context.focus_starts==2,
          "original activate-app sets bit1 and calls start even when wParam isFALSE");
    bh_window_dispatch(&context,window,WM_SYSCOMMAND,SC_MINIMIZE,0);
    CHECK((unsigned long)state==0x80000000UL,"exact minimize clears only state bit1");
    bh_window_dispatch(&context,window,WM_SYSCOMMAND,SC_RESTORE,0);
    CHECK((unsigned long)state==0x80000002UL&&context.focus_starts==3,"exact restore sets bit1 and repeats force-start owner");
    before=context.hooks;
    CHECK(!bh_window_dispatch(&context,window,WM_SYSCOMMAND,SC_SCREENSAVE|3,0)&&
          !bh_window_dispatch(&context,window,WM_SYSCOMMAND,0xf170|7,0)&&context.hooks==before,
          "masked screen-saver and monitor-power commands return0 before application hook");
    frames.message=input_message;frames.message_user=&snapshot;
    CHECK(!bh_window_dispatch(&context,window,WM_KEYDOWN,VK_F12,0)&&capture==1&&snapshot.current[VK_F12]==1,
          "initial F12 sets request and forwards to actual source input-message hook without capture execution");
    capture=0;before=snapshot.history_head;
    CHECK(!bh_window_dispatch(&context,window,WM_KEYDOWN,VK_F12,0x40000000L)&&!capture&&snapshot.history_head==before,
          "repeated keydown neither requests capture nor adds input history");
    CHECK(!bh_window_dispatch(&context,window,WM_SYSKEYDOWN,VK_F12,0)&&capture==1,"initial system-F12 shares complete request branch");capture=0;
    CHECK(!context.cursor_hides,"zero cursor flag prevents all cursor visibility changes");
    /* One native decrement followed immediately by its exact inverse; no
       focus or cursor position changes, and no visible window is involved. */
    cursor_count=ShowCursor(TRUE)-1;ShowCursor(FALSE);hide=1;
    before=context.cursor_hides;bh_window_dispatch(&context,window,WM_MOUSEMOVE,0,0);
    restored=ShowCursor(TRUE);hide=0;
    CHECK(context.cursor_hides==before+1&&context.last_cursor==cursor_count-1&&restored==cursor_count,
          "native hide path decrements once and fixture restores exact caller cursor count immediately");
    before=context.hooks;bh_window_dispatch(&context,window,0x3b9,0x10001,0);
    CHECK(context.cd_notifications==1&&context.hooks==before+1&&!notify.plays,
          "MCI notification lowword is forwarded to actual no-device repeat owner without playback");
    CHECK(bh_scene_view_init(&scene,64,64),"actual private offscreen surfaces for paint/close paths");if(!scene.device)goto done;
    draw=scene.draw;primary=scene.color;back=scene.depth;
    IDirectDraw_AddRef(draw);IDirectDrawSurface_AddRef(primary);IDirectDrawSurface_AddRef(back);
    before=context.hooks;native=context.native_calls;
    CHECK(bh_window_dispatch(&context,window,WM_PAINT,0,0)==1&&context.hooks==before&&context.paints==1&&
          context.native_calls==native+2&&!context.restores,
          "actual BeginPaint healthy IsLost checks and EndPaint return1 without invoking application hook");
    waiter.ready=CreateEventA(NULL,TRUE,FALSE,NULL);waiter.wait=CreateEventA(NULL,TRUE,FALSE,NULL);
    if(waiter.ready&&waiter.wait)thread=CreateThread(NULL,0,waiting,&waiter,0,NULL);
    CHECK(thread&&WaitForSingleObject(waiter.ready,1000)==WAIT_OBJECT_0,"only owned fixture thread is blocked in native wait before termination test");
    active=thread?1:0;before=context.hooks;
    bh_window_dispatch(&context,window,WM_CLOSE,0,0);
    CHECK(!active&&context.terminated==(thread?1UL:0UL)&&(!thread||context.last_terminate)&&
          !draw&&!primary&&!back&&context.releases==3&&context.quits==1&&context.hooks==before+1,
          "close terminates owned waiter, restores own display mode, releases three real references and posts quit before hook");
    if(thread)CHECK(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0&&GetExitCodeThread(thread,&exit_code)&&exit_code==0,
          "actual original-style TerminateThread effect completes only the owned fixture worker");
    CHECK(!IsWindow(window),"forwarded native default close destroys only the private message window");window=NULL;
    CHECK(PeekMessageA(&msg,NULL,WM_QUIT,WM_QUIT,PM_REMOVE)&&msg.wParam==0,"actual current-thread PostQuitMessage0 observed and removed");
    second=CreateWindowExA(0,"STATIC","BHunter private destroy",0,0,0,0,0,HWND_MESSAGE,NULL,GetModuleHandleA(NULL),NULL);
    CHECK(second!=NULL,"separate private message window for destroy branch");
    if(second){before=context.releases;bh_window_dispatch(&context,second,WM_DESTROY,0,0);
        CHECK(context.releases==before&&context.terminated==(thread?1UL:0UL)&&context.quits==2,
              "destroy reuses whole cleanup path with null ownership and inactive-thread gates");
        CHECK(PeekMessageA(&msg,NULL,WM_QUIT,WM_QUIT,PM_REMOVE),"destroy also posts original quit message");}
    native_window_bridge(&frames,&scene,&start,&notify);
done:
    if(thread){SetEvent(waiter.wait);WaitForSingleObject(thread,1000);CloseHandle(thread);}
    if(waiter.ready)CloseHandle(waiter.ready);if(waiter.wait)CloseHandle(waiter.wait);
    if(window&&IsWindow(window))DestroyWindow(window);if(second&&IsWindow(second))DestroyWindow(second);
    if(back)IDirectDrawSurface_Release(back);if(primary)IDirectDrawSurface_Release(primary);if(draw)IDirectDraw_Release(draw);
    bh_scene_view_dispose(&scene);if(frames.mutex)bh_frame_buffers_dispose(&frames);
    CHECK(bh_math_environment_supported(),"window dispatch preserves explicit callerPC53 profile");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"window-dispatch\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false,\"physical_input\":false,\"ui_capture\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_WINDOW_DISPATCH_STANDALONE
int main(void){return bh_window_dispatch_checkpoint(NULL);}
#endif
