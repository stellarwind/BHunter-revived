#include "bh_math.h"
#include <math.h>
#include <stddef.h>

typedef char check_vec_size[sizeof(BHVec3)==12 ? 1 : -1];
typedef char check_matrix_size[sizeof(BHMat34)==48 ? 1 : -1];
typedef char check_vector_z[offsetof(BHVec3,z)==8 ? 1 : -1];

/* 00401000. Distinct output required; the original writes sequentially. */
void __cdecl bh_mat34_multiply(const BHMat34 *a, const BHMat34 *b, BHMat34 *out)
{
    int r, c;
    for (r=0;r<3;r++) {
        for (c=0;c<3;c++)
            out->m[r][c]=a->m[r][0]*b->m[0][c]+a->m[r][1]*b->m[1][c]+a->m[r][2]*b->m[2][c];
        out->m[r][3]=a->m[r][0]*b->m[0][3]+a->m[r][1]*b->m[1][3]+a->m[r][2]*b->m[2][3]+a->m[r][3];
    }
}

/* 004011a0. Preserve sequential writes; this is not an alias-safe cross product. */
void __cdecl bh_vec3_cross(const BHVec3 *a, const BHVec3 *b, BHVec3 *out)
{
    out->x=a->y*b->z-b->y*a->z;
    out->y=a->z*b->x-b->z*a->x;
    out->z=b->y*a->x-a->y*b->x;
}

/* 004011f0. The assembly retains the length and reciprocal on the x87 stack. */
int __cdecl bh_vec3_normalize(BHVec3 *v)
{
    double length=sqrt(v->x*v->x+v->y*v->y+v->z*v->z);
    if (length==0.0) return 0;
    length=1.0/length;
    v->x=(float)(length*v->x);
    v->y=(float)(length*v->y);
    v->z=(float)(length*v->z);
    return 1;
}

/* 00401270. Zero input leaves the output untouched. */
int __cdecl bh_vec3_normalize_to(const BHVec3 *v, BHVec3 *out)
{
    double length=sqrt(v->x*v->x+v->y*v->y+v->z*v->z);
    if (length==0.0) return 0;
    length=1.0/length;
    out->x=(float)(length*v->x);
    out->y=(float)(length*v->y);
    out->z=(float)(length*v->z);
    return 1;
}

/* 004012e0. Preserve the decompiled expression grouping for comparison. */
void __cdecl bh_mat34_transform_point(const BHMat34 *m, const BHVec3 *v, BHVec3 *out)
{
    out->x=v->x*m->m[0][0]+v->z*m->m[0][2]+v->y*m->m[0][1]+m->m[0][3];
    out->y=m->m[1][0]*v->x+m->m[1][2]*v->z+v->y*m->m[1][1]+m->m[1][3];
    out->z=m->m[2][0]*v->x+m->m[2][2]*v->z+v->y*m->m[2][1]+m->m[2][3];
}

/* 00401360. */
void __cdecl bh_mat34_transform_direction(const BHMat34 *m, const BHVec3 *v, BHVec3 *out)
{
    out->x=v->x*m->m[0][0]+v->y*m->m[0][1]+v->z*m->m[0][2];
    out->y=m->m[1][0]*v->x+m->m[1][1]*v->y+v->z*m->m[1][2];
    out->z=m->m[2][0]*v->x+m->m[2][1]*v->y+v->z*m->m[2][2];
}

/* 004013d0. R-transpose * (v-t); inverse only for an orthonormal basis. */
void __cdecl bh_mat34_inverse_transform_point(const BHMat34 *m, const BHVec3 *v, BHVec3 *out)
{
    float x=v->x-m->m[0][3], y=v->y-m->m[1][3], z=v->z-m->m[2][3];
    out->x=x*m->m[0][0]+m->m[2][0]*z+m->m[1][0]*y;
    out->y=m->m[2][1]*z+m->m[1][1]*y+m->m[0][1]*x;
    out->z=m->m[2][2]*z+m->m[1][2]*y+m->m[0][2]*x;
}

/* 00401460. */
void __cdecl bh_mat34_translate_world(float x, float y, float z, BHMat34 *m)
{
    m->m[0][3]=x+m->m[0][3];
    m->m[1][3]=y+m->m[1][3];
    m->m[2][3]=z+m->m[2][3];
}

/* 00401490. */
void __cdecl bh_mat34_translate_local(float x, float y, float z, BHMat34 *m)
{
    m->m[0][3]=x*m->m[0][0]+m->m[0][1]*y+m->m[0][2]*z+m->m[0][3];
    m->m[1][3]=m->m[1][1]*y+m->m[1][2]*z+m->m[1][0]*x+m->m[1][3];
    m->m[2][3]=m->m[2][1]*y+m->m[2][2]*z+m->m[2][0]*x+m->m[2][3];
}

/* 00401a70. Not a general matrix inverse; output must be distinct. */
void __cdecl bh_mat34_inverse_rigid(const BHMat34 *m, BHMat34 *out)
{
    int r,c;
    for(r=0;r<3;r++) for(c=0;c<3;c++) out->m[r][c]=m->m[c][r];
    for(r=0;r<3;r++)
        out->m[r][3]=-m->m[0][3]*out->m[r][0]-out->m[r][1]*m->m[1][3]-m->m[2][3]*out->m[r][2];
}
