#include "../src/bh_world_geometry.h"
#include "../src/bh_rotation.h"
#include "../src/bh_blocks.h"
#include "../src/bh_navigation.h"
#include "../src/bh_route_query.h"
#include "../src/bh_world_query.h"
int bh_position_real_check(const BHNavigation *navigation);
int bh_segment_real_check(BHGeometry *geometry,unsigned long count);
int bh_sweep_real_check(BHGeometry *geometries,unsigned long count);
int bh_collision_queries_real_check(BHGeometry *geometries,unsigned long count);
int bh_collision_models_real_check(const BHBlockLibrary *library);
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "world_geometry_cases.h"
#include "portal_cases.h"
#include "navigation_cases.h"
#include "route_query_cases.h"

static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<20)printf("FAIL world geometry line %d: %s\n",line,label);}}
static unsigned long crc(const void *bytes,unsigned long size) {
    const unsigned char *p=(const unsigned char*)bytes;unsigned long value=0xffffffffUL,i,j;
    for(i=0;i<size;++i){value^=p[i];for(j=0;j<8;++j)value=(value>>1)^((value&1)?0xedb88320UL:0);}
    return value^0xffffffffUL;
}
static void section(unsigned char *bytes,unsigned long *at,unsigned long count,const void *data,unsigned long size){memcpy(bytes+*at,&count,4);*at+=4;memcpy(bytes+*at,data,size);*at+=size;}
static int cube(BHBsp *b,int materials_mode) {
    unsigned char bytes[8192];unsigned long at=0,i,j;BHBspMaterial materials[6];BHBspVertex vertices[8];
    BHBspEdge edges[24];BHBspFace faces[6];BHBspNode nodes[6];BHBspLeaf leaves[6];
    static const long indices[6][4]={{1,3,7,5},{0,4,6,2},{2,6,7,3},{0,1,5,4},{4,5,7,6},{0,2,3,1}};
    static const char *names[]={"pOrTaL.gIf","ROOF.GIF","Water.gif","SCRAPR2.TGA","scrapr.tga","stone.pcx"};
    memset(materials,0,sizeof(materials));memset(vertices,0,sizeof(vertices));memset(edges,0,sizeof(edges));memset(faces,0,sizeof(faces));memset(nodes,0,sizeof(nodes));
    for(i=0;i<6;++i)strcpy(materials[i].texture,materials_mode==0?"PORTAL.GIF":materials_mode==1?names[i]:"stone.pcx");
    for(i=0;i<8;++i){vertices[i].x=(i&1)?10:-10;vertices[i].y=(i&2)?10:-10;vertices[i].z=(i&4)?10:-10;}
    for(i=0;i<6;++i) {
        faces[i].material=i;faces[i].first_edge=i*4;
        for(j=0;j<4;++j){edges[i*4+j].vertex=indices[i][j];edges[i*4+j].next=j==3?-1:i*4+j+1;}
        for(j=0;j<3;++j){nodes[i].minimum[j]=-10;nodes[i].maximum[j]=10;}
        nodes[i].normal[i/2]=(i&1)?-1:1;nodes[i].front_leaf=i;nodes[i].back_leaf=-1;nodes[i].front_node=nodes[i].back_node=-1;
        leaves[i].face=i;leaves[i].next=-1;
    }
    section(bytes,&at,6,materials,sizeof(materials));section(bytes,&at,8,vertices,sizeof(vertices));section(bytes,&at,24,edges,sizeof(edges));
    section(bytes,&at,6,faces,sizeof(faces));section(bytes,&at,6,nodes,sizeof(nodes));section(bytes,&at,6,leaves,sizeof(leaves));
    return bh_bsp_decode(b,bytes,at);
}
static void scene(BHWorldGeometry *p,unsigned long count,const BHGeometryModel *model) {
    unsigned long i;memset(p,0,count*sizeof(*p));
    for(i=0;i<count;++i){p[i].navigation.next=&p[(i+1)%count].navigation;p[i].previous=&p[(i+count-1)%count];p[i].model=model;}
}
static void basics(void) {
    BHBsp b,other;BHGeometry g,h;BHGeometryModel model,mixed,empty,saved_model;BHWorldGeometry p[3],saved;
    unsigned long i,j,flags,quarter;BHVec3 in={2,3,4},out,back;BHMat34 matrix;
    static const unsigned long directions[]={0,4,1,5,2,6};static const unsigned long expected_flags[]={1,2,4,8,8,0};
    bh_bsp_init(&b,NULL);bh_bsp_init(&other,NULL);
    CHECK(cube(&b,0) && bh_geometry_init(&g,&b,NULL) && bh_geometry_model_init(&model,&g,10,6),"real cube geometry/model ownership");
    CHECK(model.portal_count==6 && model.flags==6 && model.minimum.x==-10 && model.maximum.z==10,"model bounds and portal list");
    for(i=0;i<6;++i)CHECK(model.portal_faces[i]==(long)i && bh_geometry_get_flags(&g,i,&flags) && flags==1,"portal face order and flag owner");
    scene(p,3,&model);
    for(quarter=0;quarter<4;++quarter) {
        p[0].flags=quarter;p[0].position.x=30;p[0].position.y=-20;p[0].position.z=70;
        CHECK(bh_world_geometry_build(p,3) && !p[0].owner && p[0].navigation.count==6,"quarter-turn world construction");
        CHECK(fabs(p[0].minimum.x-20)<0.0001 && fabs(p[0].maximum.z-80)<0.0001,"transformed face-derived bounds");
        bh_mat34_transform_point(&p[0].transform,&in,&out);bh_mat34_transform_point(&p[0].inverse,&out,&back);
        CHECK(fabs(back.x-in.x)<0.0001 && fabs(back.y-in.y)<0.0001 && fabs(back.z-in.z)<0.0001,"forward/inverse placement transforms compose");
        for(i=0;i<6;++i)CHECK(!p[0].navigation.portals[i].neighbor && !memcmp(p[0].navigation.portals[i].aperture,p[0].navigation.portals[i].overlap,16) && !memcmp(p[0].navigation.portals[i].overlap,p[0].navigation.portals[i].scratch,16),"construction initializes overlap/scratch and clears links");
    }
    p[0].flags=0;p[0].position.x=p[0].position.y=p[0].position.z=0;CHECK(bh_world_geometry_build(p,3),"axis cube construction");
    for(i=0;i<6;++i)CHECK(p[0].navigation.portals[i].direction==directions[i] && p[0].navigation.portals[i].plane==((i&1)?-10:10),"all six axis directions and integer planes");
    p[0].position.x=5;CHECK(bh_world_geometry_build(p,3) && p[0].navigation.portals[0].plane==20 && p[0].navigation.portals[1].plane==-10,"positive and negative half-grid ties away from zero");
    p[0].position.x=-5;CHECK(bh_world_geometry_build(p,3) && p[0].navigation.portals[0].plane==10 && p[0].navigation.portals[1].plane==-20,"opposite half-grid ties");
    CHECK(cube(&other,2) && bh_geometry_init(&h,&other,NULL) && bh_geometry_model_init(&empty,&h,10,6),"nonportal real geometry/model");
    p[1].model=p[2].model=&empty;p[0].position.x=0;
    CHECK(bh_world_geometry_build(p+2,3) && p[2].owner==p,"owner follows previous scene links past nonportal models");
    p[0].model=&empty;CHECK(bh_world_geometry_build(p+1,3) && !p[1].owner,"all nonportal scene has no owner");
    p[0].model=&model;p[0].previous=p; saved=p[0];CHECK(!bh_world_geometry_build(p,3) && !memcmp(p,&saved,sizeof(saved)),"malformed scene rejects transactionally");
    p[0].previous=p+2;p[0].position.x=p[0].position.y=p[0].position.z=1e20f;saved=p[0];CHECK(!bh_world_geometry_build(p,3) && !memcmp(p,&saved,sizeof(saved)),"unrepresentable integer plane rejects without partial output");
    CHECK(!bh_world_geometry_build(p,0) && !bh_world_geometry_build(NULL,1),"world argument limits");
    CHECK(bh_geometry_dispose(&h),"nonportal geometry lifecycle");bh_bsp_dispose(&other);
    CHECK(cube(&other,1) && bh_geometry_init(&h,&other,NULL),"mixed material geometry");
    for(i=0;i<6;++i)CHECK(bh_geometry_set_flags(&h,i,0x100),"seed real existing runtime flags");
    CHECK(bh_geometry_model_init(&mixed,&h,5,6) && mixed.portal_count==1 && mixed.portal_faces[0]==0 && mixed.flags==0x46,"game material classification retains model flags and water bit");
    for(i=0;i<6;++i)CHECK(bh_geometry_get_flags(&h,i,&flags) && flags==(0x100|expected_flags[i]),"portal/roof/water/scrap bits OR existing word");
    saved_model=mixed;CHECK(!bh_geometry_model_init(&mixed,&h,0,6) && !memcmp(&mixed,&saved_model,sizeof(mixed)),"invalid grid preserves model output");
    CHECK(bh_geometry_dispose(&h) && bh_geometry_dispose(&g),"model geometry owners dispose after components");bh_bsp_dispose(&other);bh_bsp_dispose(&b);
    memset(&matrix,0,sizeof(matrix));matrix.m[0][0]=matrix.m[1][1]=matrix.m[2][2]=1;matrix.m[0][3]=11;
    bh_mat34_rotate_y(0,&matrix);CHECK(matrix.m[0][0]==1 && matrix.m[2][2]==1 && matrix.m[0][3]==11,"rotation preserves translation");
    for(i=0;i<3;++i)for(j=0;j<4;++j)CHECK(_finite(matrix.m[i][j]),"finite rotation outputs");
}
static void real_scene(const char *archive) {
    BHBlockLibrary library;BHSceneLevel level;BHGeometry *geometries=NULL;BHGeometryModel *models=NULL;BHWorldGeometry *world=NULL;
    unsigned long i,j,initialized=0,connected=0,model_count=0;long index;BHVFile *file;int ok;
    BHNavigation navigation;BHSceneRoutes expected_routes;
    BHVec3 *portal_queries=NULL;
    BHWorldQuery query;BHWorldHit *query_copy=NULL;int query_live=0;
    bh_blocks_init(&library,NULL);bh_scene_level_init(&level,NULL);
    bh_navigation_init(&navigation,NULL);bh_scene_routes_init(&expected_routes,NULL);
    CHECK(bh_vfile_mount(archive) && bh_blocks_load(&library,"BSP\\BLOCKS.TXT","BSP\\",BH_BSP_PREFIX_ONLY),"source block registry for real world construction");
    if(library.count!=454)goto done;
    geometries=(BHGeometry*)calloc(454,sizeof(*geometries));models=(BHGeometryModel*)calloc(454,sizeof(*models));
    CHECK(geometries && models,"real model component allocation");if(!geometries || !models)goto done;
    for(i=0;i<454;++i) {
        ok=bh_geometry_init(geometries+i,&library.blocks[i].bsp,NULL);CHECK(ok,"real mutable geometry dependency");if(!ok)goto done;++initialized;
        ok=bh_geometry_model_init(models+i,geometries+i,world_grids[i],6);CHECK(ok,"source model face classification/bounds");if(!ok)goto done;++model_count;
    }
    file=bh_vfile_open("TEST.LEV","rb");CHECK(file!=NULL && bh_scene_level_read(&level,file),"real level placement input");if(file)bh_vfile_close(file);
    if(level.placement_count!=5815)goto done;
    world=(BHWorldGeometry*)calloc(5815,sizeof(*world));CHECK(world!=NULL,"real world component owner");if(!world)goto done;
    portal_queries=(BHVec3*)malloc(3739UL*24);CHECK(portal_queries!=NULL,"real portal query outputs allocation");if(!portal_queries)goto done;
    for(i=0;i<5815;++i) {
        index=bh_blocks_find(&library,level.placements[i].name);CHECK(index>=0,"real placement model resolution");if(index<0)goto done;
        world[i].model=models+index;world[i].flags=level.placements[i].flags;
        world[i].query_flags=models[index].flags; /* explicit fixture model flags; scripts remain unapplied */
        world[i].position.x=(float)level.placements[i].x;world[i].position.y=(float)level.placements[i].y;world[i].position.z=(float)level.placements[i].z;
        world[i].navigation.next=&world[(i+1)%5815].navigation;world[i].previous=world+(i+5814)%5815;
    }
    for(i=0;i<5815;++i){ok=bh_world_geometry_build(world+i,5815);CHECK(ok,"real placement aperture/bounds constructor");if(!ok){printf("world build failed placement %lu\n",i);goto done;}}
    for(i=0;i<1454;++i) {
        const unsigned long *expected=world_cases[i];BHWorldGeometry *p=world+expected[0];unsigned long word;
        CHECK(p->model==models+expected[1] && p->navigation.count==expected[2],"captured model and portal count reproduced");
        for(j=0;j<6;++j){memcpy(&word,(unsigned char*)&p->minimum+j*4,4);CHECK(word==expected[j+3],"captured world bounds exact bits");}
    }
    for(i=0;i<4;++i)CHECK(world[world_owners[i][0]].owner==world+world_owners[i][1],"captured sensor/launcher owner reproduced");
    for(i=0;i<5815;++i)CHECK(bh_portals_match(&world[i].navigation,5815),"constructed source apertures connect");
    for(i=0;i<3739;++i) {
        const unsigned long *expected=portal_cases[i];BHWorldGeometry *p=world+world_cases[expected[0]][0];BHPortal *port=p->navigation.portals+expected[1];unsigned long word;
        for(j=0;j<14;++j){memcpy(&word,(unsigned char*)port+j*4,4);CHECK(word==expected[j+2],"source constructed portal record exact bits");}
        CHECK(port->neighbor==(expected[16]==0xffffffffUL?NULL:&world[world_cases[expected[16]][0]].navigation),"source constructed neighbor matches captured identity");
        CHECK(bh_portal_center(port,portal_queries+i*2) && bh_portal_normal(port,portal_queries+i*2+1),"real source portal center/normal queries");
        if(port->neighbor)++connected;
    }
    CHECK(connected==3120 && model_count==454,"complete source real-data construction slice");
    CHECK(crc(portal_queries,3739UL*24)==PORTAL_QUERY_CRC,"independent complete portal scratch-center/normal checksum");
    ok=bh_navigation_build(&navigation,world,5815,NULL);CHECK(ok,"real source graph construction and route recomputation");
    if(ok) {
        CHECK(navigation.node_count==1454 && navigation.junction_count==283 && !navigation.cache_used,"real junction/corridor counts derived from source topology");
        if(navigation.node_count==1454)for(i=0;i<1454;++i) {
            CHECK(navigation.nodes[i].placement==world+world_cases[i][0],"captured original node ordering reproduced");
            CHECK(navigation.nodes[i].junction_first_port==(long)navigation_endpoints[i][0] && navigation.nodes[i].junction_second_port==(long)navigation_endpoints[i][1] && navigation.nodes[i].endpoints_valid==navigation_endpoints[i][2],"independent corridor endpoint controls");
        }
        if(navigation.junction_count==283)CHECK(crc(navigation.routes.cells,283UL*283*8)==NAVIGATION_MATRIX_CRC,"independent instruction-derived complete route matrix checksum");
    }
    file=bh_vfile_open("TEST.PTH","rb");CHECK(file && bh_scene_routes_read(&expected_routes,file,283),"independent legacy cache payload through source reader");if(file)bh_vfile_close(file);
    if(navigation.junction_count==283 && expected_routes.node_count==283)
        CHECK(!memcmp(navigation.routes.cells,expected_routes.cells,283UL*283*8),"all 80089 recomputed route cells exactly match original PTH");
    file=bh_vfile_open("TEST.PTH","rb");CHECK(file && bh_navigation_build(&navigation,world,5815,file) && navigation.cache_used,"real graph build with borrowed matching PTH cache");if(file)bh_vfile_close(file);
    if(navigation.junction_count==283 && expected_routes.node_count==283)
        CHECK(!memcmp(navigation.routes.cells,expected_routes.cells,283UL*283*8),"matching-cache branch preserves every original route bit");
    if(navigation.node_count==1454)for(i=0;i<4;++i) {
        const unsigned long *expected=route_query_cases[i];BHVec3 goal;BHRoutePlan plan;unsigned long word;
        memcpy(&goal,expected+4,12);
        ok=bh_route_plan(&navigation,world,5815,navigation.nodes[expected[0]].placement,&goal,&goal,&plan);
        CHECK(ok,"real source route-planning consumer with actual world/graph dependencies");
        if(ok) {
            CHECK(plan.exit_slot==(long)expected[2] && plan.next==navigation.nodes[expected[3]].placement,"original matrix route decision selects actual neighbor");
            for(j=0;j<13;++j){memcpy(&word,(unsigned char*)&plan.goal+j*4,4);CHECK(word==expected[j+4],"independent original-instruction route projection/rate bits");}
        }
    }
    CHECK(bh_collision_models_real_check(&library),"complete collision loader matches all 454 source registry models and resets runtime state");
    CHECK(bh_sweep_real_check(geometries,initialized),"swept source collision repeats on all real BSP models");
    CHECK(bh_collision_queries_real_check(geometries,initialized),"source collision tree queries repeat and clean counters on all real models");
    CHECK(bh_segment_real_check(geometries,initialized),"real BSP segment queries execute and repeat over all source models");
    CHECK(bh_position_real_check(&navigation),"all 1454 source world packed-position outputs match independent saved-bound CRC");
    ok=bh_world_query_init(&query,world,5815,NULL,0,NULL);CHECK(ok,"real source-scene broad-phase workspace");
    if(ok) {
        query_live=1;query_copy=(BHWorldHit*)malloc(query.capacity*sizeof(BHWorldHit));CHECK(query_copy!=NULL,"private broad-phase repeat control storage");
        for(i=0;query_copy && i<4;++i) {
            BHWorldGeometry *owner=NULL,*again=NULL;BHVec3 point;unsigned long count;
            const unsigned long *expected=route_query_cases[i];memcpy(&point,expected+7,12);
            ok=bh_world_query_run(&query,NULL,navigation.nodes[expected[0]].placement,&point,0.5f,&owner);
            CHECK(ok && owner && query.hit_count>0,"real portal-boundary broad-phase collection");
            if(ok) {
                CHECK(query.hits[0].kind==BH_WORLD_HIT_GEOMETRY && query.hits[0].object.geometry==owner && query.hits[query.hit_count].kind==BH_WORLD_HIT_END,"real candidate ordering and terminator");
                count=query.hit_count;memcpy(query_copy,query.hits,(count+1)*sizeof(BHWorldHit));
                CHECK(bh_world_query_run(&query,NULL,navigation.nodes[expected[0]].placement,&point,0.5f,&again) && again==owner && query.hit_count==count && !memcmp(query_copy,query.hits,(count+1)*sizeof(BHWorldHit)),"real broad-phase repeat preserves exact candidates and clears visitation");
            }
        }
    }
done:
    if(query_copy)free(query_copy);if(query_live)bh_world_query_dispose(&query);
    bh_navigation_dispose(&navigation);bh_scene_routes_dispose(&expected_routes);
    if(portal_queries)free(portal_queries);
    if(world)free(world);if(models)free(models);
    for(i=0;i<initialized;++i)bh_geometry_dispose(geometries+i);if(geometries)free(geometries);
    bh_scene_level_dispose(&level);bh_blocks_dispose(&library);bh_vfile_shutdown();
}
int bh_world_geometry_checkpoint(const char *archive,unsigned long *count) {
    checks=0;failures=0;basics();if(archive)real_scene(archive);*count=checks;
    printf("{\"scenario\":\"world-geometry\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}
