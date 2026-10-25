#include "../src/bh_texture_draw.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<15)printf("FAIL texture draw %d: %s\n",line,label);}}
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory *m=(Memory *)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory *)user;--m->live;free(p);}
static D3DTLVERTEX vertex(float x,float y,float u) {
    D3DTLVERTEX v;memset(&v,0,sizeof(v));v.sx=x;v.sy=y;v.sz=.25f;v.rhw=1;v.color=0xffffffffUL;v.tu=u;v.tv=.5f;return v;
}
static BHTextureImage *image(BHTextureContext *q,BHSceneView *view,Memory *memory) {
    BHSceneAllocator a;BHPcx pcx;unsigned char pixels[1024];unsigned long i;
    a.allocate=allocate;a.release=release;a.user=memory;
    if(!bh_texture_context_init(q,view->draw,view->device,&a) || !bh_texture_formats(q))return NULL;
    q->slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;q->frame=1;
    memset(&pcx,0,sizeof(pcx));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;
    pcx.palette[3]=255;pcx.palette[7]=255;for(i=0;i<1024;++i)pixels[i]=(unsigned char)(1+(i%32>=16));
    return bh_texture_stage(q,"DRAWQUEUE.PCX",&pcx,0,0);
}
static unsigned long pixel(const char *name,unsigned long x,unsigned long y) {
    FILE *f=fopen(name,"rb");unsigned char b[3];BITMAPFILEHEADER header;BITMAPINFOHEADER info;unsigned long stride;
    if(!f)return 0xffffffUL;fread(&header,sizeof(header),1,f);fread(&info,sizeof(info),1,f);
    stride=(info.biWidth*3+3)&~3UL;fseek(f,header.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET);
    fread(b,1,3,f);fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
static void arenas(BHSceneView *view) {
    BHTextureContext q;Memory m={0,0,0};BHTextureImage *im;D3DTLVERTEX triangle[3],*out,*old_vertices;
    unsigned long *old_runs,live,i,j,old_count,old_capacity;D3DTLVERTEX saved[96];unsigned long runs[32];
    triangle[0]=vertex(4,4,.25f);triangle[1]=vertex(28,4,.25f);triangle[2]=vertex(4,28,.25f);
    im=image(&q,view,&m);CHECK(im && !im->vertices && !im->runs,"real staging adapter starts without draw arenas");if(!im)goto done;
    for(i=1;i<=2;++i) {
        live=m.live;m.fail_at=m.calls+i;
        CHECK(!bh_texture_draw_polygon(&q,im,triangle,3) && q.error==BH_TEXTURE_MEMORY && m.live==live && !im->vertices && !im->runs && !im->vertex_count,
            "each initial paired arena failure publishes nothing and releases successful peer");m.fail_at=0;q.error=BH_TEXTURE_OK;
    }
    CHECK(bh_texture_draw_polygon(&q,im,triangle,3) && im->vertex_capacity==96 && im->run_count==1 && im->runs[0]==3 && !im->run_serial,
        "explicit staging composition owns original initial 96/32 arenas and complete fan");
    for(i=1;i<32;++i)CHECK(bh_texture_draw_polygon(&q,im,triangle,3),"fill all original 96 vertices and 32 run slots");
    memcpy(saved,im->vertices,sizeof(saved));memcpy(runs,im->runs,sizeof(runs));live=m.live;old_vertices=im->vertices;old_runs=im->runs;
    for(i=1;i<=2;++i) {
        m.fail_at=m.calls+i;
        CHECK(!bh_texture_draw_vertex(&q,im) && q.error==BH_TEXTURE_MEMORY && im->vertices==old_vertices && im->runs==old_runs &&
            im->vertex_count==96 && im->vertex_capacity==96 && im->run_count==32 && !im->run_serial && m.live==live,
            "each growth allocation failure preserves original arena identity, counters and completed runs");
        CHECK(!memcmp(saved,im->vertices,sizeof(saved)) && !memcmp(runs,im->runs,sizeof(runs)),"failed paired growth retains active vertex and run bytes");m.fail_at=0;q.error=BH_TEXTURE_OK;
    }
    out=bh_texture_draw_vertex(&q,im);
    CHECK(out==im->vertices+96 && im->vertex_capacity==120 && im->vertex_count==97 && im->run_serial==1 && m.live==live,
        "original floor96*5/12 gives40 runs and120 vertices; append increments count and polygon serial");
    CHECK(!memcmp(saved,im->vertices,sizeof(saved)) && !memcmp(runs,im->runs,sizeof(runs)),"successful growth copies every active run and TL vertex byte");
    *out=triangle[0];out=bh_texture_draw_vertex(&q,im);*out=triangle[1];out=bh_texture_draw_vertex(&q,im);*out=triangle[2];im->runs[im->run_count++]=im->run_serial;im->run_serial=0;
    CHECK(im->run_count==33 && im->runs[32]==3 && im->vertex_count==99,"completed polygon crosses arena growth boundary");
    /* A complete polygon may fail after a successful prior growth. It restores
       visible counts while keeping any enlarged capacity, then can retry. */
    {D3DTLVERTEX many[60];for(j=0;j<60;++j)many[j]=triangle[j%3];old_count=im->vertex_count;old_capacity=im->vertex_capacity;
     m.fail_at=m.calls+3;CHECK(!bh_texture_draw_polygon(&q,im,many,60) && im->vertex_count==old_count && im->run_count==33 && !im->run_serial && im->vertex_capacity>old_capacity,
        "mid-polygon later growth failure rolls back active counts after earlier successful reallocation");m.fail_at=0;q.error=BH_TEXTURE_OK;
     CHECK(bh_texture_draw_polygon(&q,im,many,60) && im->runs[33]==60 && im->vertex_count==159,"retry appends exactly one full long fan after transactional failure");}
    im->vertex_count=im->run_count=im->run_serial=0;
    CHECK(bh_texture_draw_flush(&q,im,NULL) && !q.pixel_uploads,"empty flush does not acquire residency or issue draws");
    CHECK(!bh_texture_draw_polygon(&q,im,triangle,2) && !im->run_count,"source composition rejects incomplete non-polygon input");
done:
    bh_texture_context_dispose(&q);CHECK(!m.live,"all initial and grown image/arena owners release once");
}
static void native_draws(BHSceneView *view) {
    BHTextureContext q;Memory m={0,0,0};BHTextureImage *im;D3DTLVERTEX first[3],second[4];unsigned long drawn=7,count;
    first[0]=vertex(4,4,.25f);first[1]=vertex(28,4,.25f);first[2]=vertex(4,28,.25f);
    second[0]=vertex(36,4,.75f);second[1]=vertex(60,4,.75f);second[2]=vertex(60,28,.75f);second[3]=vertex(36,28,.75f);
    im=image(&q,view,&m);CHECK(im && bh_texture_slot_create(&q,im->format),"actual native source/slot backing for recovered draw flush");if(!im)goto done;
    CHECK(bh_texture_draw_polygon(&q,im,first,3) && bh_texture_draw_polygon(&q,im,second,4) && im->run_count==2 && im->vertex_count==7,
        "differently sized triangle and quad occupy one ordered image queue");
    q.software=1;CHECK(bh_texture_draw_flush(&q,im,&drawn) && im->vertex_count==7 && im->run_count==2 && drawn==7 && !q.pixel_uploads,
        "original software gate leaves pending native queue and counter untouched; no software rasterizer claimed");q.software=0;
    CHECK(bh_scene_view_begin(view),"native scene for real queued fans");
    CHECK(bh_texture_draw_flush(&q,im,&drawn) && drawn==9 && !im->vertex_count && !im->run_count && !im->run_serial && q.pixel_uploads==1,
        "native flush resolves actual handle, submits both fans, counts runs and resets all three active fields");
    CHECK(bh_scene_view_end(view,"texture-queue-control.bmp"),"queued native fans complete scene");
    CHECK(pixel("texture-queue-control.bmp",8,8)==0xff0000 && pixel("texture-queue-control.bmp",48,16)==0x00ff00 && pixel("texture-queue-control.bmp",24,24)==0,
        "native pixels prove first fan, second fan offset/count, UV identity and triangle boundary");
    count=drawn;CHECK(bh_texture_draw_flush(&q,im,&drawn) && drawn==count && q.pixel_uploads==1,"second empty flush repeats no residency upload or fan count");
    CHECK(bh_texture_draw_polygon(&q,im,first,3),"pending fan for source topology controls");im->runs[0]=4;
    CHECK(!bh_texture_draw_flush(&q,im,&drawn) && q.error==BH_TEXTURE_TOPOLOGY && im->run_count==1 && im->vertex_count==3 && drawn==count,
        "corrupt fan length rejects before native call and retains queue for diagnosis");im->runs[0]=3;q.error=BH_TEXTURE_OK;
    CHECK(bh_scene_view_begin(view) && bh_texture_draw_flush(&q,im,&drawn) && bh_scene_view_end(view,"texture-queue-retry.bmp") && drawn==count+1,
        "corrected queue retries through real native fan without losing original vertices");
done:
    bh_texture_context_dispose(&q);CHECK(!m.live,"native fan image and arena lifetimes balance");
}
int bh_texture_draw_checkpoint(unsigned long *count) {
    BHSceneView view;BHMathEnvironment fp;checks=0;failures=0;
    if(!bh_math_environment_enter(&fp))return 2;memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real offscreen native draw dependency");
    if(view.device){arenas(&view);native_draws(&view);}bh_scene_view_dispose(&view);CHECK(bh_math_environment_supported(),"queued native rendering retains PC53 source environment");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"texture-draw-queue\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
