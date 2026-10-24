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

void __cdecl bh_mat34_rotate_x(float radians,BHMat34 *m) {
    float cosine,sine;double y,z;unsigned long row;
    bh_round_float(&cosine,cos((double)radians));
    bh_round_float(&sine,sin((double)radians));
    for(row=0;row<3;++row) {
        y=m->m[row][1];z=m->m[row][2];
        bh_round_float(&m->m[row][1],y*cosine+z*sine);
        bh_round_float(&m->m[row][2],z*cosine-y*sine);
    }
}
void __cdecl bh_mat34_rotate_z(float radians,BHMat34 *m) {
    float cosine,sine;double x,y;unsigned long row;
    bh_round_float(&cosine,cos((double)radians));
    bh_round_float(&sine,sin((double)radians));
    for(row=0;row<3;++row) {
        x=m->m[row][0];y=m->m[row][1];
        bh_round_float(&m->m[row][0],x*cosine+y*sine);
        bh_round_float(&m->m[row][1],y*cosine-x*sine);
    }
}

void __cdecl bh_mat34_rotate_world_x(float radians,BHMat34 *m) {
    float cosine,sine;double y,z;unsigned long column;
    bh_round_float(&cosine,cos((double)radians));
    bh_round_float(&sine,sin((double)radians));
    for(column=0;column<4;++column) {
        y=m->m[1][column];z=m->m[2][column];
        bh_round_float(&m->m[1][column],y*cosine-z*sine);
        bh_round_float(&m->m[2][column],y*sine+z*cosine);
    }
}
void __cdecl bh_mat34_rotate_world_y(float radians,BHMat34 *m) {
    float cosine,sine;double x,z;unsigned long column;
    bh_round_float(&cosine,cos((double)radians));
    bh_round_float(&sine,sin((double)radians));
    for(column=0;column<4;++column) {
        x=m->m[0][column];z=m->m[2][column];
        bh_round_float(&m->m[0][column],x*cosine+z*sine);
        bh_round_float(&m->m[2][column],z*cosine-x*sine);
    }
}
