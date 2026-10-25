#include "../src/bh_camera_route.h"
#include "../src/bh_blocks.h"
#include "../src/bh_fp.h"
#include "world_geometry_cases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int bh_camera_route_real_check(const BHNavigation *,BHWorldGeometry *,unsigned long,unsigned long *);
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL camera-route-real: %s\n",s);}} while(0)
int bh_camera_route_real_checkpoint(const char *archive,unsigned long *count) {
    BHMathEnvironment fp;BHBlockLibrary library;BHSceneLevel level;BHNavigation navigation;
    BHGeometry *geometry=NULL;BHGeometryModel *models=NULL;BHWorldGeometry *world=NULL;
    unsigned long i,initialized=0,extra=0;long index;BHVFile *file;int ok;
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    bh_blocks_init(&library,NULL);bh_scene_level_init(&level,NULL);bh_navigation_init(&navigation,NULL);
    if(!archive)goto done;
    CHECK(bh_vfile_mount(archive) && bh_blocks_load(&library,"BSP\\BLOCKS.TXT","BSP\\",BH_BSP_PREFIX_ONLY) && library.count==454,"real block registry source");
    if(library.count!=454)goto done;
    geometry=(BHGeometry *)calloc(454,sizeof(*geometry));models=(BHGeometryModel *)calloc(454,sizeof(*models));
    CHECK(geometry && models,"real model component storage");if(!geometry || !models)goto done;
    for(i=0;i<454;++i){ok=bh_geometry_init(geometry+i,&library.blocks[i].bsp,NULL);if(!ok)break;++initialized;
        if(!bh_geometry_model_init(models+i,geometry+i,world_grids[i],6))break;}
    CHECK(i==454,"all actual model geometry and portal bounds constructed");if(i!=454)goto done;
    file=bh_vfile_open("TEST.LEV","rb");ok=file && bh_scene_level_read(&level,file);if(file)bh_vfile_close(file);
    CHECK(ok && level.placement_count==5815,"actual level placement data");if(level.placement_count!=5815)goto done;
    world=(BHWorldGeometry *)calloc(5815,sizeof(*world));CHECK(world!=NULL,"real world ring storage");if(!world)goto done;
    for(i=0;i<5815;++i){index=bh_blocks_find(&library,level.placements[i].name);if(index<0)break;
        world[i].model=models+index;world[i].flags=level.placements[i].flags;
        world[i].position.x=(float)level.placements[i].x;world[i].position.y=(float)level.placements[i].y;world[i].position.z=(float)level.placements[i].z;
        world[i].navigation.next=&world[(i+1)%5815].navigation;world[i].previous=world+(i+5814)%5815;}
    CHECK(i==5815,"actual placement model names resolve");if(i!=5815)goto done;
    for(i=0;i<5815;++i)if(!bh_world_geometry_build(world+i,5815))break;
    CHECK(i==5815,"all actual world bounds and apertures source-built");if(i!=5815)goto done;
    for(i=0;i<5815;++i)if(!bh_portals_match(&world[i].navigation,5815))break;
    CHECK(i==5815,"actual portals connected by recovered owner");if(i!=5815)goto done;
    file=bh_vfile_open("TEST.PTH","rb");ok=file && bh_navigation_build(&navigation,world,5815,file);if(file)bh_vfile_close(file);
    CHECK(ok && navigation.node_count==1454 && navigation.junction_count==283 && navigation.cache_used,"actual navigation graph and real PTH route cells");
    if(!ok || navigation.node_count!=1454 || navigation.junction_count!=283)goto done;
    CHECK(bh_camera_route_real_check(&navigation,world,5815,&extra),"complete camera route owner on real graph and real selected goal positions");checks+=extra;
done:
    bh_navigation_dispose(&navigation);free(world);for(i=0;i<initialized;++i)bh_geometry_dispose(geometry+i);
    free(models);free(geometry);bh_scene_level_dispose(&level);bh_blocks_dispose(&library);if(archive)bh_vfile_shutdown();
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"camera-route-real\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"archive_skipped\":%s}\n",checks,failures,archive?"false":"true");return failures!=0;
}
#ifdef BH_CAMERA_ROUTE_REAL_STANDALONE
int main(int argc,char **argv){unsigned long count=0;return bh_camera_route_real_checkpoint(argc>1?argv[1]:NULL,&count);}
#endif
