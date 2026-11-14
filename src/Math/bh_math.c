#include "bh_math.h"
#include <math.h>
#include <stddef.h>

static_assert(sizeof(BHVec3) == 12, "BHVec3 size");
static_assert(sizeof(BHMat34) == 48, "BHMat34 size");
static_assert(offsetof(BHVec3, z) == 8, "BHVec3.z offset");

/* 00401000. Distinct output required. Preserve each output's original
   x87 sum grouping; a uniform dot-product loop changes cancellation. */
void bh_mat34_multiply(const BHMat34 *left, const BHMat34 *right, BHMat34 *out)
{
    out->m[0][0] = (left->m[0][2] * right->m[2][0] + left->m[0][1] * right->m[1][0]) + left->m[0][0] * right->m[0][0];
    out->m[0][1] = (left->m[0][0] * right->m[0][1] + left->m[0][2] * right->m[2][1]) + left->m[0][1] * right->m[1][1];
    out->m[0][2] = (left->m[0][0] * right->m[0][2] + left->m[0][1] * right->m[1][2]) + left->m[0][2] * right->m[2][2];
    out->m[0][3] = ((left->m[0][0] * right->m[0][3] + left->m[0][1] * right->m[1][3]) + left->m[0][2] * right->m[2][3]) + left->m[0][3];
    out->m[1][0] = (left->m[1][0] * right->m[0][0] + left->m[1][2] * right->m[2][0]) + left->m[1][1] * right->m[1][0];
    out->m[1][1] = (left->m[1][0] * right->m[0][1] + left->m[1][2] * right->m[2][1]) + left->m[1][1] * right->m[1][1];
    out->m[1][2] = (left->m[1][2] * right->m[2][2] + left->m[1][1] * right->m[1][2]) + left->m[1][0] * right->m[0][2];
    out->m[1][3] = ((left->m[1][0] * right->m[0][3] + left->m[1][2] * right->m[2][3]) + left->m[1][1] * right->m[1][3]) + left->m[1][3];
    out->m[2][0] = (left->m[2][1] * right->m[1][0] + left->m[2][2] * right->m[2][0]) + left->m[2][0] * right->m[0][0];
    out->m[2][1] = (left->m[2][0] * right->m[0][1] + left->m[2][1] * right->m[1][1]) + left->m[2][2] * right->m[2][1];
    out->m[2][2] = (left->m[2][1] * right->m[1][2] + left->m[2][0] * right->m[0][2]) + left->m[2][2] * right->m[2][2];
    out->m[2][3] = ((left->m[2][0] * right->m[0][3] + left->m[2][2] * right->m[2][3]) + left->m[2][1] * right->m[1][3]) + left->m[2][3];
}

/* 004011a0. Preserve sequential writes; this is not an alias-safe cross product. */
void bh_vec3_cross(const BHVec3 *left, const BHVec3 *right, BHVec3 *out)
{
    out->x = left->y * right->z - right->y * left->z;
    out->y = left->z * right->x - right->z * left->x;
    out->z = right->y * left->x - left->y * right->x;
}

/* 004011f0. The assembly retains the length and reciprocal on the x87 stack. */
int bh_vec3_normalize(BHVec3 *vector)
{
    double length = sqrt(vector->x * vector->x + vector->y * vector->y + vector->z * vector->z);
    if (length == 0.0)
        return 0;
    length = 1.0 / length;
    vector->x = (float)(length * vector->x);
    vector->y = (float)(length * vector->y);
    vector->z = (float)(length * vector->z);
    return 1;
}

/* 00401270. Zero input leaves the output untouched. */
int bh_vec3_normalize_to(const BHVec3 *vector, BHVec3 *out)
{
    double length = sqrt(vector->x * vector->x + vector->y * vector->y + vector->z * vector->z);
    if (length == 0.0)
        return 0;
    length = 1.0 / length;
    out->x = (float)(length * vector->x);
    out->y = (float)(length * vector->y);
    out->z = (float)(length * vector->z);
    return 1;
}

/* 004012e0. Sum y/z products before adding x, as in the x87 body. */
void bh_mat34_transform_point(const BHMat34 *matrix, const BHVec3 *vector, BHVec3 *out)
{
    out->x = (vector->y * matrix->m[0][1] + vector->z * matrix->m[0][2]) + vector->x * matrix->m[0][0] + matrix->m[0][3];
    out->y = (vector->y * matrix->m[1][1] + vector->z * matrix->m[1][2]) + vector->x * matrix->m[1][0] + matrix->m[1][3];
    out->z = (vector->y * matrix->m[2][1] + vector->z * matrix->m[2][2]) + vector->x * matrix->m[2][0] + matrix->m[2][3];
}

/* 00401360. */
void bh_mat34_transform_direction(const BHMat34 *matrix, const BHVec3 *vector, BHVec3 *out)
{
    out->x = (vector->z * matrix->m[0][2] + vector->y * matrix->m[0][1]) + vector->x * matrix->m[0][0];
    out->y = (vector->z * matrix->m[1][2] + vector->y * matrix->m[1][1]) + vector->x * matrix->m[1][0];
    out->z = (vector->z * matrix->m[2][2] + vector->y * matrix->m[2][1]) + vector->x * matrix->m[2][0];
}

/* 004013d0. R-transpose * (v-t); inverse only for an orthonormal basis. */
void bh_mat34_inverse_transform_point(const BHMat34 *matrix, const BHVec3 *vector, BHVec3 *out)
{
    float x = vector->x - matrix->m[0][3], y = vector->y - matrix->m[1][3], z = vector->z - matrix->m[2][3];
    out->x = x * matrix->m[0][0] + matrix->m[2][0] * z + matrix->m[1][0] * y;
    out->y = matrix->m[2][1] * z + matrix->m[1][1] * y + matrix->m[0][1] * x;
    out->z = matrix->m[2][2] * z + matrix->m[1][2] * y + matrix->m[0][2] * x;
}

/* 00401460. */
void bh_mat34_translate_world(float x, float y, float z, BHMat34 *matrix)
{
    matrix->m[0][3] = x + matrix->m[0][3];
    matrix->m[1][3] = y + matrix->m[1][3];
    matrix->m[2][3] = z + matrix->m[2][3];
}

/* 00401490. */
void bh_mat34_translate_local(float x, float y, float z, BHMat34 *matrix)
{
    matrix->m[0][3] = (matrix->m[0][2] * z + matrix->m[0][1] * y) + x * matrix->m[0][0] + matrix->m[0][3];
    matrix->m[1][3] = (matrix->m[1][0] * x + matrix->m[1][2] * z) + matrix->m[1][1] * y + matrix->m[1][3];
    matrix->m[2][3] = (matrix->m[2][0] * x + matrix->m[2][2] * z) + matrix->m[2][1] * y + matrix->m[2][3];
}

/* 00401a70. Not a general matrix inverse; output must be distinct. */
void bh_mat34_inverse_rigid(const BHMat34 *matrix, BHMat34 *out)
{
    int r, c;
    for (r = 0; r < 3; r++)
        for (c = 0; c < 3; c++)
            out->m[r][c] = matrix->m[c][r];
    for (r = 0; r < 3; r++)
        out->m[r][3] = -matrix->m[0][3] * out->m[r][0] - out->m[r][1] * matrix->m[1][3] - matrix->m[2][3] * out->m[r][2];
}
