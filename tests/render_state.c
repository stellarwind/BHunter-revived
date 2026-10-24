#include "../src/bh_render_state.h"
#include "../src/bh_texture_draw.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <string.h>
#include <stdio.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<15)printf("FAIL render state %d: %s\n",line,label);}}
static const unsigned long modes[5][4]={{0,0,2,1},{1,1,2,4},{1,1,1,4},{1,1,2,2},{1,1,5,6}};
static void software(void) {
    BHRenderState s,saved;BHTextureContext q;BHTextureImage im;unsigned long i;
    memset(&q,0,sizeof(q));q.software=1;q.frame=17;memset(&im,0,sizeof(im));im.next=&im;im.run_count=7;q.images=&im;
    CHECK(bh_render_state_init(&s,&q) && s.depth==0 && s.blend==0 && s.source==2 && s.destination==1 && !s.native_calls,"software explicit binding applies defaults without native definitions");
    for(i=0;i<5;++i) {
        CHECK(bh_render_mode(&s,i) && s.mode==i && s.depth==modes[i][0] && s.blend==modes[i][1] && s.source==modes[i][2] && s.destination==modes[i][3],"all five original software mode/cache combinations");
        CHECK(q.frame==17 && im.run_count==7 && !s.native_calls && !s.barriers,"software mode leaves even irrelevant pending/native list untouched and epoch unchanged");
    }
    saved=s;CHECK(bh_render_mode(&s,0xffffffffUL) && s.mode==0xffffffffUL && s.depth==saved.depth && s.blend==saved.blend && s.source==saved.source && s.destination==saved.destination,"unknown software mode only publishes mode");
    CHECK(bh_render_depth(&s,0xffffffffUL) && s.depth==0xffffffffUL && !s.native_calls,"unknown depth retained as raw cache word");
    CHECK(bh_render_blend(&s,17,0x80000001UL,0xffffffffUL) && s.blend==17 && s.source==0x80000001UL && s.destination==0xffffffffUL,"software blend retains full raw parameters");
    saved=s;CHECK(!bh_render_depth(NULL,0) && !bh_render_mode(NULL,0) && !bh_render_blend(NULL,0,2,1),"null context guards");
    q.software=0;CHECK(!bh_render_mode(&s,0) && !memcmp(&s,&saved,sizeof(s)),"missing native device rejects without cache mutation");
}
static int state(BHSceneView *v,D3DRENDERSTATETYPE kind,unsigned long expected) {
    DWORD value=0xffffffffUL;return IDirect3DDevice2_GetRenderState(v->device,kind,&value)==DD_OK && value==expected;
}
static unsigned long pixel(const char *name,unsigned long x,unsigned long y) {
    FILE *f=fopen(name,"rb");unsigned char b[3];BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned long stride;
    if(!f)return 0xffffffffUL;fread(&h,sizeof(h),1,f);fread(&info,sizeof(info),1,f);stride=(info.biWidth*3+3)&~3UL;
    fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET);fread(b,1,3,f);fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
static D3DTLVERTEX vertex(float x,float y,unsigned long color) {
    D3DTLVERTEX v;memset(&v,0,sizeof(v));v.sx=x;v.sy=y;v.sz=1;v.rhw=1;v.color=color;v.tu=v.tv=.5f;return v;
}
static void native_controls(BHSceneView *view) {
    BHRenderState s,saved;BHTextureContext q;BHTextureImage *im;BHPcx pcx;unsigned char pixels[1024];D3DTLVERTEX triangle[3];unsigned long i,calls,barriers;
    memset(&q,0,sizeof(q));CHECK(bh_texture_context_init(&q,view->draw,view->device,NULL) && bh_texture_formats(&q),"actual native texture context bound to recovered state owner");
    q.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    CHECK(bh_render_state_init(&s,&q) && s.native_calls==5 && !s.failures,"binding initializes actual state/cache agreement through real native calls");
    CHECK(state(view,D3DRENDERSTATE_ZWRITEENABLE,1) && state(view,D3DRENDERSTATE_ZFUNC,7) && state(view,D3DRENDERSTATE_ALPHABLENDENABLE,0) && state(view,D3DRENDERSTATE_SRCBLEND,2) && state(view,D3DRENDERSTATE_DESTBLEND,1),"native opaque defaults query correctly");
    calls=s.native_calls;CHECK(bh_render_depth(&s,0) && bh_render_blend(&s,0,2,1) && s.native_calls==calls,"unchanged cache suppresses native setter calls");
    for(i=0;i<3;++i) {
        CHECK(bh_render_depth(&s,i) && state(view,D3DRENDERSTATE_ZWRITEENABLE,i!=1) && state(view,D3DRENDERSTATE_ZFUNC,i==2?8:7),"three original native depth write/compare modes");
    }
    calls=s.native_calls;CHECK(bh_render_depth(&s,99) && s.depth==99 && s.native_calls==calls && state(view,D3DRENDERSTATE_ZFUNC,8),"unknown native depth updates cache without changing device");
    CHECK(bh_render_depth(&s,0),"restore opaque native depth");
    CHECK(IDirect3DDevice2_SetRenderState(view->device,D3DRENDERSTATE_ZWRITEENABLE,0)==DD_OK && bh_render_depth(&s,0) && state(view,D3DRENDERSTATE_ZWRITEENABLE,0),"original cache trusts exclusive owner and does not repair external state changes");
    CHECK(bh_render_depth(&s,1) && bh_render_depth(&s,0),"explicit cache transition restores device agreement");
    CHECK(bh_scene_view_begin(view),"native mode barrier starts from actual begun scene");
    for(i=0;i<5;++i) {
        barriers=s.barriers;calls=s.native_calls;
        CHECK(bh_render_mode(&s,i) && s.mode==i && s.barriers==barriers+1 && q.frame==(long)s.barriers && s.native_calls>=calls+2,"native mode always advances shared residency epoch and ends/begins scene");
        CHECK(state(view,D3DRENDERSTATE_ZWRITEENABLE,i==0) && state(view,D3DRENDERSTATE_ZFUNC,7) && state(view,D3DRENDERSTATE_ALPHABLENDENABLE,modes[i][1]) && state(view,D3DRENDERSTATE_SRCBLEND,modes[i][2]) && state(view,D3DRENDERSTATE_DESTBLEND,modes[i][3]),"native device state confirms all five independent expected mode combinations");
        calls=s.native_calls;barriers=s.barriers;CHECK(bh_render_mode(&s,i) && s.barriers==barriers+1 && s.native_calls==calls+2,"same-mode call still barriers but suppresses unchanged five native state setters");
    }
    saved=s;calls=s.native_calls;CHECK(bh_render_mode(&s,99) && s.mode==99 && s.depth==saved.depth && s.blend==saved.blend && s.source==saved.source && s.destination==saved.destination && s.native_calls==calls+2,"unknown native mode retains render caches after mandatory barrier");
    q.frame=0x7fffffffL;CHECK(bh_render_mode(&s,99) && (unsigned long)q.frame==0x80000000UL,"residency epoch preserves original32-bit wrap without signed C overflow");
    CHECK(bh_scene_view_end(view,"render-state-barrier.bmp"),"original barriers leave an actually open native scene");
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
    im=bh_texture_stage(&q,"STATEWHITE.PCX",&pcx,0,0);CHECK(im && bh_texture_slot_create(&q,im->format),"real source image/native residency for pending mode barrier");
    if(im) {
        CHECK(bh_render_depth(&s,0) && bh_render_blend(&s,0,2,1) && bh_scene_view_begin(view),"opaque queue fixture begins");
        triangle[0]=vertex(4,4,0xffff0000UL);triangle[1]=vertex(28,4,0xffff0000UL);triangle[2]=vertex(4,28,0xffff0000UL);
        CHECK(bh_texture_draw_polygon(&q,im,triangle,3) && im->run_count==1,"pending fan waits for original mode barrier");
        calls=q.pixel_uploads;CHECK(bh_render_mode(&s,4) && !im->run_count && !im->vertex_count && q.pixel_uploads==calls+1 && im->resident && im->resident->frame==q.frame,"mode flushes actual texture queue under new residency epoch before switching blend/depth");
        CHECK(bh_scene_view_end(view,"render-state-order.bmp") && pixel("render-state-order.bmp",8,8)==0xff0000 && pixel("render-state-order.bmp",24,24)==0,"native pixels prove queued fan was consumed before alpha-mode switch");
        CHECK(bh_texture_draw_polygon(&q,im,triangle,3),"pending queue for software gate");q.software=1;calls=q.frame;
        CHECK(bh_render_mode(&s,3) && im->run_count==1 && im->vertex_count==3 && (unsigned long)q.frame==calls,"software gate retains actual owned native queue and epoch");q.software=0;
        im->next=im;saved=s;calls=q.frame;CHECK(!bh_render_mode(&s,0) && !memcmp(&s,&saved,sizeof(s)) && (unsigned long)q.frame==calls && im->run_count==1,"cyclic native image list is rejected before any mutation");im->next=NULL;
        CHECK(bh_scene_view_begin(view) && bh_render_mode(&s,0) && bh_scene_view_end(view,"render-state-barrier.bmp"),"source guard repair retries original flush path");
    }
    bh_texture_context_dispose(&q);
}
int bh_render_state_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;BHSceneView view;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    software();memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real native device dependency");
    if(view.device)native_controls(&view);bh_scene_view_dispose(&view);CHECK(bh_math_environment_supported(),"state family retains caller numerical profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"render-state\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
