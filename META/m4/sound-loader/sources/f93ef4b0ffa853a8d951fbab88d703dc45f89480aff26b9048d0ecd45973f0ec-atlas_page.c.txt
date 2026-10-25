#include "../src/bh_atlas_page.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<15)printf("FAIL atlas-page %d: %s\n",line,label);}}
typedef struct Memory {unsigned long calls,live,fail_at;} Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory *m=(Memory *)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){if(p){--((Memory *)user)->live;free(p);}}
static int fixture(void) {
    unsigned char data[128+192+768];unsigned long x,y;FILE *f;
    memset(data,0,sizeof(data));data[0]=10;data[1]=2;data[2]=1;data[3]=8;data[8]=15;data[10]=11;
    for(y=0;y<12;++y)for(x=0;x<16;++x)data[128+y*16+x]=(unsigned char)((y>=1 && y<=3 && x>=2 && x<=5)?1:7);
    data[320+3]=43;data[320+4]=72;data[320+5]=101;data[320+21]=12;data[320+22]=15;data[320+23]=18;
    f=fopen("PAGE_ATLAS.PCX","wb");if(!f)return 0;if(fwrite(data,1,sizeof(data),f)!=sizeof(data)){fclose(f);return 0;}return !fclose(f);
}
static void pixel_format(DDPIXELFORMAT *p,unsigned long bits) {
    memset(p,0,sizeof(*p));p->dwSize=sizeof(*p);p->dwFlags=bits==8?DDPF_RGB|DDPF_PALETTEINDEXED8:DDPF_RGB;p->dwRGBBitCount=bits;
    if(bits==16){p->dwRBitMask=0xf800;p->dwGBitMask=0x7e0;p->dwBBitMask=0x1f;}
}
static int context(BHSceneView *view,BHTextureContext *q,BHAtlasContext *atlas,Memory *memory,unsigned long bits,unsigned long software) {
    BHSceneAllocator allocator;DDPIXELFORMAT pixel;
    memset(memory,0,sizeof(*memory));allocator.allocate=allocate;allocator.release=release;allocator.user=memory;
    if(!bh_texture_context_init(q,view->draw,view->device,&allocator) || !bh_texture_formats(q))return 0;
    q->software=software;q->palette_mode=2;q->slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;q->frame=1;
    pixel_format(&q->formats[0].pixel,bits);pixel_format(&pixel,16);
    if(!bh_texture_bind_display(q,&pixel))return 0;
    memset(atlas,0,sizeof(*atlas));return bh_atlas_context_init(atlas,q,16);
}
static void residency(BHTextureContext *q,BHAtlasPage *page,unsigned long bits) {
    DDSURFACEDESC lock;LPDIRECT3DTEXTURE2 native_texture=NULL;D3DTEXTUREHANDLE handle=0,actual=0;unsigned long x,y,pixel,j;
    memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
    CHECK(IDirectDrawSurface_Lock(page->surface,NULL,&lock,DDLOCK_WAIT,NULL)==DD_OK,"real page texture lock for downstream residency fixture");
    if(lock.lpSurface) {
        /* Constructor intentionally creates blank/uninitialized content.
           This explicit fixture fill is not a glyph-draw recovery claim. */
        for(y=0;y<32;++y)for(x=0;x<32;++x){unsigned char *out=(unsigned char *)lock.lpSurface+y*lock.lPitch+x*(bits/8);pixel=bits==8?1:0x2a4c;for(j=0;j<bits/8;++j)out[j]=(unsigned char)(pixel>>(8*j));}
        CHECK(IDirectDrawSurface_Unlock(page->surface,NULL)==DD_OK,"real page fill lock released");
    }
    CHECK(bh_texture_preallocate(q) && page->image->format->count==1,"real requested page format allocated actual native slot");
    handle=bh_texture_resident(q,page->image);CHECK(handle && page->image->resident && page->image->resident->image==page->image,"real recovered residency acquires handle for constructed page");
    if(page->image->resident) {
        CHECK(IDirectDrawSurface_QueryInterface(page->image->resident->surface,&IID_IDirect3DTexture2,(void **)&native_texture)==DD_OK,"real resident slot exposes official native texture interface");
        if(native_texture){CHECK(IDirect3DTexture2_GetHandle(native_texture,q->device,&actual)==DD_OK && actual==handle,"native GetHandle independently confirms recovered residency handle");IDirect3DTexture2_Release(native_texture);}
        memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
        CHECK(IDirectDrawSurface_Lock(page->image->resident->surface,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"read actual native resident slot contents");
        if(lock.lpSurface){pixel=*(unsigned char *)lock.lpSurface;if(bits==16)pixel|=(unsigned long)((unsigned char *)lock.lpSurface)[1]<<8;CHECK(pixel==(bits==8?1:0x2a4c),"constructed page uploads actual fixture content through real residency owner");IDirectDrawSurface_Unlock(page->image->resident->surface,NULL);}
    }
}
int bh_atlas_page_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext q;BHAtlasContext atlas;BHAtlasPage *page,*second;BHAtlasImage *cached;Memory memory;
    BHMathEnvironment fp;DDSURFACEDESC desc;PALETTEENTRY entry;unsigned long bits,software,fail_at,before;int initialized;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    CHECK(sizeof(BHAtlasPage)==0x28 && offsetof(BHAtlasPage,atlas)==0 && offsetof(BHAtlasPage,image)==4 && offsetof(BHAtlasPage,surface)==8 && offsetof(BHAtlasPage,last_color)==0xc && offsetof(BHAtlasPage,size)==0x20,"complete original40byte page layout");
    CHECK(fixture(),"real atlas source PCX fixture");memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"actual native RGB fixture backend");
    if(view.device)for(bits=8;bits<=16;bits+=8)for(software=0;software<2;++software) {
        initialized=context(&view,&q,&atlas,&memory,bits,software);CHECK(initialized,"real shared texture/atlas contexts");if(!initialized)continue;
        page=bh_atlas_page_create(&atlas,"PAGE_ATLAS.PCX",32);CHECK(page && q.error==BH_TEXTURE_OK,"complete original page creation over actual native8/16 interfaces");
        if(page) {
            CHECK(page->atlas==atlas.images && page->atlas->rectangle_count==1 && page->image==q.images && page->surface==page->image->source,"borrowed real atlas and native page/image publication");
            CHECK(page->last_color==0xffffffffUL && !page->packed_color && !page->cursor_x && !page->cursor_y && !page->left && page->size==32 && !page->line_height,"original page cursor/color sentinels and source undefined-field initialization");
            CHECK(!strcmp(page->image->name,"@@@PAGE@@@") && page->image->flags==1 && !page->image->resident && !page->image->unused54,"exact original page marker and native-image state");
            CHECK(page->image->runs && page->image->vertices && page->image->vertex_capacity==96 && !page->image->run_count && !page->image->run_serial && !page->image->vertex_count,"real original0x80/0xc00 draw arenas and zero counts");
            CHECK(page->image->format==q.formats && q.formats[0].references==1,"real format request size,size,8,1 and original pending-reference side effect");
            memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);CHECK(IDirectDrawSurface_GetSurfaceDesc(page->surface,&desc)==DD_OK && desc.dwWidth==32 && desc.dwHeight==32 && (desc.ddsCaps.dwCaps&0x1800)==0x1800 && desc.ddpfPixelFormat.dwRGBBitCount==bits,"actual native page dimensions/caps/format");
            if(bits==8) {
                CHECK(page->image->palette && IDirectDrawPalette_GetEntries(page->image->palette,0,1,1,&entry)==DD_OK && entry.peRed==43 && entry.peGreen==72 && entry.peBlue==101 && !entry.peFlags,"actual attached palette from transformed borrowed atlas RGB");
                CHECK(software?(page->image->dither && page->image->screen):(!page->image->dither && !page->image->screen),"real palette-table owner allocates software-only dither/screen buffers");
                if(software)CHECK(page->image->screen[1]==0x2a4c,"real software palette screen table uses bound565 display");
            } else CHECK(!page->image->palette && !page->image->dither && !page->image->screen,"highcolor page bypasses palette creation/table owner");
            residency(&q,page,bits);
            cached=page->atlas;second=bh_atlas_page_create(&atlas,"page_atlas.pcx",32);
            CHECK(second && second!=page && second->image!=page->image && second->atlas==cached && q.images==second->image && second->image->next==page->image,"pages are independent blank textures sharing literal case-insensitive atlas cache");
            if(second)CHECK(bh_atlas_page_dispose(&atlas,second) && q.images==page->image,"source lifetime adapter unlinks one page retaining the other");
            CHECK(bh_atlas_page_dispose(&atlas,page) && !q.images && atlas.images==cached && !q.formats[0].slots->image,"source page teardown detaches real residency and keeps borrowed atlas alive");
        }
        bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&q);CHECK(!memory.live,"all real page/atlas/native slot allocation lifetimes close");
    }
    if(view.device)for(software=0;software<2;++software)for(fail_at=1;fail_at<=(software?6:4);++fail_at) {
        initialized=context(&view,&q,&atlas,&memory,software?8:16,software);CHECK(initialized,"allocation-failure real contexts");if(!initialized)continue;
        cached=bh_atlas_image_load(&atlas,"PAGE_ATLAS.PCX",&q.formats[0].pixel);CHECK(cached!=NULL,"prewarmed real borrowed atlas for exact page-allocation fault position");
        before=memory.live;memory.calls=0;memory.fail_at=fail_at;
        page=bh_atlas_page_create(&atlas,"PAGE_ATLAS.PCX",32);
        CHECK(!page && q.error==BH_TEXTURE_MEMORY && !q.images && atlas.images==cached && memory.live==before,"each image/page/palette-table/arena allocation failure cleans only owned partial page resources");
        memory.fail_at=0;bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&q);CHECK(!memory.live,"failed page fixture all ownership released");
    }
    if(view.device) {
        initialized=context(&view,&q,&atlas,&memory,16,0);CHECK(initialized,"argument failure real contexts");
        if(initialized){CHECK(!bh_atlas_page_create(NULL,"PAGE_ATLAS.PCX",32) && !bh_atlas_page_create(&atlas,NULL,32),"null source guards");CHECK(!bh_atlas_page_create(&atlas,"PAGE_ATLAS.PCX",31) && q.error==BH_TEXTURE_FORMAT && !q.images && !memory.live,"real format owner rejects unsupported page dimension");CHECK(!bh_atlas_page_create(&atlas,"PAGE_MISSING.PCX",32) && q.error==BH_TEXTURE_IO && !q.images && !memory.live,"real missing atlas error cleans initial image allocation");bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&q);}
    }
    bh_scene_view_dispose(&view);bh_vfile_shutdown();remove("PAGE_ATLAS.PCX");CHECK(bh_math_environment_supported(),"native page checks preserve FP profile");bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"atlas-page\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_ATLAS_PAGE_STANDALONE
int main(void){unsigned long count;return bh_atlas_page_checkpoint(&count);}
#endif
