#include "../src/bh_collision_resource.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<12)printf("FAIL collision resource %d: %s\n",line,label);}}
static void paths(void) {
    BHCollisionResourcePath path,before;char long_path[258];long policy;unsigned long i;
    for(policy=-2;policy<5;++policy) {
        memset(&path,0x5a,sizeof(path));
        CHECK(bh_collision_resource_path(&path,"BSP",policy) && !strcmp(path.prefix,"BSP\\") && path.search==(policy>=0 && policy<3?(BHBspSearch)policy:BH_BSP_BARE_ONLY),"literal collision prefix and all policy branches");
        CHECK((unsigned char)path.prefix[5]==0x5a,"copy/appended terminator preserves following context bytes");
        CHECK(bh_collision_resource_path(&path,"",policy) && !path.prefix[0] && path.search==BH_BSP_BARE_ONLY && path.prefix[1]=='S',"empty path forces bare-only and preserves unused prefix tail");
    }
    CHECK(bh_collision_resource_path(&path,"Mixed/",2) && !strcmp(path.prefix,"Mixed/\\"),"forward slash and case retained literally before required backslash");
    CHECK(bh_collision_resource_path(&path,"BSP\\",0) && !strcmp(path.prefix,"BSP\\"),"existing backslash is not duplicated");
    CHECK(bh_collision_resource_path(&path,path.prefix,1) && !strcmp(path.prefix,"BSP\\") && path.search==BH_BSP_BARE_FIRST,"same owning buffer input remains valid");
    memset(long_path,'a',sizeof(long_path));long_path[254]=0;
    CHECK(bh_collision_resource_path(&path,long_path,2) && path.prefix[254]=='\\' && !path.prefix[255],"maximum appendable bounded prefix");
    long_path[254]='a';long_path[255]=0;before=path;
    CHECK(!bh_collision_resource_path(&path,long_path,1) && !memcmp(&path,&before,sizeof(path)),"missing suffix at capacity rejects atomically");
    long_path[254]='\\';CHECK(bh_collision_resource_path(&path,long_path,1) && path.prefix[254]=='\\' && !path.prefix[255],"maximum already terminated prefix accepted");
    for(i=0;i<257;++i)long_path[i]='x';long_path[257]=0;before=path;
    CHECK(!bh_collision_resource_path(&path,long_path,0) && !memcmp(&path,&before,sizeof(path)),"overlong source guard preserves both original-global equivalents");
    CHECK(!bh_collision_resource_path(0,"BSP",0) && !bh_collision_resource_path(&path,0,0) && !memcmp(&path,&before,sizeof(path)),"null source binding guards");
}
static int model_fixture(const char *name) {
    BHBspMaterial material;BHBspVertex vertices[3];BHBspEdge edges[3];BHBspFace face;BHBspNode node;BHBspLeaf leaf;
    unsigned long counts[6]={1,3,3,1,1,1},sizes[6],i;const void *sections[6];FILE *file=fopen(name,"wb");
    if(!file)return 0;
    memset(&material,0,sizeof(material));strcpy(material.texture,"STONE.PCX");
    memset(vertices,0,sizeof(vertices));memset(edges,0,sizeof(edges));
    for(i=0;i<3;++i){edges[i].vertex=i;edges[i].next=i<2?(long)i+1:-1;}
    vertices[0].y=vertices[0].z=-1;vertices[1].y=1;vertices[1].z=-1;vertices[2].z=1;
    face.material=face.first_edge=0;face.runtime=0x12345678;
    memset(&node,0,sizeof(node));for(i=0;i<3;++i){node.minimum[i]=-1;node.maximum[i]=1;}
    node.normal[0]=1;node.front_node=node.back_node=node.back_leaf=-1;leaf.face=0;leaf.next=-1;
    sections[0]=&material;sections[1]=vertices;sections[2]=edges;sections[3]=&face;sections[4]=&node;sections[5]=&leaf;
    sizes[0]=sizeof(material);sizes[1]=sizeof(vertices);sizes[2]=sizeof(edges);sizes[3]=sizeof(face);sizes[4]=sizeof(node);sizes[5]=sizeof(leaf);
    for(i=0;i<6;++i)if(fwrite(counts+i,1,4,file)!=4 || fwrite(sections[i],1,sizes[i],file)!=sizes[i]){fclose(file);return 0;}
    return !fclose(file);
}
static void load(const char *archive) {
    BHCollisionResourcePath path;BHCollisionModel model;unsigned char *storage;
    CHECK(model_fixture("COLLISION_PATH.BSP"),"actual serialized six-section triangle collision BSP fixture");
    CHECK(bh_collision_model_init(&model,0),"real collision model owner initializes");
    CHECK(bh_collision_resource_path(&path,"MISSING",0) && bh_collision_resource_load(&path,&model,"COLLISION_PATH.BSP"),"actual prefixed-first VFILE open falls back to bare source fixture");
    CHECK(model.geometry.bsp==&model.bsp && model.bsp.bytes==360 && model.bsp.counts[0]==1 && model.bsp.faces[0].runtime==0,"actual source loader decoded and bound collision geometry and reset live face metadata");
    storage=model.bsp.storage;
    CHECK(bh_collision_resource_path(&path,"MISSING",2) && !bh_collision_resource_load(&path,&model,"COLLISION_PATH.BSP") && model.bsp.error==BH_BSP_IO && model.bsp.storage==storage,"prefix-only rejects absent prefix retaining real previously loaded model");
    CHECK(bh_collision_resource_path(&path,"MISSING",99) && bh_collision_resource_load(&path,&model,"COLLISION_PATH.BSP"),"out-of-range policy actually selects bare-only loader path");
    if(archive) {
        CHECK(bh_vfile_mount(archive),"mount pinned archive as immutable collision model data");
        CHECK(bh_collision_resource_path(&path,"BSP",2) && bh_collision_resource_load(&path,&model,"PU_9A.BSP"),"actual configured prefix-only archive collision resource load");
        CHECK(model.bsp.counts[0]==2 && model.bsp.counts[1]==26 && model.bsp.counts[3]==64 && model.geometry.bsp==&model.bsp,"real archive model section metadata and geometry ownership");
    }
    CHECK(!bh_collision_resource_load(0,&model,"COLLISION_PATH.BSP") && !bh_collision_resource_load(&path,0,"COLLISION_PATH.BSP") && !bh_collision_resource_load(&path,&model,0),"source composition guards fabricate no model");
    CHECK(bh_collision_model_dispose(&model),"actual collision geometry and BSP ownership dispose");bh_vfile_shutdown();remove("COLLISION_PATH.BSP");
}
int bh_collision_resource_checkpoint(const char *archive,unsigned long *count) {
    BHMathEnvironment environment;checks=0;failures=0;if(!bh_math_environment_enter(&environment))return 1;
    paths();load(archive);bh_math_environment_leave(&environment);if(count)*count=checks;
    printf("{\"scenario\":\"collision-resource\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_COLLISION_RESOURCE_STANDALONE
int main(int argc,char **argv){unsigned long count;return bh_collision_resource_checkpoint(argc>1?argv[1]:0,&count);}
#endif
