#include "bh_force_feedback_update.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>
#include <string.h>
#include <stddef.h>
typedef char force_update_layout[sizeof(DIEFFECT)==52&&offsetof(DIEFFECT,lpvTypeSpecificParams)==48?1:-1];
int __cdecl bh_force_feedback_prepare(BHForceFeedbackUpdate *c,unsigned long tick,
    float x,float y,BHForceFeedbackParameters *out){
    double dx,dy;float stored_y;long elapsed;
    if(!c||!c->force||!out)return -1;
    if(!c->force->enabled)return 0;
    if(!_finite(x)||!_finite(y)||!_finite(c->x)||!_finite(c->y))return -1;
    if(x< -1)x=-1;else if(x>1)x=1;
    if(y< -1)y=-1;else if(y>1)y=1;
    dx=(double)x-c->x;if(dx<0)dx=-dx;
    dy=(double)y-c->y;if(dy<0)dy=-dy;
    bh_round_float(&stored_y,dy);
    if(dx<dy)dx=stored_y;
    elapsed=(long)(tick-c->last_tick);
    if(dx==0 || (double)elapsed<.5/dx)return 0;
    c->x=x;c->y=y;c->last_tick=tick;
    memset(out,0,sizeof(*out));
    out->direction[0]=(long)((double)x*10000.0);
    out->direction[1]=(long)((double)y*10000.0);
    out->magnitude.lMagnitude=(long)(sqrt((double)x*x+(double)y*y)*10000.0);
    out->effect.dwSize=52;out->effect.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
    out->effect.cAxes=2;out->effect.rglDirection=out->direction;
    out->effect.cbTypeSpecificParams=4;out->effect.lpvTypeSpecificParams=&out->magnitude;
    return 1;
}
int __cdecl bh_force_feedback_update(BHForceFeedbackUpdate *c,unsigned long tick,float x,float y){
    BHForceFeedbackParameters parameters;int ready;
    if(!c||!c->force)return 0;
    if(c->force->enabled&&!c->force->effect)return 0;
    ready=bh_force_feedback_prepare(c,tick,x,y,&parameters);
    if(ready<=0)return ready==0;
    ++c->calls;
    c->result=IDirectInputEffect_SetParameters(c->force->effect,&parameters.effect,0x20000140UL);
    return SUCCEEDED(c->result);
}
