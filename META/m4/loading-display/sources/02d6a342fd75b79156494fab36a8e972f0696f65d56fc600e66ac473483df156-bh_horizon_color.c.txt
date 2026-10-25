#include "bh_horizon_color.h"
#include <string.h>
int __cdecl bh_horizon_color_init(BHHorizonColor *c,BHTextureContext *textures) {
    if(!c || !textures || !textures->allocator.allocate || !textures->allocator.release)return 0;
    memset(c,0,sizeof(*c));c->textures=textures;c->allocator=textures->allocator;return 1;
}
void __cdecl bh_horizon_color_dispose(BHHorizonColor *c) {
    if(!c)return;
    if(c->indexed)c->allocator.release(c->indexed,c->allocator.user);
    if(c->packed)c->allocator.release(c->packed,c->allocator.user);
    memset(c,0,sizeof(*c));
}
int __cdecl bh_horizon_color_prepare(BHHorizonColor *c,unsigned long mode,unsigned long color) {
    unsigned long row,i,base[3],constant[3],value[3],packed,j;BHTextureContext *q;
    if(!c || !c->textures || !c->allocator.allocate || !c->allocator.release){if(c)c->error=BH_HORIZON_COLOR_ARGUMENT;return 0;}
    q=c->textures;
    if(!bh_texture_tables_init(q)){c->error=BH_HORIZON_COLOR_ARGUMENT;return 0;}
    if(mode==1) {
        if(c->indexed && color==c->color){c->error=BH_HORIZON_COLOR_OK;return 1;}
        if(!c->indexed)c->indexed=(unsigned char *)c->allocator.allocate(65536,c->allocator.user);
        if(!c->indexed){c->error=BH_HORIZON_COLOR_MEMORY;return 0;}
    } else {
        if(c->packed && color==c->color){c->error=BH_HORIZON_COLOR_OK;return 1;}
        if(!c->packed)c->packed=(unsigned short *)c->allocator.allocate(131072,c->allocator.user);
        if(!c->packed){c->error=BH_HORIZON_COLOR_MEMORY;return 0;}
    }
    constant[0]=(color>>16)&255;constant[1]=(color>>8)&255;constant[2]=color&255;
    for(row=0;row<256;++row)for(i=0;i<256;++i) {
        base[0]=(((i/42)%6)*255)/5;base[1]=(((i/6)%7)*255)/6;base[2]=((i%6)*255)/5;
        for(j=0;j<3;++j)value[j]=(base[j]*row+constant[j]*(255-row))/255;
        if(mode==1)c->indexed[row*256+i]=(unsigned char)((q->tables.quant[1][value[0]]*7+q->tables.quant[2][value[1]])*6+q->tables.quant[1][value[2]]);
        else {packed=0;for(j=0;j<3;++j)packed+=(value[j]>>(q->display_right[j]&31))<<(q->display_left[j]&31);
            c->packed[row*256+i]=(unsigned short)packed;}
    }
    c->color=color;++c->fills;c->error=BH_HORIZON_COLOR_OK;return 1;
}
