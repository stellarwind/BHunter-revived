#include "bh_sprite_billboard.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <string.h>

static int fail(BHSpriteBillboardContext *c,BHSpriteBillboardError error) {
    if(c)c->error=error;
    return 0;
}
static void point(BHClipVertex *p,float x,float y,float u,float v) {
    p->sx=x;p->sy=y;p->u=u;p->v=v;
}
int __cdecl bh_sprite_billboard_draw(BHSpriteBillboardContext *c,BHTextureImage *image,
    float x,float y,float z,float width,float height,float angle,
    unsigned long flags,float modulation) {
    BHVec3 world,projected;BHClipVertex quad[4],*ring,*p;
    float cx,cy,half_width,half_height,radius,cs,sn,hc,hs,ws,hsin;
    float u0,u1,v0,v1;double conversion;int shade;unsigned long i,steps;
    if(!c || !image || !c->camera || !c->config || !c->clip || !c->emit ||
       !_finite(x) || !_finite(y) || !_finite(z) || !_finite(width) ||
       !_finite(height) || !_finite(angle) || !_finite(modulation) ||
       !_finite(c->camera->focal) || c->camera->focal<=0 ||
       !_finite(c->config->near_z) || !_finite(c->config->far_z) ||
       c->config->near_z<=0 || c->config->far_z<c->config->near_z)
        return fail(c,BH_SPRITE_BILLBOARD_ARGUMENT);
    c->error=BH_SPRITE_BILLBOARD_OK;c->emitted=0;
    world.x=x;world.y=y;world.z=z;
    bh_mat34_transform_point(&c->camera->projected,&world,&projected);
    if(!_finite(projected.x) || !_finite(projected.y) || !_finite(projected.z))return fail(c,BH_SPRITE_BILLBOARD_NUMERICAL);
    if(projected.z<c->config->near_z || c->config->far_z<projected.z)return 1;
    bh_round_float(&half_width,((double)width*((double)c->camera->focal/projected.z))*.5);
    bh_round_float(&half_height,((double)height*((double)c->camera->focal/projected.z))*.5);
    bh_round_float(&cx,(double)projected.x/projected.z);
    bh_round_float(&cy,(double)projected.y/projected.z);
    bh_round_float(&radius,sqrt((double)half_height*half_height+(double)half_width*half_width));
    if(!_finite(half_width) || !_finite(half_height) || !_finite(cx) || !_finite(cy) || !_finite(radius))
        return fail(c,BH_SPRITE_BILLBOARD_NUMERICAL);
    if(!(c->config->left<radius+cx && cx-radius<c->config->right &&
         c->config->top<radius+cy && cy-radius<c->config->bottom))return 1;
    u0=(flags&1)?1.0f:0.0f;u1=1.0f-u0;
    v0=(flags&2)?1.0f:0.0f;v1=1.0f-v0;
    memset(quad,0,sizeof(quad));
    point(quad+0,0,0,u0,v0);
    if(flags&4) {
        point(quad+1,0,0,u1,v0);
        point(quad+2,0,0,u1,v1);
        point(quad+3,0,0,u0,v1);
    }else {
        point(quad+1,0,0,u0,v1);
        point(quad+2,0,0,u1,v1);
        point(quad+3,0,0,u1,v0);
    }
    if(angle==0.0f) {
        bh_round_float(&quad[0].sx,(double)cx-half_width);
        bh_round_float(&quad[0].sy,(double)cy-half_height);
        quad[1].sx=quad[0].sx;
        bh_round_float(&quad[1].sy,(double)cy+half_height);
        bh_round_float(&quad[2].sx,(double)cx+half_width);
        quad[2].sy=quad[1].sy;
        quad[3].sx=quad[2].sx;quad[3].sy=quad[0].sy;
    }else {
        bh_round_float(&cs,cos((double)angle));
        bh_round_float(&sn,sin((double)angle));
        bh_round_float(&hc,(double)half_width*cs);
        bh_round_float(&hs,(double)half_height*cs);
        bh_round_float(&ws,(double)half_width*sn);
        bh_round_float(&hsin,-(double)half_height*sn);
        bh_round_float(&quad[0].sx,((double)cx-hc)-hsin);
        bh_round_float(&quad[0].sy,((double)cy-ws)-hs);
        bh_round_float(&quad[1].sx,(double)hsin+((double)cx-hc));
        bh_round_float(&quad[1].sy,(double)hs+((double)cy-ws));
        bh_round_float(&quad[2].sx,-(double)half_height*sn+(double)hc+cx);
        bh_round_float(&quad[2].sy,(double)hs+ws+cy);
        bh_round_float(&quad[3].sx,((double)hc+cx)-hsin);
        bh_round_float(&quad[3].sy,((double)ws+cy)-hs);
    }
    for(i=0;i<4;++i){quad[i].next=quad+(i+1)%4;quad[i].previous=quad+(i+3)%4;}
    ring=bh_screen_clip_polygon(c->clip,quad,c->config->near_z);
    if(!ring) {
        if(c->clip->error!=BH_CLIP_OK)return fail(c,BH_SPRITE_BILLBOARD_CLIP);
        return 1;
    }
    conversion=(double)modulation*-255.0;
    if(!_finite(conversion) || conversion<-2147483648.0 || conversion>2147483647.0)
        return fail(c,BH_SPRITE_BILLBOARD_NUMERICAL);
    shade=(int)conversion;c->color=c->base_color-(unsigned long)shade*0x10101UL;
    p=ring;steps=0;do {
        if(++steps>12)return fail(c,BH_SPRITE_BILLBOARD_CLIP);
        p->point.z=projected.z;p=p->next;
    }while(p!=ring);
    if(!c->emit(ring,image,c->color,c->emit_user))return fail(c,BH_SPRITE_BILLBOARD_EMIT);
    c->emitted=1;return 1;
}
