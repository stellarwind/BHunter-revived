/* Opt-in original-reference diagnostic. Native Windows loader/debug APIs only.
   Temporary software execution breakpoints restored when reached; file intact.
   Stops at the first source-check return, before WinMain creates any window. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int read_bytes(HANDLE process,DWORD address,void *out,DWORD size) {
    DWORD got=0;return ReadProcessMemory(process,(LPCVOID)address,out,size,&got) && got==size;
}
static int write_bytes(HANDLE process,DWORD address,const void *bytes,DWORD size) {
    DWORD got=0;return WriteProcessMemory(process,(LPVOID)address,(LPVOID)bytes,size,&got) && got==size;
}
static int instruction_byte(HANDLE process,DWORD address,unsigned char value) {
    DWORD old,unused;int ok;
    if(!VirtualProtectEx(process,(LPVOID)address,1,PAGE_EXECUTE_READWRITE,&old))return 0;
    ok=write_bytes(process,address,&value,1) && FlushInstructionCache(process,(LPCVOID)address,1);
    if(!VirtualProtectEx(process,(LPVOID)address,1,old,&unused))ok=0;
    return ok;
}
static int error(const char *where) {printf("{\"event\":\"probe-error\",\"where\":\"%s\",\"win32_error\":%lu}\n",where,GetLastError());return 0;}
int main(int argc,char **argv) {
    static const unsigned char call_bytes[6]={0xff,0x15,0x04,0x90,0x48,0x00};
    PROCESS_INFORMATION pi;STARTUPINFOA startup;DEBUG_EVENT event;CONTEXT context;
    HKEY key=NULL;DWORD disposition,type=REG_SZ;LONG result;char command[1024];
    unsigned char actual[6],return_bytes[6];DWORD args[5],remote=0,begin,code,handled;char original_path[128];
    int redirect,created=0,armed=0,observed=0,finished=0,killed=0,ok=1,registry_created=0;
    if(argc!=6)return 2;redirect=!strcmp(argv[1],"redirect");
    if(!redirect && strcmp(argv[1],"baseline"))return 2;
    if(strlen(argv[2])>500 || strlen(argv[4])>500 || strncmp(argv[5],"Software\\BhunterM3-",19))return 2;
    memset(&pi,0,sizeof(pi));memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    if(redirect) {
        result=RegCreateKeyExA(HKEY_CURRENT_USER,argv[5],0,NULL,0,KEY_ALL_ACCESS,NULL,&key,&disposition);
        if(result!=ERROR_SUCCESS || disposition!=REG_CREATED_NEW_KEY){if(key)RegCloseKey(key);return 3;}registry_created=1;
        result=RegSetValueExA(key,"SourceDir",0,type,(const BYTE*)argv[4],(DWORD)strlen(argv[4])+1);RegCloseKey(key);key=NULL;
        if(result!=ERROR_SUCCESS){ok=error("private-registry-value");goto done;}
    }
    sprintf(command,"\"%s\"",argv[2]);begin=GetTickCount();
    if(!CreateProcessA(argv[2],command,NULL,NULL,FALSE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[3],&startup,&pi)){ok=error("CreateProcess");goto done;}created=1;
    printf("{\"event\":\"process-created\",\"pid\":%lu,\"redirect\":%s,\"instrumentation\":\"two-temporary-breakpoint-bytes\"}\n",pi.dwProcessId,redirect?"true":"false");fflush(stdout);
    while(!finished) {
        if(!WaitForDebugEvent(&event,250)) {
            if(GetLastError()!=ERROR_SEM_TIMEOUT){ok=error("WaitForDebugEvent");break;}
            if(GetTickCount()-begin>10000 && !killed){
                SuspendThread(pi.hThread);memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS;
                if(GetThreadContext(pi.hThread,&context))printf("{\"event\":\"timeout-context\",\"eip\":%lu,\"dr0\":%lu,\"dr1\":%lu,\"dr7\":%lu}\n",context.Eip,context.Dr0,context.Dr1,context.Dr7);
                ok=0;killed=1;TerminateProcess(pi.hProcess,0x4d330002);printf("{\"event\":\"timeout\"}\n");}
            if(GetTickCount()-begin>15000)break;
            continue;
        }
        handled=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
            printf("{\"event\":\"image-loaded\",\"base\":%lu,\"thread\":%lu}\n",(DWORD)event.u.CreateProcessInfo.lpBaseOfImage,event.dwThreadId);
            if((DWORD)event.u.CreateProcessInfo.lpBaseOfImage!=0x400000){ok=error("unexpected-image-base");killed=1;TerminateProcess(pi.hProcess,0x4d330003);}
        } else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            printf("{\"event\":\"process-exit\",\"code\":%lu,\"probe_termination\":%s}\n",event.u.ExitProcess.dwExitCode,killed?"true":"false");finished=1;
        } else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=event.u.Exception.ExceptionRecord.ExceptionCode;
            memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS;
            if(!GetThreadContext(pi.hThread,&context)){ok=error("GetThreadContext");killed=1;TerminateProcess(pi.hProcess,0x4d330004);}
            else if(code==EXCEPTION_BREAKPOINT && !armed) {
                if(event.dwThreadId!=pi.dwThreadId || !read_bytes(pi.hProcess,0x473a61,actual,6) || memcmp(actual,call_bytes,6) || !read_bytes(pi.hProcess,0x4723cf,return_bytes,6) || memcmp(return_bytes,"\x8b\x35\x48\x91\x48\x00",6)){ok=error("reviewed-callsite-bytes");killed=1;TerminateProcess(pi.hProcess,0x4d330005);}
                else {
                    if(redirect) {
                        remote=(DWORD)VirtualAllocEx(pi.hProcess,NULL,(DWORD)strlen(argv[5])+1,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                        if(!remote || !write_bytes(pi.hProcess,remote,argv[5],(DWORD)strlen(argv[5])+1)){ok=error("private-path-buffer");killed=1;TerminateProcess(pi.hProcess,0x4d330006);}
                    }
                    if(!killed){
                        if(!instruction_byte(pi.hProcess,0x473a61,0xcc) || !instruction_byte(pi.hProcess,0x4723cf,0xcc)){ok=error("arm-software-breakpoints");killed=1;TerminateProcess(pi.hProcess,0x4d330007);}else {armed=1;printf("{\"event\":\"breakpoints-armed\",\"temporary_bytes\":2}\n");}}
                }
            } else if(code==EXCEPTION_BREAKPOINT && armed && context.Eip==0x473a62) {
                if(!instruction_byte(pi.hProcess,0x473a61,call_bytes[0])){ok=error("restore-source-call-byte");killed=1;TerminateProcess(pi.hProcess,0x4d33000e);}
                if(!read_bytes(pi.hProcess,context.Esp,args,sizeof(args)) || args[0]!=0x80000002 || args[1]!=0x491ce4 || !read_bytes(pi.hProcess,args[1],original_path,46)) {
                    ok=error("source-call-arguments");killed=1;TerminateProcess(pi.hProcess,0x4d330008);
                } else {
                    printf("{\"event\":\"source-registry-call\",\"original_hkey\":\"80000002\",\"original_path_verified\":%s,\"redirected\":%s,\"x87_control\":%lu}\n",!strcmp(original_path,"SOFTWARE\\InterActive Vision\\BHunter\\Paths")?"true":"false",redirect?"true":"false",context.FloatSave.ControlWord&0xffff);
                    if(strcmp(original_path,"SOFTWARE\\InterActive Vision\\BHunter\\Paths")){ok=0;killed=1;TerminateProcess(pi.hProcess,0x4d330009);}
                    else if(redirect){args[0]=0x80000001;args[1]=remote;if(!write_bytes(pi.hProcess,context.Esp,args,8)){ok=error("redirect-stack-arguments");killed=1;TerminateProcess(pi.hProcess,0x4d33000a);}}
                    context.Eip=0x473a61;
                    if(!killed && !SetThreadContext(pi.hThread,&context)){ok=error("resume-restored-call");killed=1;TerminateProcess(pi.hProcess,0x4d33000b);}
                }
            } else if(code==EXCEPTION_BREAKPOINT && armed && context.Eip==0x4723d0) {
                if(!instruction_byte(pi.hProcess,0x4723cf,return_bytes[0])){ok=error("restore-return-byte");killed=1;TerminateProcess(pi.hProcess,0x4d33000f);}
                if(!read_bytes(pi.hProcess,0x473a61,actual,6) || memcmp(actual,call_bytes,6) || !read_bytes(pi.hProcess,0x4723cf,actual,6) || memcmp(actual,return_bytes,6))ok=error("restored-instruction-check");
                printf("{\"event\":\"breakpoint-bytes-restored\",\"verified\":%s}\n",ok?"true":"false");
                printf("{\"event\":\"source-check-return\",\"eax\":%lu,\"accepted\":%s,\"x87_control\":%lu,\"stopped_before_ui\":true}\n",context.Eax,(context.Eax&255)?"true":"false",context.FloatSave.ControlWord&0xffff);
                observed=1;killed=1;TerminateProcess(pi.hProcess,0x4d330001);
            } else {
                printf("{\"event\":\"exception\",\"code\":%lu,\"first_chance\":%lu,\"address\":%lu}\n",code,event.u.Exception.dwFirstChance,(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress);
                handled=DBG_EXCEPTION_NOT_HANDLED;
                if(!event.u.Exception.dwFirstChance){ok=0;killed=1;TerminateProcess(pi.hProcess,0x4d33000c);}
            }
        }
        fflush(stdout);
        if(!ContinueDebugEvent(event.dwProcessId,event.dwThreadId,handled)){ok=error("ContinueDebugEvent");break;}
    }
done:
    if(created){if(!finished){TerminateProcess(pi.hProcess,0x4d33000d);WaitForSingleObject(pi.hProcess,2000);}CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}
    if(registry_created){result=RegDeleteKeyA(HKEY_CURRENT_USER,argv[5]);printf("{\"event\":\"private-key-cleanup\",\"result\":%ld}\n",result);if(result!=ERROR_SUCCESS)ok=0;}
    printf("{\"event\":\"probe-complete\",\"observed_return\":%s,\"passed\":%s}\n",observed?"true":"false",ok&&observed?"true":"false");
    return ok && observed?0:1;
}
