#include "../src/bh_entity_collision.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL entity collision %d: %s\n",__LINE__,label);}} while(0)
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
static void cases(void) {
    unsigned char bytes[2048];unsigned long size,i;
    BHCollisionModel a,b;BHSweepQuery sweep;
    BHGeometryModel model;BHWorldGeometry world[3];
    BHCollisionCandidate c[3],saved_candidates[3];
    BHEntityPool pool;BHEntity *actor;
    BHMat34 pose,start,actor_world,actor_local;
    BHVec3 delta={-2.125f,0,0},angles={0,0,0},moving={0,0,0},saved_delta,saved_angles;
    BHSweepHit hit,saved;
    BHWorldQuery broad;BHWorldGeometry *owner;
    BHVec3 point={3,0,0};
    float total,low,high,mid,distance;
    int iteration;
    CHECK(bh_collision_model_init(&a,NULL) && bh_collision_model_init(&b,NULL),"real collision owners");
    size=fixture(bytes,0);CHECK(bh_collision_model_decode(&a,bytes,size),"source decoded");
    size=fixture(bytes,1);CHECK(bh_collision_model_decode(&b,bytes,size),"destination decoded");
    CHECK(bh_sweep_init(&sweep,&a.geometry,&b.geometry),"real borrowed swept dependency");
    CHECK(bh_geometry_model_init(&model,&b.geometry,1,0),"real model metadata owner");
    memset(world,0,sizeof(world));
    for(i=0;i<3;++i) {
        world[i].model=&model;identity(&world[i].transform);
        world[i].query_flags=2;
        world[i].transform.m[0][3]=i==0 ? -10 : (float)i-1;
        world[i].navigation.next=&world[(i+1)%3].navigation;
        world[i].minimum.x=world[i].minimum.y=world[i].minimum.z=-20;
        world[i].maximum.x=world[i].maximum.y=world[i].maximum.z=20;
        CHECK(bh_collision_candidate_world(c+i,world+i,&sweep),"typed static candidate over real geometry");
    }
    CHECK(bh_entities_init(&pool,2,NULL) && (actor=bh_entity_acquire(&pool))!=NULL,"real source actor pool");
    identity(&pose);pose.m[0][3]=3;start=pose;memset(&hit,0,sizeof(hit));saved=hit;
    CHECK(bh_entity_collision_dispatch(NULL,0,&pose,&delta,&angles,0,&hit)==0 &&
        pose.m[0][3]==.875f && !memcmp(&hit,&saved,sizeof(hit)),"empty list applies full movement without overwriting hit fields");
    pose=start;saved_delta=delta;saved_angles=angles;
    CHECK(bh_entity_collision_dispatch(c+1,1,&pose,&delta,&angles,0,&hit)==1 &&
        hit.fraction==.875f && pose.m[0][3]==1.140625f &&
        !memcmp(&delta,&saved_delta,sizeof(delta)) && !memcmp(&angles,&saved_angles,sizeof(angles)),
        "one static target uses swept source and retains request vectors");
    pose=start;delta.x=-2.125f;total=1;distance=delta.x;
    for(i=1;i<3;++i) {
        low=0;high=1;
        for(iteration=0;iteration<3;++iteration) {
            mid=(low+high)*.5f;
            if(3+distance*mid < world[i].transform.m[0][3]+1) high=mid;else low=mid;
        }
        distance*=low;total*=low;
    }
    CHECK(bh_entity_collision_dispatch(c,3,&pose,&delta,&angles,0,&hit)==1 &&
        hit.fraction==total && delta.x==distance && pose.m[0][3]==3+distance &&
        c[0].object.object.geometry==world+2 && c[2].object.object.geometry==world,
        "independent two-wall bracket product, scaled request and last-hit swap");
    identity(&actor_world);identity(&actor_local);
    CHECK(bh_collision_candidate_actor(c,actor,&sweep,&actor_world,&actor_local,&moving,0),"explicit real actor registry inputs");
    pose=start;delta.x=-2.125f;
    CHECK(bh_entity_collision_dispatch(c,1,&pose,&delta,&angles,0,&hit)==1 &&
        pose.m[0][3]==1.140625f,"physical actor collision ignores world-only enable flag");
    c[0].flags=2;pose=start;delta.x=-2.125f;
    CHECK(bh_entity_collision_dispatch(c,1,&pose,&delta,&angles,0,&hit)==1 &&
        pose.m[0][3]==1.140625f && hit.motion.x==-2.125f,"stationary enabled actor uses supplied transforms");
    c[0].movement.x=.125f;pose=start;
    CHECK(bh_entity_collision_dispatch(c,1,&pose,&delta,&angles,0,&hit)==1 &&
        pose.m[0][3]==1.140625f && hit.fraction==.875f && hit.motion.x==-2.125f &&
        delta.x==-2.125f,"physical actor velocity is not world per-tick translation");
    c[0].world.m[0][3]=-10;c[0].local.m[0][3]=10;c[0].movement.x=.25f;
    pose=start;delta.x=1;
    CHECK(bh_entity_collision_dispatch(c,1,&pose,&delta,&angles,0,&hit)==0 &&
        pose.m[0][3]==4,"physical actor miss applies requested movement once");
    CHECK(bh_collision_candidate_world(c+1,world,&sweep),"far static candidate");
    pose=start;
    CHECK(bh_entity_collision_dispatch(c,2,&pose,&delta,&angles,0,&hit)==0 && pose.m[0][3]==4,
        "multiple misses return final candidate pose");
    c[1]=c[0];c[1].flags=0;bh_collision_candidate_world(c,world,&sweep);pose=start;
    CHECK(bh_entity_collision_dispatch(c,2,&pose,&delta,&angles,0,&hit)==0 && pose.m[0][3]==4,
        "last physical actor does not use world-only collision flag");
    bh_collision_candidate_world(c,world+1,&sweep);c[0].flags=0;pose=start;delta.x=-2.125f;saved=hit;
    CHECK(bh_entity_collision_dispatch(c,1,&pose,&delta,&angles,0,&hit)==0 &&
        !memcmp(&pose,&start,sizeof(pose))&&!memcmp(&hit,&saved,sizeof(hit)),"disabled world retains original untouched pose and hit");
    c[0].flags=2;c[0].movement.x=.125f;pose=start;
    CHECK(bh_entity_collision_dispatch(c,1,&pose,&delta,&angles,0,&hit)==1 &&
        pose.m[0][3]==1.15625f&&hit.motion.x==-3.375f,"moving world uses relative displacement and original1.5 contact scaling");
    bh_collision_candidate_world(c,world+1,&sweep);bh_collision_candidate_world(c+1,world+2,&sweep);
    pose=start;delta.x=-2.125f;angles.z=.125f;saved_delta=delta;saved_angles=angles;
    CHECK(bh_entity_collision_dispatch(c,2,&pose,&delta,&angles,0,&hit)==1 &&
        delta.x==saved_delta.x*hit.fraction && angles.z==saved_angles.z*hit.fraction,
        "both translation and rotation shrink by accumulated contact fractions");
    saved=hit;pose=start;delta=saved_delta;angles=saved_angles;
    c[1].object.kind=BH_WORLD_HIT_END;memcpy(saved_candidates,c,sizeof(saved_candidates));
    CHECK(bh_entity_collision_dispatch(c,2,&pose,&delta,&angles,0,&hit)==-1 &&
        !memcmp(&pose,&start,sizeof(pose)) && !memcmp(&delta,&saved_delta,sizeof(delta)) &&
        !memcmp(&angles,&saved_angles,sizeof(angles)) && !memcmp(&hit,&saved,sizeof(hit)) &&
        !memcmp(c,saved_candidates,sizeof(saved_candidates)),"invalid later entry rolls back every caller output and ordering");
    /* Actual source broad phase emits the identity consumed by static binding. */
    world[1].navigation.next=&world[1].navigation;
    CHECK(bh_world_query_init(&broad,world+1,1,NULL,0,NULL),"source broad phase owner");
    CHECK(bh_world_query_run(&broad,NULL,world+1,&point,4,&owner) && broad.hit_count==1 &&
        broad.hits[0].kind==BH_WORLD_HIT_GEOMETRY &&
        bh_collision_candidate_world(c,broad.hits[0].object.geometry,&sweep),"real broad phase to narrow phase binding");
    pose=start;delta=saved_delta;angles.x=angles.y=angles.z=0;
    CHECK(bh_entity_collision_dispatch(c,1,&pose,&delta,&angles,0,&hit)==1 && hit.fraction==.875f,
        "source broad-phase candidate executes real swept dispatch");
    CHECK(bh_world_query_dispose(&broad),"broad-phase lifetime");
    CHECK(bh_entity_release(&pool,actor) && bh_entities_dispose(&pool),"real actor cleanup");
    CHECK(bh_sweep_dispose(&sweep) && bh_collision_model_dispose(&a) && bh_collision_model_dispose(&b),"all borrowed geometry/query owners disposed");
}
int bh_entity_collision_checkpoint(unsigned long *count) {
    checks=0;failed=0;cases();*count=checks;
    printf("{\"scenario\":\"entity-collision\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
