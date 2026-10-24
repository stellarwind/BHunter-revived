#include "bh_rotation.h"
#include "bh_round.h"
#include <math.h>

void __cdecl bh_mat34_rotate_y(float radians,BHMat34 *m) {
    float cosine,sine;double x,z;unsigned long row;
    bh_round_float(&cosine,cos((double)radians));
    bh_round_float(&sine,sin((double)radians));
    for(row=0;row<3;++row) {
        x=m->m[row][0];z=m->m[row][2];
        m->m[row][0]=(float)(x*cosine-z*sine);
        m->m[row][2]=(float)(x*sine+z*cosine);
    }
}
