#include "bh_frame_render.h"
#include <float.h>
#include <string.h>

static int error(BHFrameDisplay *d,BHFrameRenderError value){
    if(d)d->error=value;return 0;
}
static int native(BHFrameDisplay *d,HRESULT hr){
    ++d->native_calls;d->last_hresult=hr;
    if(FAILED(hr)){++d->failures;d->error=BH_FRAME_RENDER_NATIVE;return 0;}
    return 1;
}
static int rect(BHFrameDisplay *d,RECT *r,long *rows){
    if(!_finite(d->left) || !_finite(d->top) ||
       d->left<0.0f || d->top<0.0f || d->left>640.0f || d->top>480.0f)
        return error(d,BH_FRAME_RENDER_BOUNDS);
    r->left=(long)d->left;r->top=(long)d->top;
    r->right=640;r->bottom=480;
    *rows=(long)((double)480.0-(double)d->top);
    return 1;
}
static int flush_and_cycle(BHFrameDisplay *d){
    BHTextureImage *image,*slow,*fast;int ok=1;
    slow=fast=d->textures->images;
    while(fast && fast->next){
        slow=slow->next;fast=fast->next->next;
        if(slow==fast)return error(d,BH_FRAME_RENDER_TOPOLOGY);
    }
    d->textures->frame=(long)((unsigned long)d->textures->frame+1);
    for(image=d->textures->images;image;image=image->next){
        ++d->flushes;
        if(!bh_texture_draw_flush(d->textures,image,NULL))ok=0;
    }
    if(!native(d,IDirect3DDevice2_EndScene(d->textures->device)))ok=0;
    if(!native(d,IDirect3DDevice2_BeginScene(d->textures->device)))ok=0;
    return ok;
}
int __cdecl bh_frame_clear_depth(BHFrameDisplay *d){
    RECT r;DDBLTFX fx;long rows;int ok=1;
    unsigned long width,bytes,at;long y;
    if(!d || !d->textures)return error(d,BH_FRAME_RENDER_ARGUMENT);
    d->error=BH_FRAME_RENDER_OK;
    if(!d->auxiliary || (!d->textures->software && !d->textures->device))
        return error(d,BH_FRAME_RENDER_BINDING);
    if(!rect(d,&r,&rows))return 0;
    if(!d->textures->software && !flush_and_cycle(d))ok=0;
    if(d->textures->software && d->locked){
        width=(unsigned long)(r.right-r.left);bytes=width*2;
        if(!d->auxiliary_pixels || d->auxiliary_pitch<=0 ||
           bytes>(unsigned long)d->auxiliary_pitch ||
           (unsigned long)r.top*(unsigned long)d->auxiliary_pitch+width*2>d->auxiliary_bytes ||
           (rows>0 && (unsigned long)(r.top+rows-1)*(unsigned long)d->auxiliary_pitch+
               (unsigned long)r.left*2+bytes>d->auxiliary_bytes))
            return error(d,BH_FRAME_RENDER_BOUNDS);
        for(y=0;y<rows;y++){
            at=(unsigned long)(r.top+y)*(unsigned long)d->auxiliary_pitch+
               (unsigned long)r.left*2;
            memset(d->auxiliary_pixels+at,0,bytes);
        }
        return ok;
    }
    memset(&fx,0,sizeof(fx));fx.dwSize=sizeof(fx);fx.dwFillDepth=0;
    if(!native(d,IDirectDrawSurface_Blt(d->auxiliary,&r,NULL,NULL,
        DDBLT_WAIT|DDBLT_DEPTHFILL,&fx)))ok=0;
    return ok;
}
static unsigned long color_value(BHFrameDisplay *d,long red,long green,long blue){
    BHTextureContext *q=d->textures;
    if(d->indexed_color==1)return (unsigned long)
        (((red*5)/255)*42+((green*6)/255)*6+(blue*5)/255);
    return (unsigned long)(((red>>(q->display_right[0]&31))<<(q->display_left[0]&31))+
        ((green>>(q->display_right[1]&31))<<(q->display_left[1]&31))+
        ((blue>>(q->display_right[2]&31))<<(q->display_left[2]&31)));
}
int __cdecl bh_frame_clear_color(BHFrameDisplay *d,long red,long green,long blue){
    RECT r;DDBLTFX fx;DDSURFACEDESC desc;long rows;int ok=1,attempt;
    HRESULT hr;
    if(!d || !d->textures)return error(d,BH_FRAME_RENDER_ARGUMENT);
    d->error=BH_FRAME_RENDER_OK;
    if(red<0 || red>255 || green<0 || green>255 || blue<0 || blue>255)
        return error(d,BH_FRAME_RENDER_ARGUMENT);
    if(!d->color || !d->auxiliary || (!d->textures->software && !d->textures->device))
        return error(d,BH_FRAME_RENDER_BINDING);
    if(!rect(d,&r,&rows))return 0;
    if(!d->textures->software && !flush_and_cycle(d))ok=0;
    memset(&fx,0,sizeof(fx));fx.dwSize=sizeof(fx);
    fx.dwFillColor=color_value(d,red,green,blue);
    if(d->locked){
        if(!native(d,IDirectDrawSurface_Unlock(d->color,NULL)))ok=0;
    }
    if(!native(d,IDirectDrawSurface_Blt(d->color,&r,NULL,NULL,
        DDBLT_WAIT|DDBLT_COLORFILL,&fx)))ok=0;
    if(d->locked){
        for(attempt=0;attempt<500;attempt++){
            memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
            hr=IDirectDrawSurface_Lock(d->color,NULL,&desc,0,NULL);
            ++d->native_calls;d->last_hresult=hr;
            if(hr==DD_OK){
                d->color_pixels=(unsigned char*)desc.lpSurface;
                d->color_pitch=desc.lPitch;
                if(d->atlas)d->atlas->scan_step=(unsigned long)desc.lPitch;
                break;
            }
            ++d->failures;d->error=BH_FRAME_RENDER_NATIVE;
            ok=0;
            Sleep(1);++d->sleeps;
        }
        if(attempt==500){
            bh_frame_clear_depth(d);
            return error(d,BH_FRAME_RENDER_NATIVE);
        }
    }
    if(!bh_frame_clear_depth(d))ok=0;
    if(!ok)d->error=BH_FRAME_RENDER_NATIVE;
    return ok;
}
