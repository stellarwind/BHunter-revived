#include "../src/bh_screen_quad.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures,shade;static BHClipVertex corners[12];static unsigned long count_vertices;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL screen-quad: %s\n",s);}}while(0)
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user){BHClipVertex *p=ring;count_vertices=0;shade=color;do{if(count_vertices==12)return 0;corners[count_vertices++]=*p;p=p->next;}while(p!=ring);return 1;}
static BHTextureImage *__cdecl lookup(unsigned long h,void *u){return h==17?(BHTextureImage *)u:NULL;}
static void pure(void){BHViewConfig view;BHScreenClipContext clip;BHScreenQuadContext c;BHTextureImage image;BHDrawQueue q;BHScreenQuadSubmit submit;unsigned long next,flags;
    memset(&view,0,sizeof(view));view.left=view.top=0;view.right=view.bottom=64;view.near_z=.125f;memset(&c,0,sizeof(c));memset(&image,0,sizeof(image));c.view=&view;c.clip=&clip;c.emit=capture;c.base_color=0x80000000UL;
    CHECK(bh_screen_clip_init(&clip,.125f,100,0,64,0,64) && bh_draw_queue_init(&q,12,NULL),"real screen clip/queue owners");submit.draw=&c;submit.lookup=lookup;submit.lookup_user=&image;
    for(flags=0;flags<8;++flags){bh_draw_queue_begin(&q);CHECK(bh_screen_quad_queue(&q,8,17,32,32,40,40,0,flags|0x100,.5f)==1 && q.commands[9].words[6]==(flags|0x100),"whole raw screen producer payload including flags and missed modulation");
        CHECK(bh_screen_quad_execute(&submit,q.commands+9,&next) && next==BH_DRAW_END && count_vertices==4 && shade==0x807f7f7fUL,"consumer truncates flags and invokes full real owner");
        CHECK(corners[0].sx==12 && corners[0].sy==12 && corners[2].sx==52 && corners[2].sy==52 && fabs(corners[0].point.z-.135)<1e-7,"independent corners and mutable non-one near plus.01");
        CHECK(corners[0].u==((flags&1)?1:0) && corners[0].v==((flags&2)?1:0) && corners[1].u==((flags&4)?((flags&1)?0:1):((flags&1)?1:0)),"all UV flips and quarter-turn selectors");}
    CHECK(bh_screen_quad_draw(&c,&image,32,32,40,40,1.5707963267948966f,0,1) && fabs(corners[0].sx-52)<.0001 && fabs(corners[0].sy-12)<.0001 && fabs(corners[1].sx-12)<.0001,"real rotated branch quarter turn geometry");
    CHECK(bh_screen_quad_draw(&c,&image,-100,32,4,4,0,0,1) && c.culled && !c.emitted,"strict radius cull");
    CHECK(bh_screen_quad_draw(&c,&image,0,32,40,40,0,0,1) && c.emitted && count_vertices==4,"real partial screen clipping");
    bh_draw_queue_begin(&q);CHECK(bh_screen_quad_queue(&q,99,17,1,2,3,4,0,0,1)==1 && q.commands[0].next==9,"original out-of-range bucket uses header zero");
    CHECK(bh_screen_quad_queue(&q,8,17,1,2,3,4,0,0,1)==1 && bh_screen_quad_queue(&q,8,17,1,2,3,4,0,0,1)==1 && !bh_screen_quad_queue(&q,8,17,1,2,3,4,0,0,1),"full queue silently drops producer");
    q.commands[9].resource=99;CHECK(!bh_screen_quad_execute(&submit,q.commands+9,&next) && next==1,"missing image reports failure and publishes next");bh_draw_queue_dispose(&q);
}
static unsigned long pixel(const char *path,unsigned long x,unsigned long y){FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned char b[3];unsigned long stride;if(!f)return 0xffffffffUL;
    if(fread(&h,sizeof(h),1,f)!=1 || fread(&info,sizeof(info),1,f)!=1 || info.biBitCount!=24){fclose(f);return 0xffffffffUL;}stride=(info.biWidth*3+3)&~3UL;if(fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET) || fread(b,1,3,f)!=3){fclose(f);return 0xffffffffUL;}fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];}
static void native_test(void){BHSceneView view;BHTextureContext textures;BHTextureImage *image=NULL;BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;BHScreenClipContext clip;BHScreenQuadContext c;BHDrawQueue q;BHScreenQuadSubmit submit;BHPcx pcx;unsigned char pixels[1024];unsigned long x,y,flags,tile,uu,vv,next;char name[80];const unsigned long colors[4]={0xff0000UL,0x00ff00UL,0x0000ffUL,0xffff00UL};
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&native,0,sizeof(native));memset(&q,0,sizeof(q));
    CHECK(bh_scene_view_init(&view,64,64),"actual native target");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"real native texture owners");textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;for(y=0;y<32;++y)for(x=0;x<32;++x)pixels[y*32+x]=(unsigned char)(1+(x>=16)+2*(y>=16));pcx.palette[3]=255;pcx.palette[7]=255;pcx.palette[11]=255;pcx.palette[12]=pcx.palette[13]=255;
    image=bh_texture_stage(&textures,"SCREENQUAD4COLORS.PCX",&pcx,0,0);CHECK(image && image->source && image->palette,"actual multicolor full-image texture");if(!image)goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"real native material/state");view.original_depth=1;
    CHECK(bh_view_config_init(&view.projection,64,64) && bh_main_camera_settings(&view.projection,60,.125,100) && bh_screen_clip_init(&clip,.125f,100,0,64,0,64) && bh_draw_queue_init(&q,10,NULL),"actual non-one near and screen/queue definitions");
    memset(&c,0,sizeof(c));c.view=&view.projection;c.clip=&clip;c.emit=bh_native_primitive_emit;c.emit_user=&native;c.base_color=0x80000000UL;native.textures=&textures;native.near_z=.125f;submit.draw=&c;submit.lookup=lookup;submit.lookup_user=image;
    for(flags=0;flags<9;++flags){sprintf(name,"screen-quad-native-%lu.bmp",flags);bh_draw_queue_begin(&q);
        CHECK(bh_screen_quad_queue(&q,8,17,32,32,40,40,flags==8?1.5707963267948966f:0,flags==8?0:flags,1)==1 && bh_scene_view_begin(&view) && bh_screen_quad_execute(&submit,q.commands+9,&next),"real full screen producer/consumer/native source chain");
        CHECK(image->vertex_count==4 && image->run_count==1 && fabs(image->vertices[0].rhw-1.0/(double)(float).135)<.00001 && fabs(image->vertices[0].sz-.125/(double)(float).135)<.00001,"actual TL depth is live near plus.01, reciprocal and native near/z are distinct");
        CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,name),"real queue flush and pixel readback");
        for(y=0;y<2;++y)for(x=0;x<2;++x){uu=(flags&4)?y:x;vv=(flags&4)?x:y;if(flags&1)uu=1-uu;if(flags&2)vv=1-vv;tile=uu+2*vv;if(flags==8)tile=x?(y?1:0):(y?3:2);
            CHECK(pixel(name,x?42:22,y?42:22)==colors[tile],"independent four-color UV/rotation pixels");}
        CHECK(pixel(name,4,4)==0,"outside remains blank");}
done:bh_draw_queue_dispose(&q);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
}
int bh_screen_quad_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;pure();native_test();CHECK(bh_math_environment_supported(),"PC53 retained");bh_math_environment_leave(&fp);if(count)*count=checks;printf("{\"scenario\":\"screen-quad\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;}
#ifdef BH_SCREEN_QUAD_STANDALONE
int main(void){unsigned long count;return bh_screen_quad_checkpoint(&count);}
#endif
