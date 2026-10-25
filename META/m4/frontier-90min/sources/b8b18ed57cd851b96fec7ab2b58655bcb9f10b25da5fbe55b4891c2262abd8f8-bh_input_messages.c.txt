#include "bh_input_messages.h"
LRESULT __cdecl bh_input_message(BHInputSnapshot *snapshot,HWND window,UINT message,WPARAM key,LPARAM bits){
    unsigned char *state;
    if(!snapshot)return DefWindowProcA(window,message,key,bits);
    switch(message){
    case WM_KEYDOWN:case WM_SYSKEYDOWN:
        if(!((unsigned long)bits&0x40000000UL)){
            snapshot->history[snapshot->history_head]=(unsigned char)key;
            snapshot->history_head=(snapshot->history_head+1)&31;
            state=(unsigned char *)snapshot;state[(unsigned long)key&255]=1;
        }
        return 0;
    case WM_KEYUP:case WM_SYSKEYUP:
        if((unsigned long)bits&0x40000000UL){state=(unsigned char *)snapshot;state[(unsigned long)key&255]=0;}
        return 0;
    case WM_LBUTTONDOWN:snapshot->current[1]=1;return 0;
    case WM_LBUTTONUP:snapshot->current[1]=0;return 0;
    case WM_RBUTTONDOWN:snapshot->current[2]=1;return 0;
    case WM_RBUTTONUP:snapshot->current[2]=0;return 0;
    case WM_MBUTTONDOWN:snapshot->current[4]=1;return 0;
    case WM_MBUTTONUP:snapshot->current[4]=0;return 0;
    default:return DefWindowProcA(window,message,key,bits);
    }
}
