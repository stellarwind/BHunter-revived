/* Controlled debugger cleanup test. Includes real diagnostic definitions. */
#define main original_probe_main
#include "../tools/m3_startup_probe.c"
#undef main

static DWORD WINAPI waiting_worker(LPVOID unused) {for(;;)Sleep(100);return 0;}
static DWORD marker_value(DWORD value) {return value*3+1;}

int main(int argc,char **argv) {
    char path[1024],command[1200];STARTUPINFOA si;PROCESS_INFORMATION pi;DEBUG_EVENT ev;
    DWORD start,code,tid,base=(DWORD)GetModuleHandleA(NULL);int initial=0,requested=0,finished=0,passed=0,i;unsigned char before,after;
    if(argc==2 && !strcmp(argv[1],"--waiting-child")) {
        HANDLE worker=CreateThread(NULL,0,waiting_worker,NULL,0,&tid);volatile DWORD value;
        if(!worker)return 2;CloseHandle(worker);
        for(;;){value=marker_value(GetTickCount());Sleep(100);}return 0;
    }
    if(argc!=1 || !GetModuleFileNameA(NULL,path,sizeof(path)))return 2;
    open_thread=(OpenThreadProc)GetProcAddress(GetModuleHandleA("kernel32.dll"),"OpenThread");
    if(!open_thread)return 2;
    memset(&si,0,sizeof(si));memset(&pi,0,sizeof(pi));si.cb=sizeof(si);
    sprintf(command,"\"%s\" --waiting-child",path);
    if(!CreateProcessA(path,command,NULL,NULL,FALSE,DEBUG_ONLY_THIS_PROCESS|CREATE_NO_WINDOW,NULL,NULL,&si,&pi))return 2;
    child=pi.hProcess;start=GetTickCount();
    while(!finished && GetTickCount()-start<10000) {
        if(!WaitForDebugEvent(&ev,100)) {
            DebugBreakProc stop=(DebugBreakProc)GetProcAddress(GetModuleHandleA("kernel32.dll"),"DebugBreakProcess");
            if(GetLastError()!=ERROR_SEM_TIMEOUT)break;
            if(initial && traced_count>=2 && !requested){if(!stop || !stop(child))break;requested=1;}continue;
        }
        code=DBG_CONTINUE;
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            if((DWORD)ev.u.CreateProcessInfo.lpBaseOfImage!=base)break;
            trace_thread(ev.dwThreadId,(DWORD)ev.u.CreateProcessInfo.lpStartAddress,open_thread(0x1a,FALSE,ev.dwThreadId));
            module_snapshot(base,ev.u.CreateProcessInfo.hFile,path);
            if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            module_snapshot((DWORD)ev.u.LoadDll.lpBaseOfDll,ev.u.LoadDll.hFile,"<unresolved>");
            if(ev.u.LoadDll.hFile)CloseHandle(ev.u.LoadDll.hFile);
        } else if(ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            if(!trace_thread(ev.dwThreadId,(DWORD)ev.u.CreateThread.lpStartAddress,ev.u.CreateThread.hThread))break;
        } else if(ev.dwDebugEventCode==EXIT_THREAD_DEBUG_EVENT) {
            for(i=0;i<traced_count;i++)if(traced_threads[i].active && traced_threads[i].id==ev.dwThreadId){CloseHandle(traced_threads[i].handle);traced_threads[i].active=0;}
        } else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            if(ev.u.Exception.ExceptionRecord.ExceptionCode!=EXCEPTION_BREAKPOINT){code=DBG_EXCEPTION_NOT_HANDLED;}
            else if(!initial)initial=1;
            else if(requested) {
                int captured=stopped_thread_snapshots();
                int armed=read_bytes((DWORD)marker_value,&before,1) && add_point((DWORD)marker_value,99);
                int restored=armed && restore_all() && read_bytes((DWORD)marker_value,&after,1) && before==after;
                printf("{\"event\":\"timeout-control-result\",\"original_code_executes\":false,\"threads_tracked\":%d,\"contexts_read\":%s,\"breakpoint_armed\":%s,\"restored_verified\":%s}\n",traced_count,captured?"true":"false",armed?"true":"false",restored?"true":"false");
                passed=captured && restored;TerminateProcess(child,0x4d33c001);
            }
        } else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            printf("{\"event\":\"control-child-exit\",\"code\":%lu}\n",ev.u.ExitProcess.dwExitCode);finished=1;
        }
        if(!ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,code))break;
    }
    if(!finished){TerminateProcess(child,0x4d33c002);WaitForSingleObject(child,2000);}
    for(i=0;i<traced_count;i++)if(traced_threads[i].active)CloseHandle(traced_threads[i].handle);
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return finished && passed?0:1;
}
