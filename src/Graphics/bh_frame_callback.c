#include "bh_frame_callback.h"
void bh_frame_callback_set(BHFrameCallback *frame_callback, BHFrameRenderCallback render)
{
    if (frame_callback)
        frame_callback->render = render;
}
int bh_frame_callback_start(BHFrameCallback *c)
{
    if (!c)
        return 0;
    bh_frame_callback_set(c, bh_render_execute);
    return 1;
}
