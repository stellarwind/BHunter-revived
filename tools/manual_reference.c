/* Manual original-game launcher, never a recovered-source game target.
   Reuse debugger byte/creation helpers; never invoke the diagnostic main or
   its input/controller routines. Only registry/path/heap startup adapters run. */
#define main unused_diagnostic_main
#include "m3_startup_probe.c"
#undef main

int main(int argc,char **argv) {
    PROCESS_INFORMATION pi;STARTUPINFOA si;DEBUG_EVENT ev;CONTEXT c;
    HKEY key;DWORD disposition,remote,a[3],handled,code;LONG rc;
    char command[1024];int armed=0,done=0,ok=1,i;HANDLE t;
    if(argc!=6 || strlen(argv[1])>450 || strlen(argv[4])>200 ||
       strncmp(argv[5],"Software\\BhunterM3-",19))return 2;
    rc=RegCreateKeyExA(HKEY_CURRENT_USER,argv[5],0,NULL,0,KEY_ALL_ACCESS,NULL,&key,&disposition);
    if(rc || disposition!=REG_CREATED_NEW_KEY){if(!rc)RegCloseKey(key);return 3;}
    rc=RegSetValueExA(key,"SourceDir",0,REG_SZ,(BYTE*)argv[3],strlen(argv[3])+1);RegCloseKey(key);
    if(rc){RegDeleteKeyA(HKEY_CURRENT_USER,argv[5]);return 3;}
    memset(&si,0,sizeof(si));memset(&pi,0,sizeof(pi));si.cb=sizeof(si);
    si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_SHOWNORMAL;
    sprintf(command,"\"%s\"",argv[1]);
    if(!create_reference(argv[1],command,argv[2],&si,&pi,0)){RegDeleteKeyA(HKEY_CURRENT_USER,argv[5]);return 4;}
    child=pi.hProcess;open_thread=(OpenThreadProc)GetProcAddress(GetModuleHandleA("kernel32.dll"),"OpenThread");
    printf("Original game PID %lu. Manual control only; quit normally to close.\n",pi.dwProcessId);fflush(stdout);
    while(ok && !done) {
        if(!WaitForDebugEvent(&ev,INFINITE)){ok=0;break;}
        handled=DBG_CONTINUE;
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
            if((DWORD)ev.u.CreateProcessInfo.lpBaseOfImage!=0x400000)ok=0;
        } else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(ev.u.LoadDll.hFile)CloseHandle(ev.u.LoadDll.hFile);
        } else if(ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            CloseHandle(ev.u.CreateThread.hThread);
        } else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            printf("Original exit code: %lu\n",ev.u.ExitProcess.dwExitCode);done=1;
        } else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=ev.u.Exception.ExceptionRecord.ExceptionCode;
            t=open_thread?open_thread(0x1a,FALSE,ev.dwThreadId):NULL;
            memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
            if(!t || !GetThreadContext(t,&c))ok=0;
            else if(code==EXCEPTION_BREAKPOINT && !armed) {
                unsigned char original[6];
                remote=(DWORD)VirtualAllocEx(child,NULL,strlen(argv[5])+1,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                if(!remote || !write_bytes(remote,argv[5],strlen(argv[5])+1) ||
                   !read_bytes(0x47f7a9,original,6) || memcmp(original,"\xff\x15\xb0\x90\x48\x00",6) ||
                   !add_point(0x473a61,1) || !add_point(word(0x48907c),3) || !add_point(0x47f7a9,21))ok=0;
                armed=1;
            } else if(code==EXCEPTION_BREAKPOINT && armed) {
                Breakpoint *p=NULL;
                for(i=0;i<point_count;i++)if(points[i].active && points[i].address+1==c.Eip){p=points+i;break;}
                if(!p)handled=DBG_EXCEPTION_NOT_HANDLED;
                else {
                    c.Eip=p->address;
                    if(p->kind!=3 && !restore(p))ok=0;
                    if(!read_bytes(c.Esp,a,12))ok=0;
                    else if(p->kind==1) {
                        if(a[0]!=0x80000002 || a[1]!=0x491ce4)ok=0;
                        a[0]=0x80000001;a[1]=remote;if(!write_bytes(c.Esp,a,8))ok=0;
                    } else if(p->kind==3) {
                        DWORD n=strlen(argv[4]);
                        if(a[2]>n && a[1]){if(!write_bytes(a[1],argv[4],n+1))ok=0;c.Eax=n;}
                        else c.Eax=n+1;
                        c.Esp+=12;c.Eip=a[0];
                    } else if(p->kind==21) {
                        DWORD initial=128UL*1024*1024;
                        if(!write_bytes(c.Esp+4,&initial,4))ok=0;
                    }
                    if(ok && !SetThreadContext(t,&c))ok=0;
                }
            } else {
                handled=DBG_EXCEPTION_NOT_HANDLED;
                if(!ev.u.Exception.dwFirstChance)printf("Original exception %08lx at %08lx\n",code,(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress);
            }
            if(t)CloseHandle(t);
        }
        if(!ok){restore_all();TerminateProcess(child,0x4d330109);}
        if(!ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,handled)){ok=0;break;}
    }
    if(!done){TerminateProcess(child,0x4d330109);WaitForSingleObject(child,2000);}
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
    if(RegDeleteKeyA(HKEY_CURRENT_USER,argv[5])!=ERROR_SUCCESS)ok=0;
    return ok?0:1;
}
