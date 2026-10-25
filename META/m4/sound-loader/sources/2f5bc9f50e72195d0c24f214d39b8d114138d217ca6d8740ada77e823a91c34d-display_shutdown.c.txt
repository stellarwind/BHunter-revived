#include "../src/bh_display_shutdown.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <string.h>
#include <stdio.h>
#ifndef HWND_MESSAGE
#define HWND_MESSAGE ((HWND)-3)
#endif
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL display-shutdown: %s\n",s);}}while(0)
static int create(const char *path){FILE *f=fopen(path,"wb");if(!f)return 0;fputs("private shutdown fixture",f);return fclose(f)==0;}
static int exists(const char *path){FILE *f=fopen(path,"rb");if(!f)return 0;fclose(f);return 1;}
int __cdecl bh_display_shutdown_checkpoint(unsigned long *count) {
 BHDisplayShutdown c;BHSceneView view;BHMathEnvironment fp;HWND window=NULL;MSG message;unsigned long before;
 const char *first="display-shutdown-first.tmp",*second="display-shutdown-second.tmp";
 checks=failures=0;memset(&c,0,sizeof(c));memset(&view,0,sizeof(view));
 if(!bh_math_environment_enter(&fp))return 2;
 CHECK(!bh_display_shutdown(NULL),"invalid context rejects");bh_display_release(NULL);
 /* Private message-only window: no visible desktop window and no user app. */
 window=CreateWindowEx(0,"STATIC","BHunter private shutdown check",0,0,0,0,0,HWND_MESSAGE,NULL,GetModuleHandle(NULL),NULL);
 CHECK(window!=NULL,"actual private message-only window");if(!window)goto done;c.window=window;
 CHECK(bh_scene_view_init(&view,64,64),"actual private offscreen display owner");if(!view.device)goto done;
 c.viewport=view.viewport;c.device=view.device;c.auxiliary=view.depth;
 IDirect3DViewport2_AddRef(c.viewport);IDirect3DDevice2_AddRef(c.device);IDirectDrawSurface_AddRef(c.auxiliary);
 c.log=fopen("display-shutdown-log.tmp","wb");CHECK(c.log!=NULL,"owned actual optional log file");if(c.log)fputs("buffered shutdown content",c.log);
 CHECK(create(first)&&create(second),"private temporary fixtures");c.temporary_first=first;c.temporary_second=second;c.delete_temporary=1;
 CHECK(bh_display_shutdown(&c),"complete release message close and two CRT deletes");
 CHECK(c.requested==1&&!c.viewport&&!c.device&&!c.auxiliary&&!c.log&&c.releases==3&&c.messages==1&&c.files_closed==1&&c.delete_attempts==2,"all owner publications/counters retained");
 CHECK(PeekMessage(&message,window,WM_CLOSE,WM_CLOSE,PM_REMOVE)&&message.wParam==0&&message.lParam==0,"actual queued WM_CLOSE parameters for private HWND");
 CHECK(!exists(first)&&!exists(second)&&exists("display-shutdown-log.tmp"),"CRT remove deletes only both owned temporary paths; fclose flushes log");
 CHECK(bh_scene_view_begin(&view),"independently retained actual device survives owning release");
 CHECK(IDirect3DDevice2_EndScene(view.device)==DD_OK,"surviving native owner ends scene");view.begun=0;
 c.delete_temporary=0;before=c.releases;CHECK(bh_display_shutdown(&c)&&c.releases==before&&c.files_closed==1,"repeat release null/file gates are idempotent while message repeats");
 CHECK(PeekMessage(&message,window,WM_CLOSE,WM_CLOSE,PM_REMOVE),"repeat original WM_CLOSE still delivered");
 CHECK(create(first)&&create(second),"disabled-delete control creates private paths");
 CHECK(bh_display_shutdown(&c)&&exists(first)&&exists(second),"zero flag preserves both actual files");
 CHECK(PeekMessage(&message,window,WM_CLOSE,WM_CLOSE,PM_REMOVE),"disabled-delete close message delivered");
 c.delete_temporary=1;c.temporary_first="missing-display-shutdown-dir/file.tmp";before=c.failures;
 CHECK(!bh_display_shutdown(&c)&&c.failures==before+1&&!exists(second)&&exists(first),"failed first CRT delete still executes second independent delete");
 CHECK(PeekMessage(&message,window,WM_CLOSE,WM_CLOSE,PM_REMOVE),"error-path close message delivered");
done:
 if(window)DestroyWindow(window);bh_display_release(&c);if(c.log)fclose(c.log);
 remove(first);remove(second);remove("display-shutdown-log.tmp");bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"display-shutdown\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_DISPLAY_SHUTDOWN_STANDALONE
int main(void){unsigned long n;return bh_display_shutdown_checkpoint(&n);}
#endif
