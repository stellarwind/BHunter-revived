#include "../src/bh_native_primitive.h"
#include "../src/bh_render_state.h"
#include "../src/bh_render_material.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<12)printf("FAIL native primitive %d: %s\n",line,label);}}
static unsigned long pixel(const char *name,unsigned long x,unsigned long y) {
    FILE *f=fopen(name,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned char b[3];unsigned long stride;
    if(!f)return 0xffffffffUL;fread(&h,sizeof(h),1,f);fread(&info,sizeof(info),1,f);stride=(info.biWidth*3+3)&~3UL;
    fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET);fread(b,1,3,f);fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
static void ring(BHClipVertex *v,float z) {
    unsigned long i;memset(v,0,3*sizeof(*v));
    v[0].sx=8;v[0].sy=8;v[1].sx=8;v[1].sy=28;v[2].sx=28;v[2].sy=8;
    for(i=0;i<3;++i){v[i].point.z=z;v[i].u=.5f;v[i].v=.5f;v[i].next=v+(i+1)%3;v[i].previous=v+(i+2)%3;}
}
int bh_native_primitive_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext q;BHTextureImage *im=NULL;BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;
    BHMathEnvironment fp;BHPcx pcx;unsigned char pixels[1024];BHClipVertex vertices[3];unsigned long i,prior;
    BHVec3 near_triangle[3]={{-.5f,-.5f,2},{0,.5f,2},{.5f,-.5f,2}};
    BHVec3 far_triangle[3]={{-1,-1,4},{0,1,4},{1,-1,4}};
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&q,0,sizeof(q));memset(&material,0,sizeof(material));memset(&native,0,sizeof(native));
    CHECK(bh_scene_view_init(&view,64,64),"real source offscreen native dependency");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&q,view.draw,view.device,NULL) && bh_texture_formats(&q),"real texture context/format owners");q.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
    im=bh_texture_stage(&q,"NATIVEWHITE.PCX",&pcx,0,0);CHECK(im && im->source && im->palette,"real native white image");if(!im)goto done;
    CHECK(!bh_render_material_initialize(&material,&q,view.d3d) && !material.failures && bh_render_state_init(&state,&q),"original material/default state owners compose with actual native device");
    CHECK(bh_view_config_init(&view.projection,64,64),"original near1/60-degree source config");view.original_depth=1;
    native.textures=&q;native.near_z=view.projection.near_z;ring(vertices,2);
    CHECK(bh_native_primitive_emit(vertices,im,0x80402010UL,&native) && im->run_count==1 && im->runs[0]==3 && im->vertex_count==3,"native-only ring adapter composes real original vertex-growth queue");
    for(i=0;i<3;++i)CHECK(im->vertices[i].sx==vertices[i].sx && im->vertices[i].sy==vertices[i].sy && im->vertices[i].sz==.5f && im->vertices[i].rhw==.5f &&
        im->vertices[i].color==0x80402010UL && !im->vertices[i].specular && im->vertices[i].tu==.5f && im->vertices[i].tv==.5f,"all32TLbytes follow original native branch incl raw alpha and zero specular");
    q.software=1;prior=im->vertex_count;CHECK(!bh_native_primitive_emit(vertices,im,0,&native) && q.error==BH_TEXTURE_BACKEND && im->vertex_count==prior,"software is explicitly unsupported here and does not consume rings/queues");q.software=0;q.error=BH_TEXTURE_OK;
    vertices[1].previous=vertices+2;CHECK(!bh_native_primitive_emit(vertices,im,0,&native) && q.error==BH_TEXTURE_TOPOLOGY && im->vertex_count==prior,"malformed source topology fails before queue mutation");ring(vertices,2);q.error=BH_TEXTURE_OK;
    vertices[1].point.z=0;CHECK(!bh_native_primitive_emit(vertices,im,0,&native) && im->vertex_count==prior,"zero depth rejects before division and queue publication");ring(vertices,2);q.error=BH_TEXTURE_OK;
    im->vertex_count=im->run_count=im->run_serial=0;
    for(i=0;i<2;++i) {
        CHECK(bh_scene_view_begin(&view),"original native profile clears Z tozero before BeginScene");
        ring(vertices,i?4:2);CHECK(bh_native_primitive_emit(vertices,im,i?0xff0000ffUL:0xffff0000UL,&native),"first ordered native depth fan");
        ring(vertices,i?2:4);CHECK(bh_native_primitive_emit(vertices,im,i?0xffff0000UL:0xff0000ffUL,&native),"second overlapping fan uses same exact native near/z mapping");
        CHECK(bh_render_mode(&state,0) && !im->run_count,"original mode barrier flushes queued native fans");
        CHECK(bh_scene_view_end(&view,"native-reversed-depth.bmp") && pixel("native-reversed-depth.bmp",12,12)==0xff0000 && pixel("native-reversed-depth.bmp",24,24)==0,"actual GEQUAL native depth preserves nearer red in both submission orders with CWcull/default material");
    }
    CHECK(bh_scene_view_begin(&view) && bh_scene_view_polygon(&view,near_triangle,3,0x00ff00) && bh_scene_view_polygon(&view,far_triangle,3,0xff0000),"recovered projection/clipper and original native profile compose for source camera polygons");
    CHECK(bh_scene_view_end(&view,"native-projected-depth.bmp") && pixel("native-projected-depth.bmp",32,32)==0x00ff00,"native original depth/profile is exercised by existing source scene draw adapter");
done:
    bh_render_material_dispose(&material);bh_texture_context_dispose(&q);bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"native source composition retainsPC53 profile");bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"native-primitive-adapter\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
