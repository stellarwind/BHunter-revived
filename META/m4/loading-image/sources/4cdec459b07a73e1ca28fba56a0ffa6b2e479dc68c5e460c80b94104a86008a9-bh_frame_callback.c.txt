#include "bh_frame_callback.h"
void __cdecl bh_frame_callback_set(BHFrameCallback *c,BHFrameRenderCallback render){
    if(c)c->render=render;
}
int __cdecl bh_frame_callback_start(BHFrameCallback *c){
    if(!c)return 0;
    bh_frame_callback_set(c,bh_render_execute);return 1;
}
