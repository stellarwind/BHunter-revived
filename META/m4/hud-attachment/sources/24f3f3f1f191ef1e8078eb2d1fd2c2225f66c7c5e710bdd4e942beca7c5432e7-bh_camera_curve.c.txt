#include "bh_camera_curve.h"
#include "bh_round.h"
static float stored(double x) {float out;bh_round_float(&out,x);return out;}
void __cdecl bh_camera_curve(float t,BHVec3 *out,const BHVec3 *first,
    const BHVec3 *first_tangent,const BHVec3 *last_tangent,const BHVec3 *last) {
    BHVec3 a=*first,b=*first_tangent,c=*last_tangent,d=*last,p,result;
    double u,cube,blend_inverse,last_x,last_y;float square,one_minus_t,blend,inverse,last_z;
    if(t<=0) {
        result.x=stored(((double)b.x-a.x)*t+a.x);
        result.y=stored((double)a.y+((double)b.y-a.y)*t);
        result.z=stored(((double)b.z-a.z)*t+a.z);
    } else if(t>=1) {
        u=1.0-(double)t;
        result.x=stored((double)d.x+((double)c.x-d.x)*u);
        result.y=stored((double)d.y+((double)c.y-d.y)*u);
        result.z=stored((double)d.z+((double)c.z-d.z)*u);
    } else {
        square=stored((double)t*t);one_minus_t=stored(1.0-(double)t);
        p.x=stored((double)a.x*one_minus_t+(double)b.x*t);
        p.y=stored((double)b.y*t+(double)a.y*one_minus_t);
        p.z=stored((double)b.z*t+(double)a.z*one_minus_t);
        last_x=(double)d.x*t+(double)c.x*one_minus_t;
        last_y=(double)c.y*one_minus_t+(double)d.y*t;
        last_z=stored((double)c.z*one_minus_t+(double)d.z*t);
        cube=(double)t*square;
        blend=stored((double)square*3.0-(cube+cube));
        blend_inverse=1.0-(double)blend;inverse=stored(blend_inverse);
        result.x=stored(blend_inverse*p.x+last_x*blend);
        result.y=stored((double)inverse*p.y+last_y*blend);
        result.z=stored((double)inverse*p.z+(double)last_z*blend);
    }
    *out=result;
}
