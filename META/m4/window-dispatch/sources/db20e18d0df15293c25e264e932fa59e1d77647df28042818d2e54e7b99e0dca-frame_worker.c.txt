#include "../src/bh_frame_worker.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL frame-worker: %s\n",s);}}while(0)
typedef struct Resume {BHFrameWorker *worker;long saw_free;HANDLE gate;} Resume;
typedef struct Hold {HANDLE mutex,ready;} Hold;
static DWORD WINAPI hold(void *user){Hold *h=(Hold*)user;DWORD wait=WaitForSingleObject(h->mutex,2000);
    if(wait==WAIT_OBJECT_0){SetEvent(h->ready);Sleep(40);ReleaseMutex(h->mutex);}return wait;}
static DWORD WINAPI resume(void *user){
    Resume *r=(Resume*)user;unsigned long i;
    WaitForSingleObject(r->gate,2000);Sleep(25);r->worker->ready=1;
    for(i=0;i<2000;++i){
        if(r->worker->rendered&&r->worker->selected>=0&&
           !r->worker->frames->slots[r->worker->selected].busy){r->saw_free=1;break;}
        Sleep(1);
    }
    r->worker->display_state=3;return 0;
}
int bh_frame_worker_checkpoint(unsigned long *count){
    BHMathEnvironment fp;BHFrameWorker worker;BHFrameBuffers frames;
    BHSceneView view;BHTextureContext textures;BHRenderState state;
    BHFadeContext fade;BHPresentContext present;BHPresentBegin begin;
    BHFrameDisplay display;BHRenderExecute render;BHTextQueue text;
    BHDrawQueue queues[3];BHViewConfig viewport;Resume r;Hold held;BHCapture capture;
    BHFrameCallback callback;
    HANDLE helper=NULL;unsigned long i,before;DWORD wait;FILE *file;unsigned char bmp[57];
    checks=failures=0;
    memset(&worker,0,sizeof(worker));memset(&frames,0,sizeof(frames));
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));
    memset(&fade,0,sizeof(fade));memset(&present,0,sizeof(present));
    memset(&begin,0,sizeof(begin));memset(&display,0,sizeof(display));
    memset(&render,0,sizeof(render));memset(queues,0,sizeof(queues));memset(&r,0,sizeof(r));
    memset(&capture,0,sizeof(capture));memset(&held,0,sizeof(held));
    if(!bh_math_environment_enter(&fp))return 2;
    CHECK(bh_scene_view_init(&view,640,480),"real640x480 offscreen DirectDraw/Direct3D2 owner");
    if(!view.device)goto done;
    CHECK(bh_frame_buffers_bind(&frames,NULL),"real Win32 handoff mutex");
    if(!frames.mutex)goto done;
    for(i=0;i<3;++i){
        CHECK(bh_draw_queue_init(queues+i,12,NULL),"real owning queue allocation");
        if(!queues[i].commands)goto done;
        queues[i].commands[0].kind=BH_DRAW_NOP;
        queues[i].commands[0].next=BH_DRAW_END;queues[i].count=1;
        frames.slots[i].data=queues[i].commands;frames.slots[i].age=3;
        worker.queues[i]=queues+i;
    }
    textures.device=view.device;
    CHECK(bh_render_state_init(&state,&textures),"actual renderer state context");
    CHECK(bh_view_config_init(&viewport,640,480),"real viewport contract");
    fade.render=&state;fade.width=640;fade.height=480;fade.near_plane=viewport.near_z;
    present.fade=&fade;present.auxiliary=view.depth;present.color=view.color;
    begin.present=&present;
    display.textures=&textures;display.auxiliary=view.depth;display.color=view.color;
    bh_text_queue_init(&text);render.display=&display;render.text=&text;render.state=&state;
    worker.frames=&frames;worker.render=&render;worker.begin=&begin;
    callback.render=NULL;worker.callback=&callback;
    worker.viewport=&viewport;worker.width=640;worker.height=480;
    worker.primary=view.color;worker.back=view.depth;
    worker.ready=1;worker.display_state=3;worker.stop=1;
    CHECK(!bh_frame_worker_run(&worker)&&worker.done&&worker.error==BH_FRAME_WORKER_ARGUMENT&&
          !worker.iterations&&!worker.rendered&&!worker.native_calls,
          "missing callback registration rejects before native rendering or frame acquisition");
    CHECK(bh_frame_callback_start(&callback)&&callback.render==bh_render_execute,
          "restored registration wrapper installs the actual complete renderer consumer");
    CHECK(bh_frame_worker_run(&worker)==0&&worker.done,"whole worker exits naturally at bottom-of-loop stop");
    CHECK(worker.iterations==1&&worker.rendered==1&&worker.selected==0,"first eligible tied youngest slot rendered despite initial stop");
    CHECK(!frames.slots[0].busy&&frames.slots[0].age==3,"consumer releases busy without mutating producer ages");
    CHECK(render.draw==NULL&&render.draw_calls==1&&render.kinds[BH_DRAW_NOP]==1,"real renderer consumes selected borrowed queue and restores caller binding");
    CHECK(worker.flips==1&&worker.last_hresult==DDERR_NOTFLIPPABLE&&worker.error==BH_FRAME_WORKER_NATIVE,
          "actual offscreen Flip failure recorded after real BeginScene/render/EndScene");
    CHECK(worker.restores==0&&worker.native_calls==1,"initial display-state3 bypasses IsLost checks");
    CHECK(!present.failures&&render.failures==0&&textures.frame==2,"real native dependencies execute without stub success");
    for(i=0;i<3;++i)frames.slots[i].age=-9999;
    CHECK(!bh_frame_worker_run(&worker)&&worker.done&&worker.selected==0&&worker.skipped==1&&worker.rendered==1,
          "selected signed -9999 sentinel skips all rendering and presentation");
    CHECK(worker.flips==1&&!frames.slots[0].busy,"sentinel neither claims buffer nor flips");
    for(i=0;i<3;++i)frames.slots[i].busy=1;
    CHECK(!bh_frame_worker_run(&worker)&&worker.done&&worker.selected==-1&&worker.error==BH_FRAME_WORKER_EMPTY,
          "source guard rejects original invalid saved-stack selection fallback");
    for(i=0;i<3;++i){frames.slots[i].busy=0;frames.slots[i].age=3;}
    frames.slots[0].age=2;frames.slots[1].age=0;frames.slots[2].age=0;
    CHECK(!bh_frame_worker_run(&worker)&&worker.selected==1&&worker.rendered==2,
          "strict youngest signed-age choice preserves earliest equal tie");
    worker.ready=0;worker.display_state=0;worker.rendered=0;worker.sleeps=0;
    frames.slots[0].age=0;frames.slots[1].age=frames.slots[2].age=3;
    r.worker=&worker;r.gate=CreateEventA(NULL,TRUE,FALSE,NULL);
    if(r.gate)helper=CreateThread(NULL,0,resume,&r,0,NULL);
    CHECK(r.gate&&helper,"isolated real state-notification helper thread");
    if(helper){
        SetEvent(r.gate);
        CHECK(!bh_frame_worker_run(&worker)&&worker.done&&worker.rendered==1,"real ready10ms polling and display-state synchronization complete");
        wait=WaitForSingleObject(helper,3000);CHECK(wait==WAIT_OBJECT_0,"helper naturally exits");
        CHECK(r.saw_free&&worker.sleeps>=3,"buffer released before display-state wait, ready gate uses actual Sleep");
        CHECK(worker.native_calls>=5&&worker.restores==0,"healthy primary and back both call real IsLost only after wait");
        CloseHandle(helper);helper=NULL;CloseHandle(r.gate);r.gate=NULL;
    }
    held.mutex=frames.mutex;held.ready=CreateEventA(NULL,TRUE,FALSE,NULL);
    if(held.ready)helper=CreateThread(NULL,0,hold,&held,0,NULL);
    CHECK(held.ready&&helper,"isolated native mutex contention fixture");
    if(helper){
        wait=WaitForSingleObject(held.ready,2000);CHECK(wait==WAIT_OBJECT_0,"helper owns actual mutex before consumer");
        if(wait==WAIT_OBJECT_0){before=worker.timeouts;
            CHECK(!bh_frame_worker_run(&worker)&&worker.done&&worker.timeouts>before,"real5ms WAIT_TIMEOUT loop retries until native owner releases");}
        CHECK(WaitForSingleObject(helper,3000)==WAIT_OBJECT_0,"mutex helper exits naturally");CloseHandle(helper);helper=NULL;
    }
    /* Synthetic offscreen framebuffer output exercises the restored game BMP
       feature. This reads no desktop, window or original-game pixels. */
    {
        DDPIXELFORMAT pixel;memset(&pixel,0,sizeof(pixel));pixel.dwSize=sizeof(pixel);
        pixel.dwFlags=DDPF_RGB;pixel.dwRGBBitCount=16;pixel.dwRBitMask=0xf800;
        pixel.dwGBitMask=0x7e0;pixel.dwBBitMask=31;
        CHECK(bh_texture_bind_display(&textures,&pixel),"actual RGB565 channel mapping");
    }
    remove("frame-worker-fixture0000.BMP");remove("frame-worker-fixture0001.BMP");
    file=fopen("frame-worker-fixture0000.BMP","wb");
    CHECK(file!=NULL,"create owned occupied filename control");if(file){fwrite("occupied",8,1,file);fclose(file);}
    capture.present=&present;capture.pixel_bytes=2;
    capture.masks[0]=31;capture.masks[1]=0x7e0;capture.masks[2]=0xf800;
    capture.extract[1]=5;capture.extract[2]=11;capture.expand[0]=capture.expand[2]=3;capture.expand[1]=2;
    worker.capture=&capture;worker.dump_prefix="frame-worker-fixture";
    worker.request_capture=1;render.display_mode=1;viewport.origin_x=17;viewport.left=7;
    {
        DDBLTFX fill;memset(&fill,0,sizeof(fill));fill.dwSize=sizeof(fill);fill.dwFillColor=0xffff;
        CHECK(IDirectDrawSurface_Blt(view.color,NULL,NULL,NULL,DDBLT_WAIT|DDBLT_COLORFILL,&fill)==DD_OK,
              "actual uniform synthetic source target before nonzero clear-origin fixture");
        display.left=7;display.top=11;
    }
    before=worker.captures;
    CHECK(!bh_frame_worker_run(&worker)&&worker.done&&!worker.request_capture&&worker.captures==before+1,
          "whole optional BMP branch names, resets viewport, sleeps, writes and clears request");
    CHECK(viewport.origin_x==0&&viewport.left==0&&capture.right==640&&capture.bottom==480&&display.left==0&&display.top==0,
          "capture consumes actual full-target reset rather than stale source clip fields");
    file=fopen("frame-worker-fixture0001.BMP","rb");
    CHECK(file!=NULL,"occupied0000 filename advances to0001");
    if(file){CHECK(fread(bmp,1,sizeof(bmp),file)==sizeof(bmp)&&bmp[0]=='B'&&bmp[1]=='M'&&
        bmp[54]==248&&bmp[55]==252&&bmp[56]==248,"native source-white RGB565 pixels written bottom-up");
        fseek(file,0,SEEK_END);CHECK(ftell(file)==54L+640L*480L*3L,"original full640x480 packed output size");fclose(file);}
    file=fopen("frame-worker-fixture0000.BMP","rb");if(file){memset(bmp,0,sizeof(bmp));
        CHECK(fread(bmp,1,8,file)==8&&!memcmp(bmp,"occupied",8),"readable filename control remains unchanged");fclose(file);}
    remove("frame-worker-fixture0000.BMP");remove("frame-worker-fixture0001.BMP");
    textures.software=1;render.display_mode=0;display.auxiliary_bytes=640UL*480UL*2UL;
    CHECK(!bh_frame_worker_run(&worker)&&worker.done&&worker.error==BH_FRAME_WORKER_NATIVE,
          "whole software begin/render/end keeps only real offscreenFlip error");
    CHECK(display.color_pixels==fade.pixels&&display.color_pitch==fade.pitch&&
        display.auxiliary_pixels==begin.auxiliary_pixels&&!display.locked&&!present.locked,
        "shared original software mappings and lock state flow through explicit real owners");
done:
    if(helper)CloseHandle(helper);if(r.gate)CloseHandle(r.gate);if(held.ready)CloseHandle(held.ready);
    for(i=0;i<3;++i){frames.slots[i].data=NULL;if(queues[i].commands)bh_draw_queue_dispose(queues+i);}
    if(frames.mutex)CHECK(bh_frame_buffers_dispose(&frames),"borrowed queue storage detached before actual mutex cleanup");
    bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"worker preserves caller's explicitPC53 profile");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"frame-worker\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false,\"ui_capture\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_FRAME_WORKER_STANDALONE
int main(void){return bh_frame_worker_checkpoint(NULL);}
#endif
