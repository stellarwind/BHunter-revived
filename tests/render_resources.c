#include "../src/bh_render_resources.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks;
static int failures;
#define CHECK(value,label) check((value)!=0,label,__LINE__)
static void check(int ok,const char *label,int line) {
    ++checks;if(!ok){++failures;if(failures<12)
        printf("FAIL render resources %d: %s\n",line,label);}
}
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long bytes,void *user) {
    Memory *m=(Memory *)user;void *p;
    ++m->calls;if(m->calls==m->fail_at)return NULL;
    p=malloc(bytes);if(p)++m->live;return p;
}
static void __cdecl release(void *pointer,void *user) {
    Memory *m=(Memory *)user;--m->live;free(pointer);
}

int bh_render_resources_checkpoint(const char *archive,unsigned long *count) {
    BHRenderResources owner;BHSceneView view;BHTextureContext textures;
    BHModelContext models;BHModelInstanceContext prototype;BHMainCamera camera;
    BHClipContext clip;BHNativePrimitive native;
    BHSceneAllocator allocator;BHMathEnvironment fp;Memory memory={0,0,0};
    const BHDrawResource *first,*second;unsigned long h1,h2,i,old_count;
    checks=0;failures=0;memset(&owner,0,sizeof(owner));
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));
    memset(&models,0,sizeof(models));memset(&camera,0,sizeof(camera));
    memset(&prototype,0,sizeof(prototype));
    if(!bh_math_environment_enter(&fp))return 2;
    allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
    CHECK(!bh_render_resources_init(&owner,&models,&prototype,&allocator),
        "uninitialized model loader is rejected");
    CHECK(!bh_render_resources_lookup(0,&owner) &&
        !bh_render_resources_lookup(1,&owner),"zero and unknown handles reject");
    CHECK(!bh_render_resources_load_model(&owner,"") &&
        owner.error==BH_RENDER_RESOURCES_ARGUMENT && !owner.count,
        "invalid model name retains empty owner");
    bh_render_resources_dispose(&owner);
    if(archive) {
        CHECK(bh_scene_view_init(&view,64,64),"native texture dependency for real BSP");
        if(view.device) {
            CHECK(bh_vfile_mount(archive),"real archive mounted as data");
            CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) &&
                bh_texture_formats(&textures) &&
                bh_texture_set_path(&textures,"TEXTURES",2),
                "real texture cache and format bindings");
            CHECK(bh_model_context_init(&models,&textures,&allocator),
                "real recovered model load context");
            CHECK(!bh_render_resources_init(&owner,&models,&prototype,&allocator) && !owner.entries,
                "missing render bindings reject before publishing descriptors");
            CHECK(bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0) &&
                bh_clip_init(&clip,view.projection.near_z,view.projection.far_z,
                    view.projection.left,view.projection.right,view.projection.top,view.projection.bottom),
                "real camera and clip bindings precede model publication");
            memset(&native,0,sizeof(native));native.textures=&textures;native.near_z=view.projection.near_z;
            prototype.camera=&camera;
            prototype.config=&view.projection;
            prototype.clip=&clip;prototype.emit=bh_native_primitive_emit;prototype.emit_user=&native;
            prototype.frame=22;prototype.drawn=1;prototype.faces_marked=19;
            prototype.polygons_emitted=7;
            CHECK(bh_render_resources_init(&owner,&models,&prototype,&allocator),
                "model resource owner copies borrowed render bindings");
            h1=bh_render_resources_load_model(&owner,"BSP\\PU_9A.BSP");
            first=bh_render_resources_lookup(h1,&owner);
            CHECK(h1==1 && first && first->tag==BH_RESOURCE_TAG_MODEL &&
                first->data.model.instance->model &&
                first->data.model.instance->cache->model==
                    first->data.model.instance->model,
                "real PU_9A BSP yields model tag, owned arrays and cache");
            CHECK(first && first->data.model.instance->camera==prototype.camera &&
                first->data.model.instance->config==prototype.config &&
                first->data.model.instance->frame==0 &&
                !first->data.model.instance->drawn &&
                !first->data.model.instance->faces_marked &&
                !first->data.model.instance->polygons_emitted,
                "borrowed bindings copied and per-model epoch reset");
            CHECK(!bh_render_resources_init(&owner,&models,&prototype,&allocator) &&
                owner.count==1 && bh_render_resources_lookup(h1,&owner)==first,
                "reinitializing a populated owner rejects without losing ownership");
            h2=bh_render_resources_load_model(&owner,"BSP\\PU_9A.BSP");
            second=bh_render_resources_lookup(h2,&owner);
            CHECK(h2==2 && second && first!=second &&
                first->data.model.instance->model!=second->data.model.instance->model &&
                first->data.model.instance->cache!=second->data.model.instance->cache,
                "two handles own independent model and runtime cache");
            memory.fail_at=memory.calls+1;old_count=owner.count;
            CHECK(!bh_render_resources_load_model(&owner,"BSP\\PU_9A.BSP") &&
                owner.error==BH_RENDER_RESOURCES_MEMORY &&
                owner.count==old_count &&
                bh_render_resources_lookup(h1,&owner)==first,
                "failed owner allocation preserves published slots");
            memory.fail_at=0;
            for(i=0;i<8;++i)CHECK(bh_render_resources_load_model(&owner,
                "BSP\\PU_9A.BSP")==i+3,"table growth publishes stable slot handles");
            CHECK(bh_render_resources_lookup(h1,&owner)==first &&
                bh_render_resources_lookup(h2,&owner)==second &&
                !bh_render_resources_lookup(owner.count+1,&owner),
                "descriptor pointers survive table growth and bounds remain checked");
            bh_render_resources_dispose(&owner);
            CHECK(!memory.live,"all model/cache/table allocations released");
            bh_texture_context_dispose(&textures);bh_vfile_shutdown();
        }
        bh_scene_view_dispose(&view);
    }
    bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"render-resources\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",
        checks,failures);
    return failures!=0;
}
#ifdef BH_RENDER_RESOURCES_STANDALONE
int main(int argc,char **argv) {
    unsigned long count;return bh_render_resources_checkpoint(argc>1?argv[1]:NULL,&count);
}
#endif
