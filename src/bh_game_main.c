#include "bh_game_app.h"
#include <string.h>
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command,int show){char path[MAX_PATH],*end,*option;DWORD length;int result,check=0,hardware=0,fullscreen=0,profile=1,box=0,automatic=0;
 (void)previous;for(option=strtok(command," \t");option;option=strtok(0," \t")){if(!strcmp(option,"--startup-check"))check=1;else if(!strcmp(option,"--hardware"))hardware=1;else if(!strcmp(option,"--fullscreen"))fullscreen=1;else if(!strcmp(option,"--box-test"))box=1;else if(!strcmp(option,"--box-auto")){box=1;automatic=1;}else if(!strcmp(option,"--profile"))profile=1;else return 2;}
 if(box)hardware=1;if(hardware)fullscreen=1;
 length=GetModuleFileNameA(instance,path,sizeof(path));if(!length||length>=sizeof(path))return 2;
 if(strstr(path,"BHunter-box.exe")){box=1;hardware=1;fullscreen=1;}
 end=strrchr(path,'\\');if(!end)return 2;*end=0;if(!SetCurrentDirectoryA(path))return 2;
 /* Private preview option bits: bounded startup1, HAL2, fullscreen4, profile8. */
 result=bh_game_app_run(instance,"..\\..\\GAME_DATA",check|hardware*2|fullscreen*4|profile*8|box*16|automatic*32,check?SW_HIDE:show);
 if(result&&!check&&!automatic)MessageBoxA(0,"The source preview stopped at an explicit dependency error. See build\\game\\startup.log for details.","BHunter source preview",MB_OK|MB_ICONERROR);
 return result;
}
