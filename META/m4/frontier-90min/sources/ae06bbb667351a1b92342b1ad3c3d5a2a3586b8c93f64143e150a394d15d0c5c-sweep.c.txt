#include "../src/bh_sweep.h"
#include "../src/bh_rotation.h"
#include "saved_contact.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;
static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL sweep %d: %s\n",__LINE__,label);}} while(0)
static void section(unsigned char *data,unsigned long *at,unsigned long count,const void *p,unsigned long size) {
    memcpy(data+*at,&count,4);*at+=4;
    memcpy(data+*at,p,count*size);*at+=count*size;
}
static unsigned long fixture(unsigned char *data,int destination) {
    BHBspMaterial material;
    BHBspVertex vertices[4];BHBspEdge edges[4];BHBspFace faces[3];
    BHBspNode nodes[3];BHBspLeaf leaves[3];
    unsigned long at=0,i;
    memset(&material,0,sizeof(material));strcpy(material.texture,"STONE.PCX");
    memset(vertices,0,sizeof(vertices));memset(edges,0,sizeof(edges));
    memset(faces,0,sizeof(faces));memset(nodes,0,sizeof(nodes));
    for(i=0;i<4;++i) {
        vertices[i].y=(i==0 || i==3) ? -2 : 2;
        if(destination) vertices[i].z=i<2 ? -1 : 1;
        else vertices[i].x=i<2 ? -1 : 1;
        edges[i].vertex=i;edges[i].next=i==3 ? -1 : i+1;
    }
    for(i=0;i<3;++i) {
        faces[i].first_edge=0;
        nodes[i].minimum[0]=destination ? 0 : -1;
        nodes[i].maximum[0]=destination ? 0 : 1;
        nodes[i].minimum[1]=-2;nodes[i].maximum[1]=2;
        nodes[i].minimum[2]=destination ? -1 : 0;
        nodes[i].maximum[2]=destination ? 1 : 0;
        nodes[i].normal[destination ? 0 : 2]=1;
        nodes[i].front_leaf=i;nodes[i].back_leaf=-1;
        nodes[i].front_node=nodes[i].back_node=-1;
        leaves[i].face=i;leaves[i].next=-1;
    }
    section(data,&at,1,&material,sizeof(material));
    section(data,&at,4,vertices,sizeof(vertices[0]));
    section(data,&at,4,edges,sizeof(edges[0]));
    section(data,&at,3,faces,sizeof(faces[0]));
    section(data,&at,3,nodes,sizeof(nodes[0]));
    section(data,&at,3,leaves,sizeof(leaves[0]));
    return at;
}
static void identity(BHMat34 *m) {memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
static void rotations(void) {
    BHMat34 m,saved;
    unsigned long axis,row,col;
    float half_pi=1.5707963267948966f;
    for(axis=0;axis<3;++axis) {
        identity(&m);m.m[0][3]=10;m.m[1][3]=20;m.m[2][3]=30;saved=m;
        if(axis==0) bh_mat34_rotate_x(half_pi,&m);
        if(axis==1) bh_mat34_rotate_y(half_pi,&m);
        if(axis==2) bh_mat34_rotate_z(half_pi,&m);
        CHECK(m.m[0][3]==10 && m.m[1][3]==20 && m.m[2][3]==30,"all local rotations retain translation");
        if(axis==0) CHECK(fabs(m.m[1][2]+1)<1e-6 && fabs(m.m[2][1]-1)<1e-6,"X positive quarter turn signs");
        if(axis==1) CHECK(fabs(m.m[0][2]-1)<1e-6 && fabs(m.m[2][0]+1)<1e-6,"Y positive quarter turn signs");
        if(axis==2) CHECK(fabs(m.m[0][1]+1)<1e-6 && fabs(m.m[1][0]-1)<1e-6,"Z positive quarter turn signs");
        if(axis==0) bh_mat34_rotate_x(-half_pi,&m);
        if(axis==1) bh_mat34_rotate_y(-half_pi,&m);
        if(axis==2) bh_mat34_rotate_z(-half_pi,&m);
        for(row=0;row<3;++row)for(col=0;col<4;++col)
            CHECK(fabs(m.m[row][col]-saved.m[row][col])<1e-6,"inverse quarter turn restores each coefficient");
    }
}
static void cases(void) {
    unsigned char bytes[2048];unsigned long size,i;
    BHCollisionModel a,b;BHSweepQuery q,empty,destination_only;
    BHMat34 pose,start,world,local,expected;
    BHVec3 movement={-2.125f,0,0},angles={0,0,0};
    BHSweepHit hit,saved;
    float low,high,mid,expected_fraction;
    int iteration,case_index,expected_hit;
    identity(&world);identity(&local);
    CHECK(bh_collision_model_init(&a,NULL) && bh_collision_model_init(&b,NULL),"real model lifecycle");
    size=fixture(bytes,0);CHECK(bh_collision_model_decode(&a,bytes,size),"source model decoded");
    size=fixture(bytes,1);CHECK(bh_collision_model_decode(&b,bytes,size),"destination model decoded");
    CHECK(bh_sweep_init(&q,&a.geometry,&b.geometry),"mesh plus fallback workspaces");
    memset(&hit,0,sizeof(hit));identity(&pose);pose.m[0][3]=3;
    CHECK(bh_sweep_run(&q,&pose,&movement,&angles,&world,&local,1,&hit)==1 &&
        hit.fraction==SAVED_CONTACT_FRACTION && hit.surface_present &&
        fabs(hit.surface.normal.x-saved_contact_normal.x)<1e-6 &&
        fabs(hit.surface.normal.y-saved_contact_normal.y)<1e-6 &&
        fabs(hit.surface.normal.z-saved_contact_normal.z)<1e-6 &&
        pose.m[0][3]==1.140625f && hit.motion.x==movement.x,
        "recovered swept detector produces saved M3 fraction/normal on explicit analytical geometry");
    /* Independent geometric predicate: source interval (center-1,center+1)
       straddles x=0. Work out the original fixed three-step bracket on paper. */
    for(case_index=0;case_index<12;++case_index) {
        identity(&pose);pose.m[0][3]=3;movement.x=-2.125f-case_index*.125f;
        low=0;high=1;
        for(iteration=0;iteration<3;++iteration) {
            mid=(low+high)*.5f;
            if(3+movement.x*mid<1) high=mid;else low=mid;
        }
        expected_fraction=low;
        CHECK(bh_sweep_run(&q,&pose,&movement,&angles,&world,&local,0,&hit)==1 &&
            hit.fraction==expected_fraction && pose.m[0][3]==3+movement.x*expected_fraction &&
            hit.motion.x==movement.x && hit.motion.y==0 && hit.motion.z==0,
            "analytical plane/interval bracket and contact displacement");
    }
    /* Rotating source half extent against an analytical plane: each midpoint
       rotates by the scaled requested angle, while translation stays world-space. */
    for(case_index=1;case_index<=8;++case_index) {
        identity(&pose);pose.m[0][3]=3;movement.x=-2.125f;
        angles.z=case_index*.0625f;low=0;high=1;
        for(iteration=0;iteration<3;++iteration) {
            double theta;
            mid=(low+high)*.5f;theta=angles.z*mid;
            if(3+movement.x*mid < cos(theta)+2*sin(theta)) high=mid;else low=mid;
        }
        CHECK(bh_sweep_run(&q,&pose,&movement,&angles,&world,&local,0,&hit)==1 &&
            hit.fraction==low && pose.m[0][3]==3+movement.x*low &&
            fabs(pose.m[0][0]-cos(angles.z*low))<1e-6 &&
            fabs(pose.m[0][1]+sin(angles.z*low))<1e-6,
            "rotating mesh sweep follows independently derived plane extent bracket");
    }
    angles.z=0;
    identity(&pose);pose.m[0][3]=3;movement.x=-6;
    CHECK(bh_sweep_run(&q,&pose,&movement,&angles,&world,&local,0,&hit)==0 &&
        hit.fraction==1 && pose.m[0][3]==-3 && !hit.surface_present,
        "endpoint misses retain original tunneling behavior when segment fallback disabled");
    identity(&pose);pose.m[0][3]=3;
    CHECK(bh_sweep_run(&q,&pose,&movement,&angles,&world,&local,1,&hit)==1 &&
        hit.fraction==.3125f && pose.m[0][3]==1.125f && hit.surface_present,
        "segment crossing scales one mesh refinement and multiplies fractions");
    CHECK(bh_sweep_init(&destination_only,NULL,&b.geometry),"null source still owns real destination segment workspace");
    identity(&pose);pose.m[0][3]=3;memset(&hit,0x36,sizeof(hit));saved=hit;
    CHECK(bh_sweep_run(&destination_only,&pose,&movement,&angles,&world,&local,1,&hit)==1 &&
        hit.fraction==.5f && pose.m[0][3]==0 && !hit.surface_present &&
        !memcmp(&hit.surface,&saved.surface,sizeof(hit.surface)) &&
        !memcmp(&hit.motion,&saved.motion,sizeof(hit.motion)),
        "destination-only fallback returns fraction without inventing mesh fields");
    CHECK(bh_sweep_dispose(&destination_only),"optional source query disposed");
    CHECK(bh_sweep_init(&empty,NULL,NULL),"original absent-model path");
    identity(&pose);movement.x=1;movement.y=2;movement.z=3;
    angles.x=1.5707963267948966f;angles.y=1.5707963267948966f;angles.z=1.5707963267948966f;
    identity(&expected);expected.m[0][3]=1;expected.m[1][3]=2;expected.m[2][3]=3;
    /* Rx * Ry * Rz for three positive quarter turns. */
    expected.m[0][0]=0;expected.m[0][1]=0;expected.m[0][2]=1;
    expected.m[1][0]=0;expected.m[1][1]=-1;expected.m[1][2]=0;
    expected.m[2][0]=1;expected.m[2][1]=0;expected.m[2][2]=0;
    CHECK(bh_sweep_run(&empty,&pose,&movement,&angles,&world,&local,1,&hit)==0 &&
        hit.fraction==1 && !hit.surface_present,"absent models still commit free motion");
    for(i=0;i<12;++i) CHECK(fabs(((float *)&pose)[i]-((float *)&expected)[i])<1e-6,
        "translation then X/Y/Z composition in recovered order");
    CHECK(bh_sweep_dispose(&empty),"empty query lifetime");
    angles.x=angles.y=angles.z=0;movement.x=-2.125f;movement.y=movement.z=0;
    identity(&pose);pose.m[0][3]=3;start=pose;saved=hit;
    ((BHBspNode *)b.bsp.nodes)[0].front_leaf=99;
    CHECK(bh_sweep_run(&q,&pose,&movement,&angles,&world,&local,1,&hit)==-1 &&
        !memcmp(&pose,&start,sizeof(pose)) && !memcmp(&hit,&saved,sizeof(hit)) && !q.busy,
        "malformed topology rolls back pose/output and releases busy state");
    ((BHBspNode *)b.bsp.nodes)[0].front_leaf=0;
    CHECK(bh_sweep_run(&q,&pose,&movement,&angles,&world,&local,1,&hit)==1,
        "swept owner reusable after failed attempt");
    CHECK(bh_sweep_dispose(&q),"all sweep workspaces released");
    CHECK(bh_collision_model_dispose(&a) && bh_collision_model_dispose(&b),"borrowed model owners disposed last");
}
int bh_sweep_real_check(BHGeometry *geometries,unsigned long count) {
    unsigned long i;
    BHSweepQuery query;
    BHSweepHit first,second;
    BHMat34 world,local,a,b;
    BHVec3 movement,angles={0,0,.01f};
    int x,y;
    identity(&world);identity(&local);
    for(i=0;i<count;++i) {
        if(!geometries[i].bsp->counts[4]) continue;
        if(!bh_sweep_init(&query,geometries+i,geometries+i)) return 0;
        identity(&a);identity(&b);memset(&first,0,sizeof(first));memset(&second,0,sizeof(second));
        movement.x=geometries[i].bsp->bounds[0].radius*.125f;movement.y=movement.z=0;
        x=bh_sweep_run(&query,&a,&movement,&angles,&world,&local,0,&first);
        y=bh_sweep_run(&query,&b,&movement,&angles,&world,&local,0,&second);
        if(x<0 || x!=y || first.fraction!=second.fraction ||
            memcmp(&a,&b,sizeof(a)) || first.surface_present!=second.surface_present ||
            (first.surface_present && (memcmp(&first.surface.contact,&second.surface.contact,sizeof(first.surface.contact)) ||
             memcmp(&first.motion,&second.motion,sizeof(first.motion))))) {
            bh_sweep_dispose(&query);return 0;
        }
        if(!bh_sweep_dispose(&query)) return 0;
    }
    return 1;
}
int bh_sweep_checkpoint(unsigned long *count) {
    checks=0;failed=0;rotations();cases();*count=checks;
    printf("{\"scenario\":\"swept-collision\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
