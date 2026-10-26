#include "bh_world_edit.h"
#include "bh_round.h"
#include <stddef.h>
typedef char editor_geometry_first[offsetof(BHWorldEditPlacement,geometry)==0?1:-1];
static long add(long a,long b){return (long)((unsigned long)a+(unsigned long)b);}
static long sub(long a,long b){return (long)((unsigned long)a-(unsigned long)b);}
int __cdecl bh_world_edit_rebuild(BHWorldGeometry *p,unsigned long count){
    if(!bh_world_geometry_build(p,count))return 0;
    return bh_portals_match(&p->navigation,count);
}
static int snap(BHWorldEditPlacement *p,unsigned long count){
    unsigned long i;long grid;
    if(!p->geometry.model||(grid=p->geometry.model->grid)<=0)return 0;
    for(i=0;i<3;i++)p->coordinates[i]=(p->coordinates[i]/grid)*grid;
    for(i=0;i<3;i++)bh_round_float((&p->geometry.position.x)+i,(double)((p->coordinates[i]/grid)*grid));
    return bh_world_edit_rebuild(&p->geometry,count);
}
int __cdecl bh_world_edit_move(BHWorldEditPlacement *p,unsigned long count,long x,long y,long z,int single){
    unsigned long steps=0;
    if(!p||!count||count>65535)return 0;
    do{
        if(!p->geometry.model||p->geometry.model->grid<=0||++steps>count)return 0;
        p->coordinates[0]=add(p->coordinates[0],x);p->coordinates[1]=add(p->coordinates[1],y);p->coordinates[2]=add(p->coordinates[2],z);
        if(!snap(p,count))return 0;if(single)return 1;
        p=(BHWorldEditPlacement*)p->geometry.navigation.next;if(!p)return 0;
    }while(p->geometry.owner);
    return 1;
}
int __cdecl bh_world_edit_rotate(BHWorldEditPlacement *p,unsigned long count,long turn,long pivot_x,long pivot_z){
    long x,z;unsigned long steps=0;
    if(!p||!count||count>65535)return 0;
    while(p->geometry.owner){
        if(!p->geometry.model||p->geometry.model->grid<=0||++steps>count)return 0;
        p->geometry.flags=((unsigned long)turn+p->geometry.flags)&3;
        p->angles.y=p->angles.z=0;bh_round_float(&p->angles.x,(double)p->geometry.flags*1.5707963267948966);
        x=sub(p->coordinates[0],pivot_x);z=sub(p->coordinates[2],pivot_z);
        if(turn<0)z=sub(0,z);else x=sub(0,x);
        p->coordinates[0]=add(z,pivot_x);p->coordinates[2]=add(x,pivot_z);
        if(!snap(p,count))return 0;p=(BHWorldEditPlacement*)p->geometry.navigation.next;if(!p)return 0;
    }
    return 1;
}
