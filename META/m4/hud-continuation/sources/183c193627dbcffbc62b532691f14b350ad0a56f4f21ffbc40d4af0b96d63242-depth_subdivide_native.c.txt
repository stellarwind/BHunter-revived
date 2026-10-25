#include "../src/bh_depth_subdivide.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL depth-subdivide-native: %s\n",s);}} while(0)
typedef struct LeafBinding {BHNativePrimitive native;BHTextureImage *image;unsigned long color;} LeafBinding;
/* Real source native primitive binding: no software kernel or fake device. */
static int __cdecl native_leaf(BHClipVertex *ring,void *user) {
    LeafBinding *b=(LeafBinding *)user;return bh_native_primitive_emit(ring,b->image,b->color,&b->native);
}
static unsigned long pixel(const char *path,unsigned long x,unsigned long y) {
    FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned char b[3];unsigned long stride;
    if(!f)return 0xffffffffUL;
    if(fread(&h,sizeof(h),1,f)!=1 || fread(&info,sizeof(info),1,f)!=1 || info.biBitCount!=24){fclose(f);return 0xffffffffUL;}
    stride=(info.biWidth*3+3)&~3UL;
    if(fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET) || fread(b,1,3,f)!=3){fclose(f);return 0xffffffffUL;}
    fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
int bh_depth_subdivide_native_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHRenderMaterial material;BHRenderState state;LeafBinding leaf;BHDepthSplitContext split;
    BHPcx pcx;unsigned char pixels[1024];BHClipVertex v[3],before[3];BHMathEnvironment fp;unsigned long variant,i;float a;
    const char *names[3]={"depth-native-plain.bmp","depth-native-scalar.bmp","depth-native-black.bmp"};
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&leaf,0,sizeof(leaf));
    CHECK(bh_scene_view_init(&view,64,64),"actual native offscreen source scene");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"real texture context and format ownership");
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
    leaf.image=bh_texture_stage(&textures,"DEPTHWHITE.PCX",&pcx,0,0);
    CHECK(leaf.image && leaf.image->source && leaf.image->palette,"real native white texture source and palette");if(!leaf.image)goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"actual original material and state owners");
    leaf.native.textures=&textures;leaf.native.near_z=1;view.original_depth=1;
    for(variant=0;variant<3;++variant) {
        memset(v,0,sizeof(v));v[0].sx=8;v[0].sy=8;v[1].sx=8;v[1].sy=52;v[2].sx=52;v[2].sy=8;
        for(i=0;i<3;++i){v[i].point.z=i==2?17:15;v[i].u=v[i].v=.5f;v[i].next=v+(i+1)%3;v[i].previous=v+(i+2)%3;a=i*.5f;memcpy(&v[i].unknown28,&a,4);}
        memcpy(before,v,sizeof(v));leaf.color=variant==2?0xff000000UL:0xffffffffUL;leaf.native.polygons=leaf.native.vertices=0;
        CHECK(bh_depth_split_init(&split,0,64,0,64,native_leaf,&leaf) && bh_scene_view_begin(&view),"selected real primitive callback and actual scene begin");
        CHECK(variant==1?bh_depth_subdivide_scalar(&split,v):bh_depth_subdivide(&split,v),"complete recursive source owner reaches real primitive leaf");
        CHECK(split.splits==1 && split.leaves==2 && leaf.native.polygons==2 && leaf.native.vertices==7 &&
            leaf.image->run_count==2 && leaf.image->vertex_count==7,"near quad and far triangle publish real TL runs");
        CHECK(!memcmp(v,before,sizeof(v)),"native sink retains no temporary stack nodes");
        CHECK(bh_render_mode(&state,0) && !leaf.image->vertex_count && !leaf.image->run_count,"real state barrier flushes both source queue runs");
        CHECK(bh_scene_view_end(&view,names[variant]),"actual native readback artifact");
        CHECK(pixel(names[variant],16,16)==(variant==2?0:0xffffffUL) && pixel(names[variant],50,50)==0,
            "independent interior/exterior pixels plus black-color positive control");
    }
done:
    bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"native selected leaf preserves PC53 profile");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"depth-subdivide-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_DEPTH_SUBDIVIDE_NATIVE_STANDALONE
int main(void){unsigned long count;return bh_depth_subdivide_native_checkpoint(&count);}
#endif
