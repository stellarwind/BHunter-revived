#include "bh_loading_display.h"
#include "bh_round.h"
#include <string.h>
#include <float.h>
static void restore(BHLoadingDisplay *c,LPDIRECTDRAWSURFACE surface){
    if(surface&&IDirectDrawSurface_IsLost(surface)==DDERR_SURFACELOST){
        ++c->restores;IDirectDrawSurface_Restore(surface);
    }
}
static int wedge(BHLoadingDisplay *c,const BHLoadingBlitTarget *t,int n,float a,float b){
    ++c->wedge_calls;
    switch(n){
    case 0:return bh_loading_blit_top(&c->blit,t,a,b);
    case 1:return bh_loading_blit_right(&c->blit,t,a,b);
    case 2:return bh_loading_blit_bottom(&c->blit,t,a,b);
    case 3:return bh_loading_blit_left(&c->blit,t,a,b);
    default:return bh_loading_blit_top_last(&c->blit,t,a,b);
    }
}
int __cdecl bh_loading_display(BHLoadingDisplay *c,float progress){
    BHLoadingImages *im;DDSURFACEDESC desc;BHLoadingBlitTarget target;
    const unsigned char *indices,*source;const unsigned long *native;
    unsigned char *row,*out;unsigned long offset,now,x,y,value,depth,pitch_bytes,row_offset;
    unsigned __int64 span,needed;
    long old_redraw=0;float previous;int ok=1;
    if(!c)return 0;++c->calls;im=c->images;
    if(!im||!im->arena||!c->color)return 1;
    if(im->stage==0&&im->redraw_count==2){++im->tick_calls;im->deadline=GetTickCount()+5000UL;}
    if(im->stage<2){
        ++im->tick_calls;now=GetTickCount();old_redraw=im->redraw_count;
        if(im->redraw_count)im->redraw_count=(long)((unsigned long)im->redraw_count-1UL);
        if((long)(now-im->deadline)>=0){
            im->stage=(long)((unsigned long)im->stage+1UL);
            if(im->stage==1){++im->tick_calls;im->deadline=GetTickCount()+5000UL;im->redraw_count=2;}
            else if(im->stage==2){c->previous[0]=c->previous[1]=-1.0f;c->baseline=progress;
                bh_round_float(&c->scale,1.0/(1.0-(double)progress));}
        }
        if(!old_redraw)return 1;
    }
    if(im->stage<0||im->stage>2||!c->primary||c->width<=0||c->height<=0||
       c->width>640||c->height>480){++c->failures;return 0;}
    offset=im->stage==0?0x96000UL:im->stage==1?0xe1000UL:0;
    if(offset>im->arena_bytes||(unsigned long)c->width*(unsigned long)c->height>im->arena_bytes-offset){++c->failures;return 0;}
    indices=im->stage==0?im->midas_indexed:im->stage==1?im->iel_indexed:im->indexed;
    native=im->stage==0?im->midas_native:im->stage==1?im->iel_native:im->native;
    bh_round_float(&c->normalized,((double)progress-c->baseline)*c->scale);
    restore(c,c->color);memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
    c->lock_result=IDirectDrawSurface_Lock(c->color,NULL,&desc,0,NULL);
    if(c->lock_result!=DD_OK){++c->failures;return 0;}++c->locks;
    depth=im->pixel_bytes;
    pitch_bytes=desc.lPitch<0?0UL-(unsigned long)desc.lPitch:(unsigned long)desc.lPitch;
    span=(unsigned __int64)pitch_bytes*(unsigned long)c->height;
    needed=(unsigned __int64)(unsigned long)c->width*depth;
    /* Lock returns logical row0, including for bottom-up native storage. The
       wedge span uses the lower allocation address and a signed row0 offset. */
    row_offset=desc.lPitch<0?(unsigned long)((unsigned __int64)pitch_bytes*(c->height-1)):0;
    if(!pitch_bytes||pitch_bytes<needed||span>0xffffffffUL||
       (desc.lPitch<0&&((unsigned __int64)pitch_bytes*(c->height-1)>0x7fffffffUL||
                        (unsigned long)desc.lpSurface<row_offset))||
       (unsigned __int64)((unsigned long)desc.lpSurface-row_offset)+span>0x100000000ui64||
       desc.dwWidth<(unsigned long)c->width||desc.dwHeight<(unsigned long)c->height){
        IDirectDrawSurface_Unlock(c->color,NULL);++c->unlocks;++c->failures;return 0;}
    if(c->alternate){previous=c->previous[0];c->previous[0]=c->normalized;c->alternate=0;}
    else{previous=c->previous[1];c->previous[1]=c->normalized;c->alternate=1;}
    source=im->arena+offset;
    if(_isnan(previous)||previous<0.0f||old_redraw){
        ++c->full_copies;
        row=(unsigned char*)desc.lpSurface;
        for(y=0;y<(unsigned long)c->height;++y){out=row;
            for(x=0;x<(unsigned long)c->width;++x){value=*source++;
                if(depth==1)*out++=indices[value];
                else if(depth==2){value=native[value];*out++=(unsigned char)value;*out++=(unsigned char)(value>>8);}
                else if(depth==3){value=native[value];*out++=(unsigned char)(value>>16);*out++=(unsigned char)(value>>8);*out++=(unsigned char)value;}
                else if(depth==4){value=native[value];memcpy(out,&value,4);out+=4;}
            }if(y+1<(unsigned long)c->height)row+=desc.lPitch;
        }previous=0.0f;
    }
    c->blit.images=im;c->blit.width=c->width;c->blit.height=c->height;
    target.base=(unsigned char*)desc.lpSurface-row_offset;target.bytes=(unsigned long)span;
    target.row0=(long)row_offset;target.pitch=desc.lPitch;
    if(!old_redraw&&!_isnan(c->normalized)&&!_isnan(previous)&&c->normalized!=previous){
        if(!(c->normalized>0.125f))ok=wedge(c,&target,0,previous,c->normalized)&&ok;
        else{ok=wedge(c,&target,0,previous,0.125f)&&ok;
            if(!(c->normalized>0.375f))ok=wedge(c,&target,1,0.125f,c->normalized)&&ok;
            else{ok=wedge(c,&target,1,0.125f,0.375f)&&ok;
                if(!(c->normalized>0.625f))ok=wedge(c,&target,2,0.375f,c->normalized)&&ok;
                else{ok=wedge(c,&target,2,0.375f,0.625f)&&ok;
                    if(!(c->normalized>0.875f))ok=wedge(c,&target,3,0.625f,c->normalized)&&ok;
                    else{ok=wedge(c,&target,3,0.625f,0.875f)&&ok;
                        ok=wedge(c,&target,4,0.875f,c->normalized>1.0f?1.0f:c->normalized)&&ok;}}}}
    }
    c->unlock_result=IDirectDrawSurface_Unlock(c->color,NULL);++c->unlocks;
    restore(c,c->color);
    if(c->software&&c->software_target){restore(c,c->software_target);
        c->copy_result=IDirectDrawSurface_BltFast(c->software_target,0,0,c->color,NULL,DDBLTFAST_WAIT);}
    restore(c,c->primary);restore(c,c->back);++c->flips;
    c->flip_result=IDirectDrawSurface_Flip(c->primary,NULL,DDFLIP_WAIT);
    if(!ok||FAILED(c->flip_result)){++c->failures;return 0;}return 1;
}
