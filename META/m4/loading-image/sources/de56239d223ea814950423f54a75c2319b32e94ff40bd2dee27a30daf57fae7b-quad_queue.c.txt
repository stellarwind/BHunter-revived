#include "../src/bh_quad_queue.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) check((x)!=0,s,__LINE__)
static void check(int ok,const char *s,int line){++checks;if(!ok){++failures;if(failures<20)printf("FAIL quad queue %d: %s\n",line,s);}}
static BHTextureImage *__cdecl lookup(unsigned long handle,void *user){return handle==17?(BHTextureImage *)user:NULL;}
static void payloads(void){BHDrawQueue q;BHVec3 p={1,2,3},up={4,5,6},n={7,8,9};unsigned long i,j,at,words[18],following;BHDrawCommand before;BHQuadQueueContext context;BHSpriteQuadContext draw;
    static const unsigned long headers[9]={1,2,3,4,5,0,6,7,8};float scalar[18]={1,2,3,4,5,6,7,8,9,10,11,12,14,0,15,16,17,18};
    CHECK(bh_draw_queue_init(&q,20,NULL),"real draw owner");memcpy(words,scalar,sizeof(words));words[13]=13;
    for(i=0;i<9;++i){CHECK(bh_draw_queue_begin(&q),"reset actual queue");
        CHECK(bh_quad_queue(&q,i,17,&p,&up,&n,10,11,12,13,14,15,16,17,18)==1,"each bucket producer");
        at=q.commands[headers[i]].next;CHECK(at==9 && q.commands[headers[i]].resource==at,"actual bucket selector mapping");
        CHECK(q.commands[at].kind==BH_DRAW_QUAD && q.commands[at].resource==17 && !memcmp(q.commands[at].words,words,sizeof(words)),"all nineteen payload words including swapped flags/modulation");
        for(j=0;j<9;++j)if(j!=headers[i])CHECK(q.commands[j].resource==j,"other tails preserved");
    }
    CHECK(bh_draw_queue_begin(&q),"reset full control");q.capacity=10;
    CHECK(bh_quad_queue(&q,0,17,&p,&up,&n,10,11,12,13,14,15,16,17,18)==1,"last slot accepted");before=q.commands[9];
    CHECK(bh_quad_queue(&q,0,18,&p,&up,&n,1,1,0,0,1,0,1,0,1)==0 && !memcmp(&before,q.commands+9,sizeof(before)) && q.count==10,"exhaustion silently drops without payload mutation");
    memset(&context,0,sizeof(context));memset(&draw,0,sizeof(draw));context.draw=&draw;context.lookup=lookup;
    q.commands[9].resource=999;q.commands[9].next=123;
    CHECK(!bh_quad_queue_execute(&context,q.commands+9,&following) && following==123,"unresolved source binding publishes next and fails");
    q.capacity=20;CHECK(bh_draw_queue_dispose(&q),"real queue lifetime cleanup");
}
static unsigned long pixel(const char *name,unsigned long x,unsigned long y){FILE *f=fopen(name,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned char b[3];unsigned long stride;if(!f)return 0xffffffffUL;
    if(fread(&h,sizeof(h),1,f)!=1 || fread(&info,sizeof(info),1,f)!=1 || info.biBitCount!=24){fclose(f);return 0xffffffffUL;}
    stride=(info.biWidth*3+3)&~3UL;if(fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET) || fread(b,1,3,f)!=3){fclose(f);return 0xffffffffUL;}
    fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
static void native(void){BHSceneView view;BHTextureContext textures;BHTextureImage *image=NULL;BHRenderMaterial material;BHRenderState state;BHNativePrimitive primitive;
    BHMainCamera camera;BHClipContext clip;BHLightConfig directional;BHSpriteQuadContext draw;BHQuadQueueContext context;BHDrawQueue queue;
    BHPcx pcx;unsigned char pixels[1024];BHVec3 p={0,0,4},up={0,1,0},normal={0,0,-1};unsigned long mode,next,i;int texture_live=0,queue_live=0;
    const char *names[]={"quad-native-white.bmp","quad-native-half.bmp","quad-native-back-blank.bmp","quad-native-back.bmp","quad-native-light.bmp"};
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&camera,0,sizeof(camera));memset(&primitive,0,sizeof(primitive));memset(&draw,0,sizeof(draw));memset(&directional,0,sizeof(directional));
    CHECK(bh_scene_view_init(&view,64,64),"real offscreen D3D scene");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL),"real texture owner");if(!textures.device)goto done;texture_live=1;textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    CHECK(bh_texture_formats(&textures),"native texture formats");memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
    image=bh_texture_stage(&textures,"QUADWHITE.PCX",&pcx,0,0);CHECK(image!=NULL,"owned texture");if(!image)goto done;image->flags=1;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && bh_render_state_init(&state,&textures),"real material/render barrier");
    CHECK(bh_view_config_init(&view.projection,64,64) && bh_main_camera_settings(&view.projection,60,1,100) && bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0),"recovered projection camera");view.original_depth=1;
    CHECK(bh_clip_init(&clip,1,100,0,64,0,64) && bh_clip_set_camera(&clip,&camera.projected),"real polygon clipper binding");
    primitive.textures=&textures;primitive.near_z=1;draw.camera=&camera;draw.clip=&clip;draw.directional=&directional;draw.emit=bh_native_primitive_emit;draw.emit_user=&primitive;draw.base_color=0xff000000UL;
    context.draw=&draw;context.lookup=lookup;context.lookup_user=image;
    CHECK(bh_draw_queue_init(&queue,12,NULL),"queue lifetime");if(!queue.commands)goto done;queue_live=1;
    for(mode=0;mode<5;++mode){unsigned long expected=mode==1?0x7f7f7fUL:mode==2?0:mode==4?0xff0000UL:0xffffffUL;
        normal.z=mode==2 || mode==3?1:-1;directional.enabled=mode==4;directional.direction.z=-1;directional.directional[0]=255;
        CHECK(bh_draw_queue_begin(&queue) && bh_scene_view_begin(&view),"begin source native composition");
        CHECK(bh_quad_queue(&queue,0,17,&p,&up,&normal,2,2,0,mode==3?0x10008UL:0x10000UL,mode==1?.5f:1,0,1,0,1)==1,"actual recovered producer");
        CHECK(bh_quad_queue_execute(&context,queue.commands+9,&next) && next==queue.commands[9].next,"real command consumer invokes quad owner with low-byte flags");
        CHECK(mode==2?!image->vertex_count:image->vertex_count>=4,"backface control or real TL vertices");
        for(i=0;i<image->vertex_count;++i)CHECK(image->vertices[i].sz==.25f && image->vertices[i].rhw==.25f && (image->vertices[i].color&0xffffffUL)==expected,"actual native queue depth and recovered lighting");
        CHECK(bh_render_mode(&state,0) && !image->vertex_count,"real native queue flush");CHECK(bh_scene_view_end(&view,names[mode]),"read actual native image");
        if(mode==1){unsigned long actual=pixel(names[mode],32,32);CHECK((actual&255)>=122 && (actual&255)<=132,"native half modulation quantization");}
        else CHECK(pixel(names[mode],32,32)==expected,"independent center-pixel positive and blank controls");
        CHECK(pixel(names[mode],2,2)==0 && (mode==2?!view.pixels:view.pixels>50),"bounded native geometry footprint");
    }
done:
    if(queue_live)bh_draw_queue_dispose(&queue);bh_render_material_dispose(&material);if(texture_live)bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
}
int bh_quad_queue_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;payloads();native();CHECK(bh_math_environment_supported(),"PC53 retained");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"quad-queue\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
