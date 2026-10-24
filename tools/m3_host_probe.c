/* Read-only host availability diagnostic. No original game code or sessions. */
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <initguid.h>
#include <dplay.h>
#include <stdio.h>
#include <string.h>

static unsigned connections;
static void json_string(const char *p) {
    unsigned char c;putchar('"');
    while(p && (c=(unsigned char)*p++)) {
        if(c=='"' || c=='\\'){putchar('\\');putchar(c);}
        else if(c<32 || c>=127)printf("\\u%04x",c);else putchar(c);
    }putchar('"');
}
static BOOL FAR PASCAL connection(LPCGUID guid,LPVOID address,DWORD bytes,LPCDPNAME name,DWORD flags,LPVOID context) {
    ++connections;
    printf("{\"event\":\"directplay-provider\",\"guid\":\"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\",\"name\":",guid->Data1,guid->Data2,guid->Data3,guid->Data4[0],guid->Data4[1],guid->Data4[2],guid->Data4[3],guid->Data4[4],guid->Data4[5],guid->Data4[6],guid->Data4[7]);
    json_string(name->lpszShortNameA);printf(",\"connection_bytes\":%lu,\"flags\":%lu}\n",bytes,flags);return TRUE;
}
int main(void) {
    DWORD drives=GetLogicalDrives(),i;char path[4]="A:\\",error[256];MCI_OPEN_PARMSA open;
    MCI_STATUS_PARMS status;MCIERROR result,closed;HRESULT initialized,created,enumerated;
    LPDIRECTPLAY3A play=NULL;
    printf("{\"event\":\"host-api-probe\",\"pointer_bits\":%u,\"original_code_executes\":false}\n",sizeof(void*)*8);
    for(i=0;i<26;i++)if(drives&(1UL<<i)) {
        path[0]=(char)('A'+i);
        if(GetDriveTypeA(path)==DRIVE_CDROM)printf("{\"event\":\"cdrom-drive\",\"letter\":\"%c\"}\n",path[0]);
    }
    printf("{\"event\":\"multimedia-device-counts\",\"aux\":%u,\"waveout\":%u}\n",auxGetNumDevs(),waveOutGetNumDevs());
    memset(&open,0,sizeof(open));open.lpstrDeviceType="cdaudio";
    result=mciSendCommandA(0,MCI_OPEN,MCI_OPEN_TYPE,(DWORD)&open);
    memset(error,0,sizeof(error));mciGetErrorStringA(result,error,sizeof(error));
    printf("{\"event\":\"mci-cdaudio-open\",\"error\":%lu,\"device_id\":%u,\"error_text\":",result,open.wDeviceID);json_string(error);printf("}\n");
    if(!result) {
        memset(&status,0,sizeof(status));status.dwItem=MCI_STATUS_NUMBER_OF_TRACKS;
        result=mciSendCommandA(open.wDeviceID,MCI_STATUS,MCI_STATUS_ITEM,(DWORD)&status);
        printf("{\"event\":\"mci-cdaudio-tracks\",\"error\":%lu,\"tracks\":%lu}\n",result,status.dwReturn);
        closed=mciSendCommandA(open.wDeviceID,MCI_CLOSE,0,0);
        printf("{\"event\":\"mci-cdaudio-close\",\"error\":%lu}\n",closed);
        if(closed)return 2;
    }
    initialized=CoInitialize(NULL);printf("{\"event\":\"com-initialize\",\"hresult\":%lu}\n",(DWORD)initialized);
    if(SUCCEEDED(initialized)) {
        created=CoCreateInstance(&CLSID_DirectPlay,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectPlay3A,(void**)&play);
        printf("{\"event\":\"directplay3a-create\",\"hresult\":%lu,\"interface_created\":%s}\n",(DWORD)created,play?"true":"false");
        if(SUCCEEDED(created) && play) {
            enumerated=IDirectPlay3_EnumConnections(play,NULL,connection,NULL,DPCONNECTION_DIRECTPLAY);
            printf("{\"event\":\"directplay-enumeration\",\"hresult\":%lu,\"providers\":%u}\n",(DWORD)enumerated,connections);
            IDirectPlay3_Release(play);
        }
        CoUninitialize();
    }
    printf("{\"event\":\"host-probe-complete\",\"playback_requested\":false,\"volume_write_requested\":false,\"network_session_requested\":false}\n");return 0;
}
