#include "../src/bh_texture.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){++failures;printf("FAIL texture %d: %s\n",__LINE__,label);}} while(0)
typedef struct Memory {unsigned long calls,live,fail_at;} Memory;
static void *__cdecl allocate(unsigned long size,void *user){Memory *m=(Memory *)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(size);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory *)user;--m->live;free(p);}
static void callback_cases(void) {
    BHTextureFormat f;DDSURFACEDESC d;DDPIXELFORMAT saved;unsigned long i;
    memset(&f,0,sizeof(f));memset(&d,0,sizeof(d));d.dwSize=sizeof(d);d.ddpfPixelFormat.dwSize=32;
    f.requested_depth=24;d.ddpfPixelFormat.dwFlags=DDPF_RGB;d.ddpfPixelFormat.dwRGBBitCount=16;
    CHECK(bh_texture_format_callback(&d,&f)==D3DENUMRET_OK && f.pixel.dwRGBBitCount==16,"first valid depth accepted below request, original fallback");
    d.ddpfPixelFormat.dwRGBBitCount=32;bh_texture_format_callback(&d,&f);
    CHECK(f.pixel.dwRGBBitCount==32,"unsigned difference replaces initial below-request fallback");
    d.ddpfPixelFormat.dwRGBBitCount=24;bh_texture_format_callback(&d,&f);CHECK(f.pixel.dwRGBBitCount==24,"closer supported depth wins");
    saved=f.pixel;d.ddpfPixelFormat.dwFlags=DDPF_RGB|DDPF_ALPHAPIXELS;bh_texture_format_callback(&d,&f);
    CHECK(!memcmp(&saved,&f.pixel,32),"alpha formats rejected");
    for(i=0;i<2;++i) {
        memset(&f,0,sizeof(f));f.requested_depth=8;d.ddpfPixelFormat.dwRGBBitCount=8;
        d.ddpfPixelFormat.dwFlags=i?0x60:0x40;bh_texture_format_callback(&d,&f);
        CHECK(f.pixel.dwRGBBitCount==(i?8UL:0UL),"8-bit requires indexed flag even with RGB flag");
    }
}
static int pixels(BHTextureImage *image,unsigned long expected) {
    DDSURFACEDESC d;unsigned long value=0,bytes,i;int ok;
    memset(&d,0,sizeof(d));d.dwSize=sizeof(d);
    if(IDirectDrawSurface_Lock(image->resident?image->resident->surface:image->source,NULL,&d,DDLOCK_WAIT|DDLOCK_READONLY,NULL)!=DD_OK)return 0;
    bytes=(d.ddpfPixelFormat.dwRGBBitCount+7)/8;for(i=0;i<bytes;++i)value|=(unsigned long)((unsigned char *)d.lpSurface)[i]<<(i*8);
    ok=value==expected;IDirectDrawSurface_Unlock(image->resident?image->resident->surface:image->source,NULL);return ok;
}
static unsigned long bmp_pixel(const char *path,unsigned long x,unsigned long y) {
    BITMAPFILEHEADER header;BITMAPINFOHEADER info;unsigned char rgb[3];FILE *f=fopen(path,"rb");unsigned long color=0xffffffffUL,stride;
    if(!f)return color;
    if(fread(&header,sizeof(header),1,f)==1 && fread(&info,sizeof(info),1,f)==1 && info.biBitCount==24) {
        stride=(info.biWidth*3+3)&~3UL;
        if(!fseek(f,header.bfOffBits+(info.biHeight-y-1)*stride+x*3,SEEK_SET) && fread(rgb,3,1,f)==1)color=((unsigned long)rgb[2]<<16)|((unsigned long)rgb[1]<<8)|rgb[0];
    }
    fclose(f);return color;
}
static void native_cases(BHSceneView *view) {
    BHTextureContext q;BHSceneAllocator a;Memory memory={0,0,0};BHPcx pcx;
    BHTextureFormat *format;BHTextureImage *red,*green,*blue;unsigned char bytes[1024];D3DTEXTUREHANDLE handle;
    unsigned long i,uploads;BHTextureSlot *slot;
    a.allocate=allocate;a.release=release;a.user=&memory;
    CHECK(bh_texture_context_init(&q,view->draw,view->device,&a),"source context AddRefs native draw/device");
    q.software=1;CHECK(bh_texture_formats(&q) && q.formats[8].pixel.dwRGBBitCount==8 && q.formats[8].pixel.dwFlags==0x60,
        "original software branch populates indexed8 format metadata without native enumeration");
    q.software=0;for(i=0;i<9;++i){memset(&q.formats[i].pixel,0,32);q.formats[i].pixel.dwSize=32;}
    CHECK(bh_texture_formats(&q),"real native texture-format enumeration");
    printf("{\"native_texture_depth\":%lu,\"red_mask\":%lu,\"green_mask\":%lu,\"blue_mask\":%lu}\n",q.formats[0].pixel.dwRGBBitCount,q.formats[0].pixel.dwRBitMask,q.formats[0].pixel.dwGBitMask,q.formats[0].pixel.dwBBitMask);
    CHECK(q.formats[0].pixel.dwRGBBitCount>=8,"native RGB renderer exposes an actual texture format");
    CHECK(!bh_texture_request_format(&q,48,32,8,1) && q.error==BH_TEXTURE_FORMAT,"unsupported dimensions cannot invent texture class");
    format=bh_texture_request_format(&q,32,32,8,1);
    CHECK(format==q.formats && format->references==1 && format->minimum==16,"exact original class and reference count");
    --format->references;
    /* Deliberate platform binding: offscreen RGB device uses system memory.
       Production default original VIDMEM/ALLOCONLOAD caps remain in source. */
    q.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));memset(bytes,1,sizeof(bytes));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=bytes;
    pcx.palette[3]=255;red=bh_texture_stage(&q,"RED.PCX",&pcx,0,0);
    CHECK(red!=NULL,"source palette converted into a real native image surface");if(!red)goto done;
    CHECK(bh_texture_stage(&q,"red.pcx",&pcx,0,0)==red && format->references==1,"cache identity case-insensitive and does not repeat reference request");
    CHECK(pixels(red,format->pixel.dwRGBBitCount==8?1:format->pixel.dwRBitMask),"actual native staging pixel matches selected indexed/RGB format");
    memset(pcx.palette,0,768);pcx.palette[4]=255;green=bh_texture_stage(&q,"GREEN.PCX",&pcx,0,0);
    memset(pcx.palette,0,768);pcx.palette[5]=255;blue=bh_texture_stage(&q,"BLUE.PCX",&pcx,0,0);
    CHECK(green && blue && format->references==3,"three source image owners share one real format class");if(!green || !blue)goto done;
    memory.fail_at=memory.calls+1;
    CHECK(!bh_texture_preallocate(&q) && !format->slots && format->minimum==3 && q.error==BH_TEXTURE_MEMORY,"mandatory allocation failure rejects without fabricated native slot");
    memory.fail_at=0;
    /* One deliberate residency slot exercises original eviction, rather than
       allocating a fresh native texture for every image. */
    CHECK(bh_texture_slot_create(&q,format) && format->count==1 && format->slots->handle,"real native Texture2 QueryInterface/GetHandle");
    slot=format->slots;q.error=BH_TEXTURE_OK;q.frame=1;
    CHECK(bh_scene_view_begin(view),"native scene begins before residency assignment");
    handle=bh_texture_resident(&q,red);
    CHECK(handle==slot->handle && red->resident==slot && slot->image==red && pixels(red,format->pixel.dwRGBBitCount==8?1:format->pixel.dwRBitMask),"resident native upload and reciprocal ownership");
    uploads=q.pixel_uploads;
    CHECK(bh_texture_resident(&q,red)==handle && q.pixel_uploads==uploads,"cached resident handle avoids repeated pixel upload");
    CHECK(bh_texture_resident(&q,green)==handle && !red->resident && green->resident==slot && q.evictions==1 && q.flushes==1,
        "same-frame occupied eviction flushes actual EndScene/BeginScene before reusing handle");
    CHECK(pixels(green,format->pixel.dwRGBBitCount==8?1:format->pixel.dwGBitMask),"eviction uploads green pixels to same native device texture");
    q.frame=2;CHECK(bh_texture_resident(&q,blue)==handle && q.flushes==1 && q.evictions==2,"older-frame eviction requires no same-frame flush");
    CHECK(pixels(blue,format->pixel.dwRGBBitCount==8?1:format->pixel.dwBBitMask),"reused native texture now contains blue");
    if(blue->palette && slot->palette) {
        PALETTEENTRY entry;
        CHECK(IDirectDrawPalette_GetEntries(slot->palette,0,1,1,&entry)==DD_OK && !entry.peRed && !entry.peGreen && entry.peBlue==255,
            "actual resident palette identifies blue after same-index image eviction");
        entry.peRed=entry.peGreen=entry.peBlue=255;entry.peFlags=0;
        CHECK(IDirectDrawPalette_SetEntries(blue->palette,0,1,1,&entry)==DD_OK,"real source palette edit");
        slot->palette_dirty=1;uploads=q.palette_uploads;bh_texture_resident(&q,blue);
        CHECK(!slot->palette_dirty && q.palette_uploads==uploads+1 && IDirectDrawPalette_GetEntries(slot->palette,0,1,1,&entry)==DD_OK &&
            entry.peRed==255 && entry.peGreen==255 && entry.peBlue==255,"resident palette dirty flag copies entries and calls native PaletteChanged");
    }
    slot->pixels_dirty=1;uploads=q.pixel_uploads;
    CHECK(bh_texture_resident(&q,blue)==handle && !slot->pixels_dirty && q.pixel_uploads==uploads+1,"resident dirty pixel flag clears and uploads real source");
    CHECK(bh_scene_view_end(view,"texture-control.bmp"),"native scene ownership remains balanced after flush");
    /* Existing slots survive an allocator failure in original best-effort pass. */
    for(i=0;i<9;++i)q.formats[i].minimum=q.formats[i].references=0;
    format->references=1;memory.fail_at=memory.calls+1;
    CHECK(bh_texture_preallocate(&q) && format->references==0 && format->count==1 && q.error==BH_TEXTURE_MEMORY,"best-effort pass retains original partial allocation result and exposes failure separately");memory.fail_at=0;
    for(i=0;i<9;++i)q.formats[i].minimum=q.formats[i].references=0;
    format->minimum=1;format->references=2;
    CHECK(bh_texture_preallocate(&q) && format->count==3 && !format->references && !format->minimum,"two-pass native allocation consumes mandatory and remaining references");
    CHECK(bh_scene_view_begin(view),"begin independent multi-slot native residency check");q.frame=3;
    CHECK(bh_texture_resident(&q,red) && bh_texture_resident(&q,green) && red->resident!=green->resident && blue->resident,
        "free slot selection precedes occupied oldest-slot eviction");
    {BHTextureImage *yellow;BHTextureSlot *oldest=format->slots;
     memset(pcx.palette,0,768);pcx.palette[3]=pcx.palette[4]=255;
     yellow=bh_texture_stage(&q,"YELLOW.PCX",&pcx,0,0);
     for(slot=format->slots;slot;slot=slot->next)slot->frame=7;
     oldest->frame=(-2147483647L-1);q.frame=8;
     CHECK(yellow && bh_texture_resident(&q,yellow)==oldest->handle && yellow->resident==oldest,"oldest slot compares signed frame values, including negative wrap boundary");
    }
    CHECK(bh_scene_view_end(view,"texture-multi-control.bmp"),"multi-slot source/native lifetime remains balanced");
done:
    bh_texture_context_dispose(&q);CHECK(memory.live==0,"staging/pool surfaces and all source allocator owners released");
}
static void coordinates(BHSceneView *view) {
    BHTextureContext q;BHPcx pcx;BHTextureImage *image;unsigned char bytes[1024];unsigned long x,y;
    const float uv[8]={0,1,1,1,1,0,0,0};
    const BHVec3 square[4]={{-2,-2,2},{2,-2,2},{2,2,2},{-2,2,2}};
    const BHVec3 receding[4]={{-2,-2,2},{4,-4,4},{4,4,4},{-2,2,2}};
    CHECK(bh_texture_context_init(&q,view->draw,view->device,NULL) && bh_texture_formats(&q),"native image coordinate context");
    q.slot_caps_override=DDSCAPS_SYSTEMMEMORY|DDSCAPS_TEXTURE;q.frame=1;
    memset(&pcx,0,sizeof(pcx));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=bytes;
    pcx.palette[3]=255;pcx.palette[7]=255;pcx.palette[11]=255;pcx.palette[12]=pcx.palette[13]=pcx.palette[14]=255;
    for(y=0;y<32;++y)for(x=0;x<32;++x)bytes[y*32+x]=(unsigned char)(1+(x>=16)+2*(y>=16));
    image=bh_texture_stage(&q,"QUADRANTS.PCX",&pcx,0,0);
    CHECK(image && bh_texture_preallocate(&q),"quadrant palette fixture uses actual native slot allocation");
    if(image) {
        CHECK(bh_scene_view_begin(view) && bh_scene_view_textured_polygon(view,square,uv,4,&q,image) &&
            bh_scene_view_end(view,"uv-control.bmp"),"raw normalized UV pairs reach native textured DrawPrimitive");
        CHECK(bmp_pixel("uv-control.bmp",8,8)==0xff0000 && bmp_pixel("uv-control.bmp",56,8)==0x00ff00 &&
            bmp_pixel("uv-control.bmp",8,56)==0x0000ff && bmp_pixel("uv-control.bmp",56,56)==0xffffff,"four native pixels verify palette identity, UV orientation and edge order");
        ++q.frame;
        CHECK(bh_scene_view_begin(view) && bh_scene_view_textured_polygon(view,receding,uv,4,&q,image) &&
            bh_scene_view_end(view,"perspective-control.bmp"),"native perspective interpolation over unequal reciprocal depths");
        CHECK(bmp_pixel("perspective-control.bmp",32,8)==0xff0000 && bmp_pixel("perspective-control.bmp",48,8)==0x00ff00,
            "perspective center U=1/3 selects red while right U=3/5 selects green, rejecting affine interpolation");
    }
    bh_texture_context_dispose(&q);
}
int bh_texture_checkpoint(unsigned long *count) {
    BHSceneView view;BHMathEnvironment fp;
    if(!bh_math_environment_enter(&fp))return 2;
    callback_cases();memset(&view,0,sizeof(view));
    CHECK(bh_scene_view_init(&view,64,64),"real native RGB rendering dependency");
    CHECK(bh_math_environment_supported(),"native device creation retains required PC53 source boundary");
    if(view.device){native_cases(&view);coordinates(&view);}bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"native texture/render lifetime retains required source numerical environment");
    bh_math_environment_leave(&fp);
    *count=checks;
    printf("{\"scenario\":\"texture-ownership\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}
#ifndef BH_TEXTURE_LINKED
int main(void){unsigned long count;return bh_texture_checkpoint(&count);}
#endif
