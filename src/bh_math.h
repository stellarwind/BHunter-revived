#ifndef BH_MATH_H
#define BH_MATH_H

/* Recovered binary layouts. Names are ours; original source names are unknown. */
typedef struct BHVec3 { float x, y, z; } BHVec3;
typedef struct BHMat34 { float m[3][4]; } BHMat34;

void __cdecl bh_mat34_multiply(const BHMat34 *a, const BHMat34 *b, BHMat34 *out);
void __cdecl bh_vec3_cross(const BHVec3 *a, const BHVec3 *b, BHVec3 *out);
int __cdecl bh_vec3_normalize(BHVec3 *v);
int __cdecl bh_vec3_normalize_to(const BHVec3 *v, BHVec3 *out);
void __cdecl bh_mat34_transform_point(const BHMat34 *m, const BHVec3 *v, BHVec3 *out);
void __cdecl bh_mat34_transform_direction(const BHMat34 *m, const BHVec3 *v, BHVec3 *out);
void __cdecl bh_mat34_inverse_transform_point(const BHMat34 *m, const BHVec3 *v, BHVec3 *out);
void __cdecl bh_mat34_translate_world(float x, float y, float z, BHMat34 *m);
void __cdecl bh_mat34_translate_local(float x, float y, float z, BHMat34 *m);
void __cdecl bh_mat34_inverse_rigid(const BHMat34 *m, BHMat34 *out);

#endif
