#include "../src/bh_physics_response.h"
#include "../src/bh_rotation.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL physics response %d: %s\n",__LINE__,label);}} while(0)
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
static unsigned long bits(float value) {unsigned long word;memcpy(&word,&value,4);return word;}
static void numeric(void) {
    const float mass[4]={-1,0,1,3};
    const float first[16]={-2,0,0,0,-4,-2,-4,-4,-4,0,-2,-3,-4,0,-1,-2};
    const float second[16]={-2,-4,-4,-4,0,-2,0,0,0,-4,-2,-1,0,-4,-3,-2};
    BHPhysicsState state,saved;
    BHPhysicsResponse response,old_response;
    BHSweepHit hit;
    BHMat34 previous,m;
    BHVec3 movement,angles,slide,force,saved_force;
    float margin,old_margin;
    unsigned long a,b,i;
    memset(&state,0,sizeof(state));identity(&state.pose);state.last_clear=state.pose;
    state.velocity.x=2;state.acceleration.x=4;state.angular_velocity.z=1;state.angular_acceleration.z=2;
    CHECK(bh_physics_prepare(&state,.5f,2,&movement,&angles,&margin) &&
        state.velocity.x==4 && movement.x==2 && state.angular_velocity.z==2 && angles.z==1 &&
        state.acceleration.x==0 && state.angular_acceleration.z==0 && margin==4.5f,
        "integration, one-shot acceleration clearing and original radius+0.5 broad margin");
    /* Each source rotation coefficient is tested against its own angle sign. */
    saved=state;old_margin=margin;
    CHECK(!bh_physics_prepare(&state,0,2,&movement,&angles,&margin) &&
        !memcmp(&state,&saved,sizeof(state)) && margin==old_margin,"zero delta rejected without state changes");
    memset(&hit,0,sizeof(hit));hit.surface_present=1;hit.fraction=.25f;
    hit.surface.normal.x=1;hit.surface.contact.point.x=.05f;hit.motion.x=-4;
    movement.x=-4;movement.y=2;movement.z=0;
    CHECK(bh_physics_slide(&movement,&hit,.5f,&slide) && slide.x==0 && slide.y==.75f && slide.z==0,
        "tangent projection times friction and remaining movement fraction");
    for(a=0;a<4;++a)for(b=0;b<4;++b) {
        force.x=force.y=force.z=0;srand(1);
        CHECK(bh_physics_contact_response(&hit,.25f,.5f,mass[a],mass[b],&force,&response),"actual numerical contact response");
        CHECK(response.first.impulse==first[a*4+b] && response.second.impulse==second[a*4+b] &&
            response.emit_first==(first[a*4+b]!=0) && response.emit_second==(second[a*4+b]!=0),
            "independent 16-case signed/zero/positive mass split table");
        CHECK(response.first.event==2 && response.second.event==2 && response.first.point.x==.05f &&
            response.first.normal.x==1 && response.second.normal.x==-1 &&
            fabs(response.first.severity-.04)<1e-7 && response.second.severity==response.first.severity,
            "real event-2 packet fields and shared clamped severity");
        CHECK(response.speed==8 && fabs(force.x-30.01)<1e-5 && fabs(response.audio_level-.16)<1e-6 &&
            force.y==0 && force.z==0,"contact speed, normal acceleration and audio scalar contract");
        CHECK(rand()==26500,"exactly three real shared VC5 CRT random draws");
    }
    force.x=force.y=force.z=0;srand(1);bh_physics_contact_response(&hit,.25f,.5f,1,-1,&force,&response);
    CHECK(fabs(response.jittered_motion.x-(-4+(41-16384)*(float)(8*7.62939453125e-8)))<1e-6 &&
        fabs(response.jittered_motion.y-(18467-16384)*(float)(8*7.62939453125e-8))<1e-9,
        "independent known CRT sequence controls stored jitter components");
    for(i=2;i<=6;i+=2) {
        hit.surface.flags=i;force.x=force.y=force.z=0;srand(1);
        CHECK(bh_physics_contact_response(&hit,.25f,.5f,1,-1,&force,&response) &&
            response.first.severity==0 && response.audio_level==0 &&
            fabs(force.x-((i&2) ? 130.01 : 180.01))<1e-4 &&
            response.jittered_motion.x < -16,
            "material 2/4 priority, doubled effect family, zero severity and fourfold motion");
    }
    hit.surface.flags=8;force.x=force.y=force.z=0;
    CHECK(bh_physics_contact_response(&hit,0,.5f,1,-1,&force,&response) &&
        response.slide_fraction==.01f && fabs(response.first.impulse+4.3)<1e-6 &&
        fabs(response.first.severity-.00172)<1e-7,"material 8 signed bias and minimum slide fraction");
    hit.motion.x=4;force.x=force.y=force.z=0;
    CHECK(bh_physics_contact_response(&hit,.5f,.5f,1,-1,&force,&response) &&
        fabs(response.first.impulse-4.3)<1e-6,"positive material-8 impulse bias");
    hit.motion.x=-400;hit.surface.flags=0;
    CHECK(bh_physics_contact_response(&hit,1,.5f,1,-1,&force,&response) &&
        response.first.severity==1 && response.audio_level==1,"severity and sound scalar saturation");
    hit.motion.x=-4;hit.surface.flags=0;force.x=force.y=force.z=0;
    CHECK(bh_physics_contact_response(&hit,.25f,.5f,16777216.0f,1,&force,&response) &&
        bits(response.first.impulse)==0xb47fffffUL,
        "original mass denominator remains PC53 double before packet float store");
    old_response=response;saved_force=force;srand(1);
    CHECK(!bh_physics_contact_response(&hit,.5f,0,1,-1,&force,&response) &&
        !memcmp(&response,&old_response,sizeof(response)) && !memcmp(&force,&saved_force,sizeof(force)) &&
        rand()==41,"invalid response rejects before shared random consumption");
    identity(&previous);state.pose=previous;state.pose.m[0][3]=2;state.pose.m[1][3]=1;
    state.acceleration.x=state.acceleration.y=state.acceleration.z=0;
    state.angular_velocity.x=1;state.angular_velocity.y=2;state.angular_velocity.z=3;
    CHECK(bh_physics_finish(&state,&previous,.5f,1) && fabs(state.velocity.x-3.98)<1e-6 &&
        fabs(state.velocity.y-1.99)<1e-6 && fabs(state.angular_velocity.z-2.94)<1e-6 &&
        fabs(state.acceleration.y-9.82)<1e-6 && state.euler.x==0 && state.euler.y==0 &&
        state.euler.z==0 && !memcmp(&state.pose,&state.last_clear,sizeof(state.pose)),
        "finish reconstructs velocity, damps spin, queues positive-Y gravity and saves clear pose");
    state.pose=previous;state.pose.m[0][3]=.2f;
    CHECK(bh_physics_finish(&state,&previous,.3f,0) && bits(state.velocity.x)==0x3f29d036UL,
        "original reciprocal remains PC53 double through final velocity drag");
    saved=state;state.pose.m[0][3]=3;
    CHECK(bh_physics_finish(&state,&previous,.5f,0) && !memcmp(&state.last_clear,&saved.last_clear,sizeof(state.last_clear)),
        "collision finish retains last known clear pose");
    /* World rotations affect translation as well as all three basis columns. */
    identity(&m);m.m[1][3]=2;m.m[2][3]=3;
    bh_mat34_rotate_world_x(1.5707963267948966f,&m);
    CHECK(fabs(m.m[1][3]+3)<1e-6 && fabs(m.m[2][3]-2)<1e-6 &&
        fabs(m.m[1][2]+1)<1e-6 && fabs(m.m[2][1]-1)<1e-6,"world X rotates all four columns");
    identity(&m);m.m[0][3]=2;m.m[2][3]=3;
    bh_mat34_rotate_world_y(1.5707963267948966f,&m);
    CHECK(fabs(m.m[0][3]-3)<1e-6 && fabs(m.m[2][3]+2)<1e-6 &&
        fabs(m.m[0][2]-1)<1e-6 && fabs(m.m[2][0]+1)<1e-6,"world Y rotates all four columns");
    identity(&state.pose);bh_mat34_rotate_x(.25f,&state.pose);
    CHECK(bh_physics_finish(&state,&previous,.5f,0) && fabs(state.euler.y-.25)<1e-6 &&
        fabs(state.euler.x)<1e-6 && fabs(state.euler.z)<1e-6,"pitch Euler extraction through real world X rotation");
    identity(&state.pose);bh_mat34_rotate_z(.25f,&state.pose);
    CHECK(bh_physics_finish(&state,&previous,.5f,0) && fabs(state.euler.z-.25)<1e-6 &&
        fabs(state.euler.x)<1e-6 && fabs(state.euler.y)<1e-6,"roll Euler extraction after yaw/pitch removal");
    identity(&state.pose);bh_mat34_rotate_y(.25f,&state.pose);
    CHECK(bh_physics_finish(&state,&previous,.5f,0) && fabs(state.euler.x-.25)<1e-6 &&
        fabs(state.euler.y)<1e-6 && fabs(state.euler.z)<1e-6,"yaw Euler extraction through real world rotations");
}
static void linked_slice(void) {
    unsigned char bytes[2048];unsigned long size;
    BHCollisionModel a,b;BHSweepQuery sweep;
    BHGeometryModel model;BHWorldGeometry world;
    BHCollisionCandidate candidate;
    BHCollisionHit overlap;
    BHPhysicsState state;
    BHPhysicsResponse response;
    BHMat34 previous,identity_matrix;
    BHSweepHit first,second;
    BHVec3 delta,angles,slide,zero={0,0,0};
    float margin;
    CHECK(bh_collision_model_init(&a,NULL) && bh_collision_model_init(&b,NULL),"source model owners");
    size=fixture(bytes,0);CHECK(bh_collision_model_decode(&a,bytes,size),"real source BSP");
    size=fixture(bytes,1);CHECK(bh_collision_model_decode(&b,bytes,size),"real destination BSP");
    CHECK(bh_sweep_init(&sweep,&a.geometry,&b.geometry),"real collision workspace");
    identity(&identity_matrix);
    CHECK(bh_physics_overlap(&sweep,&identity_matrix,&identity_matrix,&identity_matrix,&overlap)==1 &&
        overlap.normal.x==1,"complete static overlap wrapper calls real mesh traversal");
    CHECK(bh_geometry_model_init(&model,&b.geometry,1,0),"real metadata definition");
    memset(&world,0,sizeof(world));world.model=&model;world.transform=identity_matrix;
    CHECK(bh_collision_candidate_world(&candidate,&world,&sweep),"real world candidate binding");
    memset(&state,0,sizeof(state));state.pose=identity_matrix;state.pose.m[0][3]=3;state.last_clear=state.pose;
    state.velocity.x=-4.25f;state.velocity.y=1;previous=state.pose;
    memset(&first,0,sizeof(first));memset(&second,0,sizeof(second));
    CHECK(bh_physics_prepare(&state,.5f,2,&delta,&angles,&margin),"linked source motion integration");
    CHECK(bh_entity_collision_dispatch(&candidate,1,&state.pose,&delta,&angles,0,&first)==1 &&
        first.fraction==.875f,"linked dispatch to recovered sweep");
    CHECK(bh_physics_slide(&delta,&first,.5f,&slide) && slide.x==0 && slide.y==.03125f,
        "source contact computes real tangent slide request");
    CHECK(bh_entity_collision_dispatch(&candidate,1,&state.pose,&slide,&zero,0,&second)==0 &&
        state.pose.m[1][3]==.46875f,"second real dispatch applies tangent movement");
    CHECK(bh_physics_contact_response(&first,second.fraction,.5f,1,-1,&state.acceleration,&response) &&
        response.emit_first && !response.emit_second && response.first.event==2,
        "actual contact response packet and mass partition produced from source sweep");
    CHECK(bh_physics_finish(&state,&previous,.5f,0) &&
        fabs(state.velocity.x-(-3.71875*.995))<1e-6 &&
        fabs(state.velocity.y-(.9375*.995))<1e-6,"finish uses actual accepted displacement");
    CHECK(bh_sweep_dispose(&sweep) && bh_collision_model_dispose(&a) && bh_collision_model_dispose(&b),"complete source ownership cleanup");
}
int bh_physics_response_checkpoint(unsigned long *count) {
    checks=0;failed=0;numeric();linked_slice();*count=checks;
    printf("{\"scenario\":\"physics-response\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
