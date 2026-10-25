#include "bh_weapon_hud_target.h"
#include "bh_round.h"
#include <float.h>

typedef union TargetPacket {
    BHEntityPositionPacket position;
    unsigned long words[10];
} TargetPacket;
static int fail(BHWeaponHudTargetContext *c,BHWeaponHudTargetError error){if(c)c->error=error;return 0;}
static void rectangle(BHWeaponHudTargetContext *c,double x,double y,float width,float height,
    float angle,float u,float ue,float v,float ve) {
    float sx,sy,depth;int result;
    bh_round_float(&sx,x);bh_round_float(&sy,y);
    bh_round_float(&depth,(double)c->projection->view->near_z-(-.01));
    result=bh_pickup_rect_queue(c->projection->queue,3,c->atlas_image,sx,sy,width,height,angle,
        0,1,u,ue,v,ve,depth);
    ++c->attempted;if(result>0)++c->queued;else if(!result)++c->dropped;else c->error=BH_WEAPON_HUD_TARGET_QUEUE;
}
int __cdecl bh_weapon_hud_target_draw(BHWeaponHudTargetContext *c,const BHWeaponHudState *state) {
    BHEntity *target;TargetPacket packet;long unit,charge,distance,size,bar_width;
    float x,y,z,scale,span,offset,bar_height;
    if(!c || !state)return fail(c,BH_WEAPON_HUD_TARGET_ARGUMENT);
    c->error=BH_WEAPON_HUD_TARGET_OK;c->queries=c->projected=c->attempted=c->queued=c->dropped=0;
    unit=c->viewport_width/160;target=state->target;if(!target)return 1;
    if(!c->projection || !c->projection->queue || !c->projection->camera || !c->projection->view)
        return fail(c,BH_WEAPON_HUD_TARGET_ARGUMENT);
    if(!target->query)return fail(c,BH_WEAPON_HUD_TARGET_CALLBACK);
    packet.position.event=3;++c->queries;target->query(target,&packet);
    x=packet.position.position.x;y=packet.position.position.y;z=packet.position.position.z;
    if(!_finite(x) || !_finite(y) || !_finite(z) || !_finite(c->projection->view->near_z))
        return fail(c,BH_WEAPON_HUD_TARGET_NUMERICAL);
    if(!bh_pickup_project(c->projection,&x,&y,&z,&scale))return 1;
    if(!_finite(x) || !_finite(y) || !_finite(z))return fail(c,BH_WEAPON_HUD_TARGET_NUMERICAL);
    ++c->projected;charge=state->charge;
    if(charge<48){
        distance=(long)((80UL-(unsigned long)charge)*(unsigned long)unit)/4;
        size=unit*4;
        if((unsigned long)charge&4){
            bar_width=unit*13;bh_round_float(&bar_height,(double)(unit*3));
            rectangle(c,x,y,(float)bar_width,bar_height,0,0,.5f,.719f,.109f);
        }
    }else{
        distance=unit*(((unsigned long)charge&4)?8:12);
        size=unit*(((unsigned long)charge&4)?4:6);
        bar_width=unit*16;bh_round_float(&bar_height,(double)unit*5.5);
        rectangle(c,x,y,(float)bar_width,bar_height,0,0,.5f,.828f,.172f);
    }
    bh_round_float(&span,(double)size);bh_round_float(&offset,(double)distance);
    rectangle(c,x,(double)y+offset,span,span,.78539818525314331055f,.25f,.25f,0,.25f);
    rectangle(c,x,(double)y-offset,span,span,3.9269907474517822266f,.25f,.25f,0,.25f);
    rectangle(c,(double)x+offset,y,span,span,5.4977869987487792969f,.25f,.25f,0,.25f);
    rectangle(c,(double)x-offset,y,span,span,2.3561944961547851563f,.25f,.25f,0,.25f);
    return c->error==BH_WEAPON_HUD_TARGET_OK;
}
