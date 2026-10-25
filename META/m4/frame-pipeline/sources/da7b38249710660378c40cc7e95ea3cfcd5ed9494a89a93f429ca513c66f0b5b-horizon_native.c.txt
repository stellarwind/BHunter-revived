#include "../src/bh_horizon_queue.h"
#include "../src/bh_render_material.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL horizon-native: %s\n",s);}}while(0)
static BHTextureImage *__cdecl lookup(unsigned long handle,void *user){return handle==42?(BHTextureImage *)user:NULL;}
static unsigned long pixel(const char *path,unsigned long x,unsigned long y) {
    FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned char b[3];unsigned long stride;
    if(!f)return 0xffffffffUL;
    if(fread(&h,sizeof(h),1,f)!=1 || fread(&info,sizeof(info),1,f)!=1 || info.biBitCount!=24){fclose(f);return 0xffffffffUL;}
    stride=(info.biWidth*3+3)&~3UL;if(fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET) || fread(b,1,3,f)!=3){fclose(f);return 0xffffffffUL;}
    fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
int bh_horizon_native_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHTextureImage *image=NULL;BHRenderMaterial material;BHRenderState state;
    BHMainCamera camera;BHAttributeClipContext clip;BHHorizonContext horizon;BHHorizonNative native;BHDrawQueue queue;BHHorizonQueueContext binding;
    BHPcx pcx;unsigned char pixels[1024];BHClipVertex ring[3];BHMathEnvironment fp;float alpha;unsigned long i,variant,next,sky[2],edge[2],epoch,states;
    const char *names[2]={"horizon-native-brown.bmp","horizon-native-blue.bmp"};
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&camera,0,sizeof(camera));memset(&native,0,sizeof(native));memset(&queue,0,sizeof(queue));
    CHECK(bh_scene_view_init(&view,64,64),"actual offscreen native scene");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"real native texture context and formats");
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
    image=bh_texture_stage(&textures,"HORIZONWHITE.PCX",&pcx,0,0);CHECK(image && image->source && image->palette,"actual white image owner");if(!image)goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"actual material and state definitions");
    native.textures=&textures;native.near_z=1;memset(ring,0,sizeof(ring));
    for(i=0;i<3;++i){ring[i].point.z=2;ring[i].u=ring[i].v=.5f;ring[i].next=ring+(i+1)%3;ring[i].previous=ring+(i+2)%3;alpha=i*.5f;memcpy(&ring[i].unknown28,&alpha,4);}
    ring[0].sx=8;ring[0].sy=8;ring[1].sx=8;ring[1].sy=28;ring[2].sx=28;ring[2].sy=8;
    CHECK(bh_horizon_native_emit(ring,image,1,NULL,&native) && image->vertex_count==3 && image->run_count==1,"native gradient adapter real TL queue composition");
    for(i=0;i<3;++i)CHECK(image->vertices[i].sz==.5f && image->vertices[i].rhw==.5f && image->vertices[i].color==0xffffffUL &&
        image->vertices[i].specular==(i==0?0xff000000UL:i==1?0x80000000UL:0),"baseline near1 and exact truncating specular fog alpha including midpoint");
    textures.software=1;
    CHECK(!bh_horizon_native_emit(ring,image,1,NULL,&native) && image->vertex_count==3,"native-only adapter refuses software dispatch without consuming input");textures.software=0;
    CHECK(!bh_horizon_native_emit(ring,image,0,NULL,&native) && image->vertex_count==3,"plain software selection cannot silently use gradient native adapter");
    image->vertex_count=image->run_count=image->run_serial=0;native.polygons=native.vertices=0;
    native.near_z=.125f;
    CHECK(bh_horizon_native_emit(ring,image,1,NULL,&native) && image->vertices[0].sz==.0625f && image->vertices[0].rhw==.5f,"live non-one near numerator changes depth but preserves reciprocal W");
    native.near_z=1;image->vertex_count=image->run_count=image->run_serial=0;native.polygons=native.vertices=0;
    CHECK(bh_view_config_init(&view.projection,64,64) && bh_main_camera_settings(&view.projection,60,1,100) &&
        bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0) && bh_attribute_clip_init(&clip,1,100,0,64,0,64),"real main camera and full attribute clipper");view.original_depth=1;
    CHECK(bh_horizon_init(&horizon,&camera,&clip,&state,NULL,bh_horizon_native_emit,&native) && bh_draw_queue_init(&queue,10,NULL),"real horizon and command storage bindings");
    binding.draw=&horizon;binding.lookup=lookup;binding.lookup_user=image;
    for(variant=0;variant<2;++variant){
        CHECK(bh_draw_queue_begin(&queue) && bh_horizon_queue(&queue,42,50,5,25,12.5f,variant?0x0000ff:0x804000)==1,"complete horizon producer retains all sparse parameters");
        CHECK(bh_scene_view_begin(&view),"actual native scene begin");epoch=textures.frame;states=state.native_calls;next=99;
        CHECK(bh_horizon_queue_execute(&binding,&queue,queue.commands+9,&next) && next==1 && horizon.attempted==8 && horizon.emitted>0 && horizon.emitted<=8,"full consumer barrier and horizon source with real gradient leaf");
        CHECK((unsigned long)textures.frame==epoch+2 && !image->vertex_count && !image->run_count && state.depth==0 && state.native_calls>states,"pre-command and final horizon barriers flush real queues and restore depth");
        CHECK(bh_scene_view_end(&view,names[variant]),"native horizon readback");sky[variant]=pixel(names[variant],32,8);edge[variant]=pixel(names[variant],32,30);
        CHECK(sky[variant]==0xffffffUL && pixel(names[variant],32,45)==0,"independent sky interior and below-horizon exterior pixels");
        CHECK(edge[variant]!=0 && edge[variant]!=0xffffffUL,"outer gradient produces actual fog-colored pixels");
    }
    CHECK(sky[0]==sky[1] && edge[0]!=edge[1] && ((edge[0]>>16)&255)>(edge[0]&255) && (edge[1]&255)>((edge[1]>>16)&255),"fog color positive control changes actual horizon edge while white sky remains");
done:
    bh_draw_queue_dispose(&queue);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"PC53 profile retained through native horizon");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"horizon-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_HORIZON_NATIVE_STANDALONE
int main(void){unsigned long count;return bh_horizon_native_checkpoint(&count);}
#endif
