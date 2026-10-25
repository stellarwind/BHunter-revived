#include "../src/bh_input_messages.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL input messages: %s\n",s);}}while(0)
static LRESULT CALLBACK test_window(HWND window,UINT message,WPARAM w,LPARAM l){
    BHInputSnapshot *s=(BHInputSnapshot *)GetWindowLongA(window,GWL_USERDATA);
    if(message==WM_NCCREATE){s=(BHInputSnapshot *)((CREATESTRUCTA *)l)->lpCreateParams;SetWindowLongA(window,GWL_USERDATA,(LONG)s);}
    return bh_input_message(s,window,message,w,l);
}
static void keys(void){BHInputSnapshot s,before;unsigned long key,sys,head;
    for(sys=0;sys<2;++sys)for(key=0;key<512;++key){memset(&s,0x5a,sizeof(s));s.history_head=31;before=s;
        CHECK(!bh_input_message(&s,NULL,sys?WM_SYSKEYDOWN:WM_KEYDOWN,key,0),"first press swallowed");
        CHECK(s.history[31]==(unsigned char)key && s.history_head==0 && ((unsigned char *)&s)[key&255]==1,"VK low byte and head wrap");
        before=s;CHECK(!bh_input_message(&s,NULL,sys?WM_SYSKEYDOWN:WM_KEYDOWN,key,0x40000000L) && !memcmp(&s,&before,sizeof(s)),"autorepeat swallowed without state or history change");
        CHECK(!bh_input_message(&s,NULL,sys?WM_SYSKEYUP:WM_KEYUP,key,0) && !memcmp(&s,&before,sizeof(s)),"release without previous-state flag swallowed unchanged");
        CHECK(!bh_input_message(&s,NULL,sys?WM_SYSKEYUP:WM_KEYUP,key,0xc0000000UL) && ((unsigned char *)&s)[key&255]==0 && s.history_head==0,"true release clears low-byte VK");
    }
    memset(&s,0,sizeof(s));for(key=0;key<100;++key)bh_input_message(&s,NULL,WM_KEYDOWN,key,0);
    CHECK(s.history_head==4,"100 first presses wrap32-byte history");for(head=0;head<32;++head)CHECK(s.history[head]==(unsigned char)(head<4?head+96:head+64),"history contains actual final32 VK presses");
    CHECK(s.previous[0]==0,"unwritten VK255 alias initially zero");bh_input_message(&s,NULL,WM_KEYDOWN,255,0);CHECK(s.previous[0]==1,"VK255 writes exact previous/current overlapping byte");
}
static void native(void){WNDCLASSA cls;HWND window;BHInputSnapshot s,before;BHPlayerInputList list;unsigned long i;LRESULT expected;
    memset(&cls,0,sizeof(cls));cls.lpfnWndProc=test_window;cls.hInstance=GetModuleHandleA(NULL);cls.lpszClassName="BHunterSourceInputMessageFixture";
    CHECK(RegisterClassA(&cls)!=0,"register private source-only test window");memset(&s,0,sizeof(s));
    window=CreateWindowExA(0,cls.lpszClassName,"source input fixture",WS_POPUP,0,0,1,1,NULL,NULL,cls.hInstance,&s);
    CHECK(window!=NULL,"create hidden private window, no game process");if(window){
        CHECK(!IsWindowVisible(window),"fixture never shown");
        SendMessageA(window,WM_KEYDOWN,'A',0);SendMessageA(window,WM_SYSKEYDOWN,'B',0);SendMessageA(window,WM_KEYDOWN,'A',0x40000000L);
        CHECK(s.current['A']==1 && s.current['B']==1 && s.history_head==2 && s.history[0]=='A' && s.history[1]=='B',"actual Win32 dispatch reaches recovered procedure");
        memset(&list,0,sizeof(list));bh_player_input_bind_key(&list,0,"FIRE",0,0,'A');list.count=1;
        CHECK(bh_input_evaluate(&list,&s,1) && list.state[1]>0,"message state consumed by real admitted binding evaluator");
        SendMessageA(window,WM_KEYUP,'A',0x40000000L);CHECK(s.current['A']==0,"private-window keyup dispatch");
        {const UINT down[3]={WM_LBUTTONDOWN,WM_RBUTTONDOWN,WM_MBUTTONDOWN},up[3]={WM_LBUTTONUP,WM_RBUTTONUP,WM_MBUTTONUP};const unsigned long index[3]={1,2,4};
         for(i=0;i<3;++i){SendMessageA(window,down[i],0,0);CHECK(s.current[index[i]]==1,"native button-down mapping");SendMessageA(window,up[i],0,0);CHECK(s.current[index[i]]==0,"native button-up mapping");}}
        before=s;expected=DefWindowProcA(window,WM_GETDLGCODE,0,0);CHECK(SendMessageA(window,WM_GETDLGCODE,0,0)==expected && !memcmp(&s,&before,sizeof(s)),"unhandled message delegates real default procedure without state mutation");
        CHECK(DestroyWindow(window),"private window orderly destruction");
    }
    CHECK(UnregisterClassA(cls.lpszClassName,cls.hInstance),"private window class cleanup");
}
int bh_input_messages_checkpoint(unsigned long *count){checks=0;failures=0;keys();native();if(count)*count=checks;
    printf("{\"scenario\":\"input-messages\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
