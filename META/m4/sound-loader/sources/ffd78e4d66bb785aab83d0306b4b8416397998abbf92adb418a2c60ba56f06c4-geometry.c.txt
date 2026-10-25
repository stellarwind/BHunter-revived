#include "../src/bh_geometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef struct Case {const char *name;unsigned long face_crc,vertex_crc,missing;} Case;
static const Case cases[]={
#include "geometry_cases.h"
};
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL geometry line %d: %s\n",line,label);}}
static unsigned long crc(unsigned long value,const void *bytes,unsigned long size) {
    const unsigned char *p=(const unsigned char*)bytes;unsigned long i,j;
    for(i=0;i<size;++i){value^=p[i];for(j=0;j<8;++j)value=(value>>1)^((value&1)?0xedb88320UL:0);}
    return value;
}
static void section(unsigned char *bytes,unsigned long *at,unsigned long count,const void *data,unsigned long size) {
    memcpy(bytes+*at,&count,4);*at+=4;memcpy(bytes+*at,data,size);*at+=size;
}
static int synthetic(BHBsp *b,int normals) {
    unsigned char bytes[2048];unsigned long at=0;BHBspMaterial materials[2];BHBspVertex vertices[6];
    BHBspEdge edges[6];BHBspFace faces[2];BHBspNode nodes[2];BHBspLeaf leaves[4];unsigned long i;
    memset(materials,0,sizeof(materials));strcpy(materials[0].texture,"portal.gif");strcpy(materials[1].texture,"STONE.PCX");
    memset(vertices,0,sizeof(vertices));for(i=0;i<6;++i){vertices[i].x=(float)(i*2);vertices[i].y=(float)(i*3);vertices[i].z=-(float)i;}
    memset(edges,0,sizeof(edges));for(i=0;i<6;++i){edges[i].vertex=5-i;edges[i].next=-1;}
    edges[3].next=0;edges[0].next=5;edges[1].next=4;edges[4].next=2;
    memset(faces,0,sizeof(faces));faces[0].first_edge=3;faces[1].first_edge=1;faces[1].material=1;faces[0].runtime=0x12345678;
    memset(nodes,0,sizeof(nodes));for(i=0;i<2;++i) {
        nodes[i].minimum[0]=nodes[i].minimum[1]=nodes[i].minimum[2]=-100;
        nodes[i].maximum[0]=nodes[i].maximum[1]=nodes[i].maximum[2]=100;
        nodes[i].front_node=nodes[i].back_node=-1;nodes[i].front_leaf=nodes[i].back_leaf=-1;
    }
    nodes[0].normal[0]=1;nodes[0].normal[1]=2;nodes[0].normal[2]=3;
    nodes[1].normal[0]=-4;nodes[1].normal[1]=5;nodes[1].normal[2]=-6;
    if(normals){nodes[0].front_leaf=0;nodes[0].back_leaf=1;nodes[1].front_leaf=3;}
    leaves[0].face=0;leaves[0].next=2;leaves[1].face=1;leaves[1].next=-1;
    leaves[2].face=0;leaves[2].next=-1;leaves[3].face=1;leaves[3].next=2;
    section(bytes,&at,2,materials,sizeof(materials));section(bytes,&at,6,vertices,sizeof(vertices));
    section(bytes,&at,6,edges,sizeof(edges));section(bytes,&at,2,faces,sizeof(faces));
    section(bytes,&at,2,nodes,sizeof(nodes));section(bytes,&at,4,leaves,sizeof(leaves));
    return bh_bsp_decode(b,bytes,at);
}
typedef struct Memory {unsigned long live,allocations;int reject;} Memory;
static void *__cdecl alloc(unsigned long n,void *user){Memory *m=(Memory*)user;void *p;++m->allocations;if(m->reject)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl drop(void *p,void *user){Memory *m=(Memory*)user;CHECK(m->live>0,"balanced geometry ownership");--m->live;free(p);}
static unsigned long callback_count;static int reject_nested;
static void __cdecl callback(BHFaceInfo *face,void *user) {
    BHGeometry *g=(BHGeometry*)user;
    CHECK(face->face==(long)callback_count,"face callback order");++callback_count;
    CHECK(bh_geometry_set_flags(g,face->face,0xabcdUL+face->face),"real callback mutates flag owner");
    reject_nested+=!bh_geometry_each_face(g,callback,user) && !bh_geometry_dispose(g);
}
static void basics(void) {
    BHBsp bsp;BHGeometry g,saved;BHFaceInfo info,before;BHVec3 point={99,98,97},old;unsigned long flags=7;
    Memory memory={0,0,0};BHSceneAllocator allocator;unsigned long i;
    memset(&g,0,sizeof(g));allocator.allocate=alloc;allocator.release=drop;allocator.user=&memory;
    CHECK(bh_bsp_init(&bsp,NULL) && synthetic(&bsp,1),"decode synthetic real BSP layout");
    memset(&g,0x5a,sizeof(g));saved=g;memory.reject=1;
    CHECK(!bh_geometry_init(&g,&bsp,&allocator) && !memcmp(&g,&saved,sizeof(g)) && !memory.live,"metadata allocation failure preserves caller output");
    memory.reject=0;CHECK(bh_geometry_init(&g,&bsp,&allocator) && memory.live==1,"geometry owns flags");
    CHECK(bh_geometry_get_flags(&g,0,&flags) && flags==0,"flags zeroed independently of serialized runtime word");
    CHECK(bh_geometry_set_flags(&g,0,0xdeadbeefUL) && bh_geometry_get_flags(&g,0,&flags) && flags==0xdeadbeefUL,"exact full flag word roundtrip");
    memset(&info,0,sizeof(info));info.normal.x=11;info.normal.y=12;info.normal.z=13;
    CHECK(bh_geometry_face(&g,0,&info) && info.face==0 && info.vertices==3 && info.flags==0xdeadbeefUL && info.geometry==&g,"descriptor fields and real owner");
    CHECK(!strcmp(info.texture,"portal.gif") && info.point.x==4 && info.point.y==6 && info.point.z==-2,"descriptor material and first chain vertex");
    CHECK(info.normal.x==1 && info.normal.y==2 && info.normal.z==3,"shared-tail ancestry selects first predecessor");
    CHECK(bh_geometry_face(&g,1,&info) && info.normal.x==-4 && info.normal.y==5 && info.normal.z==-6,"last matching leaf overrides earlier normal");
    CHECK(bh_geometry_vertex(&g,0,1,&point) && point.x==10 && point.y==15 && point.z==-5,"noncontiguous edge order");
    CHECK(bh_geometry_vertex(&g,0,2,&point) && point.x==0 && point.y==0 && point.z==0,"last edge vertex");old=point;
    CHECK(!bh_geometry_vertex(&g,0,3,&point) && !memcmp(&point,&old,12),"ordinal past chain rejects without output writes");
    before=info;CHECK(!bh_geometry_face(&g,-1,&info) && !memcmp(&before,&info,sizeof(info)),"negative face rejection");
    CHECK(!bh_geometry_get_flags(&g,2,&flags) && flags==0xdeadbeefUL && !bh_geometry_set_flags(&g,-1,0),"invalid flag index");
    callback_count=0;reject_nested=0;
    CHECK(bh_geometry_each_face(&g,callback,&g) && callback_count==2 && reject_nested==2,"real enumeration and nested/dispose guard");
    for(i=0;i<2;++i)CHECK(bh_geometry_get_flags(&g,i,&flags) && flags==0xabcdUL+i,"callback flags persisted");
    CHECK(bh_geometry_dispose(&g) && !memory.live,"geometry teardown releases flags");bh_bsp_dispose(&bsp);
    CHECK(synthetic(&bsp,0) && bh_geometry_init(&g,&bsp,NULL),"geometry with no assigned normal");
    memset(&info,0,sizeof(info));info.normal.x=7;info.normal.y=-8;info.normal.z=9;
    CHECK(bh_geometry_face(&g,0,&info) && info.normal.x==7 && info.normal.y==-8 && info.normal.z==9,"missing normal preserves initialized caller value");
    CHECK(bh_geometry_dispose(&g),"second geometry teardown");bh_bsp_dispose(&bsp);
}
typedef struct Hashes {unsigned long faces,vertices;} Hashes;
static void __cdecl hash_face(BHFaceInfo *face,void *user) {
    Hashes *h=(Hashes*)user;BHVec3 point;unsigned long i;
    h->faces=crc(h->faces,&face->face,12);h->faces=crc(h->faces,face->texture,23);
    h->faces=crc(h->faces,&face->point,24);
    for(i=0;i<face->vertices;++i) {
        CHECK(bh_geometry_vertex(face->geometry,face->face,i,&point),"real local vertex ordinal");
        h->vertices=crc(h->vertices,&point,12);
    }
}
static void real_data(const char *archive) {
    BHBsp bsp;BHGeometry g;unsigned long i;Hashes hashes;int ok;
    CHECK(bh_vfile_mount(archive),"mount source VFILE for all BSP face consumers");
    bh_bsp_init(&bsp,NULL);
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        CHECK(bh_bsp_load(&bsp,cases[i].name,"",BH_BSP_BARE_ONLY),"load real BSP data");
        if(bsp.error)break;
        ok=bh_geometry_init(&g,&bsp,NULL);CHECK(ok,"real BSP mutable metadata owner");
        if(!ok)break;
        hashes.faces=hashes.vertices=0xffffffffUL;
        CHECK(bh_geometry_each_face(&g,hash_face,&hashes),"real face descriptor enumeration");
        CHECK((hashes.faces^0xffffffffUL)==cases[i].face_crc,"independent real descriptor/name/normal checksum");
        CHECK((hashes.vertices^0xffffffffUL)==cases[i].vertex_crc,"independent real edge-ordered vertex checksum");
        CHECK(bh_geometry_dispose(&g),"real metadata lifetime before BSP reload");
    }
    bh_bsp_dispose(&bsp);bh_vfile_shutdown();CHECK(bh_vfile_entry_count()==0,"source VFILE lifetime ends after geometry");
}
int bh_geometry_checkpoint(const char *archive,unsigned long *count) {
    checks=0;failures=0;basics();if(archive)real_data(archive);*count=checks;
    printf("{\"scenario\":\"geometry-faces\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}
