#include "../src/bh_model_draw_lit.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<18)printf("FAIL model draw native %d: %s\n",line,label);}}
static unsigned long pixel(const char *name,unsigned long x,unsigned long y) {
    FILE *file=fopen(name,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned long stride;unsigned char b[3];
    if(!file)return 0xffffffffUL;
    if(fread(&h,sizeof(h),1,file)!=1 || fread(&info,sizeof(info),1,file)!=1 || h.bfType!=0x4d42 || info.biBitCount!=24){fclose(file);return 0xffffffffUL;}
    stride=(info.biWidth*3+3)&~3UL;if(fseek(file,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET) || fread(b,1,3,file)!=3){fclose(file);return 0xffffffffUL;}fclose(file);
    return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
int bh_model_draw_native_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHTextureImage *image=NULL,*images[1];BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;
    BHModel model;BHBspVertex vertices[3];BHBspEdge edges[3];BHBspFace face;BHBspNode node;BHBspLeaf leaf;BHModelDrawCache cache;BHModelDrawContext draw;BHModelDrawLitContext lit;
    BHLightConfig directional;BHLightContext lights;BHLight point;BHViewCamera camera;BHClipContext clip;BHMat34 orientation;BHMathEnvironment fp;
    BHPcx pcx;unsigned char pixels[1024];unsigned long i,mode,expected,prior,queued;double focal,projected_left;
    const char *names[5]={"model-native-unlit-blue.bmp","model-native-lit-red.bmp","model-native-lit-green.bmp","model-native-point-blue.bmp","model-native-mask-zero.bmp"};
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&cache,0,sizeof(cache));memset(&native,0,sizeof(native));
    CHECK(bh_scene_view_init(&view,64,64),"real native RGB offscreen scene target");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"real texture context and native format enumeration");
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
    image=bh_texture_stage(&textures,"MODELDRAWWHITE.PCX",&pcx,0,0);CHECK(image && image->source && image->palette,"actual staged white native texture and palette interfaces");if(!image)goto done;image->flags=1;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"real original material/default state and depth owners");
    CHECK(bh_view_config_init(&view.projection,64,64),"original60-degree near1 projection config");view.projection.far_z=100;view.original_depth=1;identity(&orientation);
    CHECK(bh_view_camera_build(&view.projection,&orientation,&camera) && bh_clip_init(&clip,1,100,0,64,0,64),"real per-model projection and complete ring clipper configuration");
    memset(&model,0,sizeof(model));memset(vertices,0,sizeof(vertices));memset(edges,0,sizeof(edges));memset(&face,0,sizeof(face));memset(&node,0,sizeof(node));memset(&leaf,0,sizeof(leaf));
    vertices[0].x=-1;vertices[0].y=-1;vertices[1].x=0;vertices[1].y=1;vertices[2].x=1;vertices[2].y=-1;
    for(i=0;i<3;++i){vertices[i].z=4;edges[i].vertex=i;edges[i].next=i==2?-1:(long)i+1;edges[i].u=edges[i].v=.5f;}
    node.minimum[0]=node.minimum[1]=-1;node.maximum[0]=node.maximum[1]=1;node.minimum[2]=node.maximum[2]=4;node.normal[2]=1;node.plane=-4;
    node.front_leaf=-1;node.back_leaf=0;node.front_node=node.back_node=-1;leaf.face=0;leaf.next=-1;
    model.vertex_count=3;model.vertices=vertices;model.edge_count=3;model.edges=edges;model.face_count=1;model.faces=&face;model.node_count=1;model.nodes=&node;model.leaf_count=1;model.leaves=&leaf;model.material_count=1;images[0]=image;model.images=images;
    CHECK(bh_model_draw_cache_init(&cache,&model,NULL),"real typed model runtime cache owner");if(!cache.vertices)goto done;
    native.textures=&textures;native.near_z=1;memset(&lights,0,sizeof(lights));identity(&lights.world_to_model);memset(&point,0,sizeof(point));point.type=3;point.color=0x0000ff;point.intensity=1;point.position.z=8;lights.lights=&point;
    {BHVec3 center={0,0,4};CHECK(bh_light_prepare(&lights,&center,2) && point.prepared_type==3,"real point-light preparation retains true model-local evaluation mode");}
    focal=32/tan((double)view.projection.fov*.5);projected_left=32-focal/4;
    CHECK(projected_left>18 && projected_left<19 && 32+focal/4>45 && 32+focal/4<46,"independent pinhole bounds establish interior/exterior pixel controls");
    for(mode=0;mode<5;++mode){expected=mode==1?0xff0000:mode==2?0x00ff00:mode==4?0:0x0000ff;
        CHECK(bh_model_draw_context_init(&draw,&model,&cache,&camera,&clip,mode+1,mode==4?0:0xffffffffUL,0xff0000ffUL,bh_native_primitive_emit,&native),"source traversal binds REAL native ring emitter and fresh integer epoch");
        CHECK(bh_scene_view_begin(&view),"original native zeroZ/GREATEREQUAL profile begins actual scene");prior=native.polygons;
        if(mode==1 || mode==2 || mode==3){CHECK(bh_light_configure(&directional,mode!=3,0,0,1,1,0,mode==1?0xff0000:0x00ff00,0),"real directional config positive-control color/enable change");
            CHECK(bh_model_draw_lit_context_init(&lit,&draw,&lights,&directional,&orientation,mode==3,1,0xff000000UL) && bh_model_draw_lit(&lit,0,63),"complete lit BSP owner evaluates real lights then actual face/clip/native owners");}
        else CHECK(bh_model_draw_unlit(&draw,0,63),"complete unlit BSP/face/list/cache/clip/native source composition");
        queued=image->vertex_count;
        CHECK(draw.faces_marked==1 && native.polygons==prior+(mode==4?0:1) && image->run_count==(mode==4?0:1) && queued==(mode==4?0:3),"actual queue publication follows original traversal and texture-mask filter");
        if(mode!=4)for(i=0;i<3;++i)CHECK(image->vertices[i].sz==.25f && image->vertices[i].rhw==.25f && (image->vertices[i].color&0xffffff)==expected,"actual native TLqueue stores exact near/z,rhw and independently expected uniform color");
        CHECK(bh_render_mode(&state,0) && !image->run_count && !image->vertex_count,"real original mode barrier/residency consumes queue via DrawPrimitive");
        CHECK(bh_scene_view_end(&view,names[mode]),"actual native bitmap readback artifact");
        CHECK(pixel(names[mode],32,28)==expected && pixel(names[mode],32,32)==expected,"independent interior pixels prove actual unlit/directional/point colors and mask0 blanking");
        CHECK(pixel(names[mode],4,4)==0 && pixel(names[mode],55,55)==0,"independent exterior pixels remain black");
        CHECK(mode==4?!view.pixels:(view.pixels>300 && view.pixels<500),"actual raster area agrees with projected triangle footprint or mask0 empty target");
    }
    CHECK(pixel(names[0],32,28)!=pixel(names[1],32,28) && pixel(names[1],32,28)!=pixel(names[2],32,28),"positive lighting control changes actual native pixels without replacing geometry");
done:
    bh_model_draw_cache_dispose(&cache);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"complete native model composition retains callerPC53 profile");bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"model-draw-native\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_MODEL_DRAW_NATIVE_STANDALONE
int main(void){unsigned long count;return bh_model_draw_native_checkpoint(&count);}
#endif
