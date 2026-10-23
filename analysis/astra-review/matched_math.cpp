/* Four complete raw byte matches under the fixed VC5 SP3 C++ setup.
   Preserve this translation-unit context; see FIXED_SETUP_FINDINGS.md.
   Types/names/temporaries are reconstructions, not recovered original source. */
#include <math.h>
typedef struct Vec3 {float x,y,z;} Vec3;
typedef struct Mat34 {float m[3][4];} Mat34;
extern "C" {

// BEGIN bh_vec3_cross
void __cdecl bh_vec3_cross(const float *a,const float *b,float *out)
{
    {
        float t1=b[2]; float t0=a[1]; float t2=b[1]; float t3=a[2];
        out[0]=(float)(t0*t1-t2*t3);
    }
    {
        float q=b[2]; float p=a[2];
        out[1]=(float)(p*b[0]-q*a[0]);
    }
    {
        float q=a[1]; float p=b[1];
        out[2]=(float)(p*a[0]-q*b[0]);
    }
}
// END bh_vec3_cross

// BEGIN bh_mat34_transform_point
void __cdecl bh_mat34_transform_point(const float *m,const float *v,float *out)
{
    {
        double t0=v[0]; double t1=v[1]; double t2=v[2];
        out[0]=(float)((t0*m[0]+(t2*m[2]+t1*m[1]))+m[3]);
    }
    {
        float t0=m[5]; double t1=v[1]; double t2=m[6]; float t3=m[4];
        out[1]=(float)((v[0]*t3+(t0*t1+t2*v[2]))+m[7]);
    }
    {
        float t0=m[9]; double t1=v[1]; double t2=m[10]; float t3=m[8];
        out[2]=(float)((v[0]*t3+(t0*t1+t2*v[2]))+m[11]);
    }
}
// END bh_mat34_transform_point

// BEGIN bh_mat34_transform_direction
void __cdecl bh_mat34_transform_direction(const float *m,const float *v,float *out)
{
    {
        double t0=v[0]; double t1=v[1]; double t2=v[2];
        out[0]=(float)(m[0]*t0+(t1*m[1]+m[2]*t2));
    }
    {
        double t0=m[6]; double t2=m[5]; float t3=m[4]; float t1=v[2];
        out[1]=(float)(t3*v[0]+(t1*t0+v[1]*t2));
    }
    {
        double t0=m[10]; double t2=m[9]; float t3=m[8]; float t1=v[2];
        out[2]=(float)(t3*v[0]+(t1*t0+v[1]*t2));
    }
}
// END bh_mat34_transform_direction

// BEGIN bh_mat34_translate_world
void __cdecl bh_mat34_translate_world(float x,float y,float z,float *m)
{
    m[3]=(float)(m[3]+x);
    m[7]=(float)(y+m[7]);
    m[11]=(float)(z+m[11]);
}
// END bh_mat34_translate_world

}
