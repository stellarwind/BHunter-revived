#include "../src/bh_queue_reserve.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_texture_draw.h"
#include "../src/bh_render_material.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL queue-reserve: %s\n",s);}}while(0)
static void reserve(void){BHDrawQueue q;BHDrawCommand *p,before,all[12];unsigned long i,header;long selectors[13]={0,1,2,3,4,5,6,7,8,-1,9,2147483647L,(-2147483647L-1L)};const unsigned long mapping[9]={1,2,3,4,5,0,6,7,8};
    CHECK(bh_draw_queue_init(&q,12,NULL),"real owned queue storage");
    for(i=0;i<13;++i){bh_draw_queue_begin(&q);memset(q.commands+9,0xab,sizeof(BHDrawCommand));before=q.commands[9];header=i<9?mapping[i]:0;
        p=bh_queue_reserve(&q,selectors[i]);CHECK(p==q.commands+9 && q.count==10 && q.commands[header].resource==9 && q.commands[header].next==9 && p->next==(header==8?BH_DRAW_END:header+1),"all nine jump-table selectors and invalid/negative default routing");
        CHECK(!memcmp(&before,p,sizeof(BHDrawCommand)-4),"reservation leaves kind resource and all18 payload words untouched");
        CHECK(bh_queue_reserve(&q,selectors[i])==q.commands+10 && q.commands[9].next==10 && q.commands[10].next==(header==8?BH_DRAW_END:header+1),"same-bucket append preserves FIFO successor");}
    CHECK(bh_queue_reserve(&q,0)==q.commands+11,"last available slot");memcpy(all,q.commands,sizeof(all));
    CHECK(!bh_queue_reserve(&q,-1) && q.error==BH_DRAW_FULL && q.count==12 && !memcmp(all,q.commands,sizeof(all)),"full reservation changes no command or count");
    bh_draw_queue_begin(&q);q.commands[1].resource=99;memcpy(all,q.commands,sizeof(all));
    CHECK(!bh_queue_reserve(&q,0) && q.error==BH_DRAW_TOPOLOGY && q.count==9 && !memcmp(all,q.commands,sizeof(all)),"bad tail cannot consume a slot");
    q.commands[1].resource=1;q.commands[1].next=99;CHECK(!bh_queue_reserve(&q,0) && q.count==9 && q.error==BH_DRAW_TOPOLOGY,"bad successor rejected");
    q.count=8;CHECK(!bh_queue_reserve(&q,0) && q.error==BH_DRAW_ARGUMENT,"missing headers rejected");CHECK(!bh_queue_reserve(NULL,0),"missing queue rejected");bh_draw_queue_dispose(&q);
}
static void modes(void){BHDrawQueue q;BHTextureContext textures;BHRenderState state;BHDrawCommand before;unsigned long i,next;const unsigned long expected[5][4]={{0,0,2,1},{1,1,2,4},{1,1,1,4},{1,1,2,2},{1,1,5,6}};
    memset(&textures,0,sizeof(textures));textures.software=1;textures.frame=41;CHECK(bh_draw_queue_init(&q,20,NULL) && bh_render_state_init(&state,&textures),"actual source software render-state owner");
    memset(q.commands+9,0xcd,sizeof(BHDrawCommand));before=q.commands[9];
    CHECK(bh_queue_mode(&q,8,4)==1 && q.commands[9].kind==BH_DRAW_STATE && q.commands[9].resource==before.resource && q.commands[9].words[0]==4 && !memcmp(q.commands[9].words+1,before.words+1,17*4),"source adapter initializes only selector and original+08 mode word");
    for(i=0;i<5;++i){q.commands[9].words[0]=i;q.commands[9].next=100+i;next=99;
        CHECK(bh_queue_mode_execute(&state,q.commands+9,&next) && next==100+i && state.mode==i && state.depth==expected[i][0] && state.blend==expected[i][1] && state.source==expected[i][2] && state.destination==expected[i][3] && textures.frame==41,"actual mode owner all five cases then next read");}
    q.commands[9].words[0]=0xffffffffUL;CHECK(bh_queue_mode_execute(&state,q.commands+9,&next) && state.mode==0xffffffffUL && next==104,"unknown mode is forwarded raw");
    state.textures=NULL;q.commands[9].next=123;next=99;CHECK(!bh_queue_mode_execute(&state,q.commands+9,&next) && next==123,"real owner failure still forwards current successor");
    CHECK(!bh_queue_mode_execute(NULL,q.commands+9,&next) && !bh_queue_mode_execute(&state,NULL,&next) && !bh_queue_mode_execute(&state,q.commands+9,NULL),"consumer binding guards");bh_draw_queue_dispose(&q);
}
static unsigned long pixel(const char *path,unsigned long x,unsigned long y){FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned char b[3];unsigned long stride;if(!f)return 0xffffffffUL;if(fread(&h,sizeof(h),1,f)!=1 || fread(&info,sizeof(info),1,f)!=1){fclose(f);return 0xffffffffUL;}stride=(info.biWidth*3+3)&~3UL;fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET);if(fread(b,1,3,f)!=3){fclose(f);return 0xffffffffUL;}fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];}
static void native_test(void){BHSceneView view;BHTextureContext textures;BHTextureImage *image=NULL;BHRenderState state;BHRenderMaterial material;BHDrawQueue q;BHPcx pcx;unsigned char pixels[1024];D3DTLVERTEX tri[3];unsigned long i,next,epoch;DWORD value;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&q,0,sizeof(q));
    CHECK(bh_scene_view_init(&view,64,64),"actual native offscreen scene");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"real texture/cache definitions");textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
    image=bh_texture_stage(&textures,"RESERVEWHITE.PCX",&pcx,0,0);CHECK(image && bh_texture_slot_create(&textures,image->format),"actual image and native resident slot");if(!image)goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures) && bh_draw_queue_init(&q,10,NULL),"actual material state and queue owners");view.original_depth=1;
    memset(tri,0,sizeof(tri));for(i=0;i<3;++i){tri[i].sz=tri[i].rhw=1;tri[i].color=0xffff0000UL;tri[i].tu=tri[i].tv=.5f;}tri[0].sx=4;tri[0].sy=4;tri[1].sx=4;tri[1].sy=28;tri[2].sx=28;tri[2].sy=4;
    CHECK(bh_scene_view_begin(&view) && bh_texture_draw_polygon(&textures,image,tri,3) && image->run_count==1,"actual pending native triangle fan");epoch=textures.frame;
    CHECK(bh_queue_mode(&q,8,4)==1 && bh_queue_mode_execute(&state,q.commands+9,&next) && next==BH_DRAW_END && state.mode==4 && (unsigned long)textures.frame==epoch+1 && !image->run_count && !image->vertex_count,"queued callback executes actual residency/flush/end-begin before mode switch");
    value=0;CHECK(IDirect3DDevice2_GetRenderState(view.device,D3DRENDERSTATE_ALPHABLENDENABLE,&value)==DD_OK && value==1,"actual native device reflects queued mode");
    CHECK(bh_scene_view_end(&view,"queue-reserve-native.bmp") && pixel("queue-reserve-native.bmp",8,8)==0xff0000 && !pixel("queue-reserve-native.bmp",24,24),"actual queued drawing flushed before blend transition with blank exterior control");
done:bh_draw_queue_dispose(&q);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
}
int bh_queue_reserve_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;reserve();modes();native_test();CHECK(bh_math_environment_supported(),"PC53 retained");bh_math_environment_leave(&fp);if(count)*count=checks;printf("{\"scenario\":\"queue-reserve\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;}
#ifdef BH_QUEUE_RESERVE_STANDALONE
int main(void){unsigned long count;return bh_queue_reserve_checkpoint(&count);}
#endif
