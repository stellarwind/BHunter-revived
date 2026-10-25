#include "../src/bh_texture.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failures<12)printf("FAIL texture image %d: %s\n",__LINE__,label);}} while(0)
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long size,void *user){Memory *m=(Memory *)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(size);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory *)user;--m->live;free(p);}
static unsigned long crc(const unsigned char *p,unsigned long n){unsigned long c=0xffffffffUL,i;int j;for(i=0;i<n;++i){c^=p[i];for(j=0;j<8;++j)c=(c>>1)^(0xedb88320UL&(0UL-(c&1)));}return c^0xffffffffUL;}
static int fixture(const char *name,unsigned char r,unsigned char g,unsigned char b,unsigned long width,int valid) {
    unsigned char data[4096];unsigned long at=128,left=width*width;FILE *f;
    memset(data,0,sizeof(data));data[0]=valid?10:11;data[1]=2;data[2]=1;data[3]=8;
    data[4]=7;data[6]=9;data[8]=(unsigned char)(width-1);data[10]=(unsigned char)(width-1);data[65]=2;
    data[at++]=0xc0; /* original zero run; no value follows */
    while(left){data[at++]=0xff;data[at++]=1;left=left>63?left-63:0;}
    /* Last run deliberately crosses the requested pixel count, no palette
       marker, non-v5/origin/planes/stride. Original streaming dialect accepts. */
    memset(data+at,0,768);data[at+3]=r;data[at+4]=g;data[at+5]=b;at+=768;
    f=fopen(name,"wb");if(!f)return 0;if(fwrite(data,1,at,f)!=at){fclose(f);return 0;}return !fclose(f);
}
static int context(BHTextureContext *q,BHSceneView *view,Memory *m) {
    BHSceneAllocator a;a.allocate=allocate;a.release=release;a.user=m;
    return bh_texture_context_init(q,view->draw,view->device,&a) && bh_texture_formats(q);
}
static int palette_color(BHTextureImage *image,unsigned char r,unsigned char g,unsigned char b) {
    PALETTEENTRY entry;return image && image->palette && IDirectDrawPalette_GetEntries(image->palette,0,1,1,&entry)==DD_OK && entry.peRed==r && entry.peGreen==g && entry.peBlue==b;
}
static void paths(BHSceneView *view) {
    BHTextureContext q;Memory m;BHTextureImage *image;unsigned long i,calls,refs;char errors[8192];
    CreateDirectoryA("bhimage-prefix",NULL);
    CHECK(fixture("IMGCACHE.PCX",255,0,0,32,1) && fixture("bhimage-prefix\\IMGCACHE.PCX",0,255,0,32,1),"real loose root/prefix PCX fixtures");
    for(i=0;i<4;++i) {
        memset(&m,0,sizeof(m));CHECK(context(&q,view,&m),"actual image owner context");
        CHECK(bh_texture_set_path(&q,"bhimage-prefix",i==3?7:(long)i),"source path setter owns prefix and search policy");
        CHECK(!strcmp(q.prefix,"bhimage-prefix\\") && q.search==(i==3?-1:(long)i),"trailing separator and out-of-range bare-only policy");
        image=bh_texture_load(&q,"imgcache.tga");
        CHECK(image && !strcmp(image->name,"IMGCACHE.PCX") && palette_color(image,(i==0 || i==2)?0:255,(i==0 || i==2)?255:0,0),"native palette distinguishes all four real search policies");
        CHECK(image && image->runs && image->vertices && image->vertex_capacity==96 && !image->vertex_count && !image->run_count && image->flags==1,
            "complete original image draw arenas and cache fields have real owning allocations");
        calls=m.calls;refs=q.formats[0].references;
        CHECK(bh_texture_set_path(&q,"missing-prefix",2),"change source search after cache publication");q.palette_mode=2;
        CHECK(bh_texture_load(&q,"IMGCACHE.JPG")==image && m.calls==calls && q.formats[0].references==refs,"alias cache hit ignores changed path/palette and repeats no allocation/format request");
        CHECK(!bh_texture_load(&q,"NOTHERE.BMP") && q.error==BH_TEXTURE_IO && !strcmp(q.errors,"\nLOADERR:NOTHERE.PCX"),"missing real file appends canonical original load tag");
        strcpy(errors,q.errors);CHECK(!bh_texture_load(&q,"NOTHERE.GIF") && !strcmp(q.errors,errors),"load-error substring suppression retains original alias behavior");
        bh_texture_context_dispose(&q);CHECK(!m.live,"all image/native/arena owners dispose after cache use");
    }
    CHECK(fixture("bhimage-prefix\\IMGCACHE.PCX",0,255,0,32,0),"malformed preferred PCX fixture");
    memset(&m,0,sizeof(m));context(&q,view,&m);bh_texture_set_path(&q,"bhimage-prefix",0);
    image=bh_texture_load(&q,"IMGCACHE.PCX");CHECK(palette_color(image,255,0,0),"prefix-first falls back on invalid PCX, not only missing file");
    bh_texture_context_dispose(&q);CHECK(!m.live,"invalid-header fallback closes native/source ownership");
    CHECK(fixture("IMGCACHE.PCX",255,0,0,32,0) && fixture("bhimage-prefix\\IMGCACHE.PCX",0,255,0,32,1),"reverse malformed root control");
    memset(&m,0,sizeof(m));context(&q,view,&m);bh_texture_set_path(&q,"bhimage-prefix",1);
    CHECK(palette_color(bh_texture_load(&q,"IMGCACHE.PCX"),0,255,0),"bare-first falls back on invalid PCX to prefixed file");
    CHECK(bh_texture_set_path(&q,"",0) && q.search==-1 && !q.prefix[0],"empty prefix chooses bare-only");
    bh_texture_context_dispose(&q);CHECK(!m.live,"reverse fallback cleanup");
    remove("IMGCACHE.PCX");remove("bhimage-prefix\\IMGCACHE.PCX");RemoveDirectoryA("bhimage-prefix");bh_vfile_shutdown();
}
static void tables_and_failures(BHSceneView *view) {
    BHTextureContext q;Memory m;BHTextureImage *image;DDSURFACEDESC display;unsigned long i,j;unsigned char rgb[768];
    CHECK(fixture("IMGTABLE.PCX",43,72,101,32,1),"software table fixture with independently traced residual phases");
    for(i=1;i<=5;++i) {
        memset(&m,0,sizeof(m));CHECK(context(&q,view,&m),"nested allocation failure context");q.software=1;bh_texture_formats(&q);
        memset(&display,0,sizeof(display));display.dwSize=sizeof(display);
        CHECK(IDirectDrawSurface_GetSurfaceDesc(view->color,&display)==DD_OK && bh_texture_bind_display(&q,&display.ddpfPixelFormat),"real native display defines software output shifts");
        m.fail_at=i;image=bh_texture_load(&q,"IMGTABLE.PCX");
        CHECK(!image && !q.images && q.error==BH_TEXTURE_MEMORY && !m.live,"each image/table/run/vertex allocation failure releases all unpublished ownership");
        m.fail_at=0;image=bh_texture_load(&q,"IMGTABLE.PCX");
        CHECK(image && image->dither && image->screen && image->runs && image->vertices,"actual full software image retry creates all owners");
        if(image) {
            CHECK(image->dither[1]==50 && image->dither[257]==14 && image->dither[513]==14 && image->dither[769]==92,
                "hand-traced RGB43/72/101 proves original residual recurrence and phase0,1,3,2 placement");
            for(j=0;j<4;++j)CHECK(image->screen[j*256+1]==10828,"original software screen table repeats exact RGB565 value");
            memset(rgb,0,sizeof(rgb));rgb[3]=43;rgb[4]=72;rgb[5]=101;
            CHECK(bh_texture_palette_tables(&q,image,rgb,768) && image->dither[769]==92 && image->screen[1]==10828,"RGB triples and RGBA palette entries have identical color semantics");
            q.display_right[0]=0;CHECK(bh_texture_tables_init(&q) && q.tables.right[0][255]==31,"one-time table latch preserves original configured right shifts");
            CHECK(!bh_texture_bind_display(&q,&display.ddpfPixelFormat),"source binding rejects display changes after one-time initialization");
        }
        bh_texture_context_dispose(&q);CHECK(!m.live,"full software image and table disposal");
    }
    CHECK(fixture("IMGSIZE.PCX",255,0,0,16,1),"valid image unsupported by original class catalog");
    memset(&m,0,sizeof(m));context(&q,view,&m);
    CHECK(!bh_texture_load(&q,"IMGSIZE.PCX") && !strcmp(q.errors,"\nFORMATERR:IMGSIZE.PCX") && !m.live,"format failure preserves original tag without guessed dimensions/class");
    bh_texture_context_dispose(&q);remove("IMGSIZE.PCX");remove("IMGTABLE.PCX");bh_vfile_shutdown();
}
static void rgb_formats(BHSceneView *view) {
    BHTextureContext q;Memory m;BHTextureImage *image;DDSURFACEDESC lock;
    unsigned long bits,i,j,value,bytes;DDPIXELFORMAT *pixel;
    CHECK(fixture("IMGRGB.PCX",43,72,101,32,1),"known RGB image fixture");
    for(i=0;i<3;++i) {
        bits=i==0?16:i==1?24:32;memset(&m,0,sizeof(m));context(&q,view,&m);
        /* The fixture binds a real native system-memory staging surface at
           each original output depth. It is not a claim that this RGB device
           enumerates all three as usable device textures. */
        q.software=1;pixel=&q.formats[0].pixel;memset(pixel,0,sizeof(*pixel));pixel->dwSize=32;pixel->dwFlags=DDPF_RGB;
        pixel->dwRGBBitCount=bits;pixel->dwRBitMask=bits==16?0xf800:0xff0000;
        pixel->dwGBitMask=bits==16?0x7e0:0xff00;pixel->dwBBitMask=bits==16?0x1f:0xff;
        image=bh_texture_load(&q,"IMGRGB.PCX");CHECK(image && !image->palette && !image->dither,"native RGB image owner does not fabricate indexed palette tables");
        if(image) {
            memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
            CHECK(IDirectDrawSurface_Lock(image->source,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"actual native16/24/32-bit pixel storage");
            if(lock.lpSurface) {
                bytes=bits/8;value=0;for(j=0;j<bytes;++j)value|=(unsigned long)((unsigned char *)lock.lpSurface)[j]<<(j*8);
                CHECK(value==(bits==16?10828UL:0x2b4865UL),"independent exact RGB565/RGB888 low-endian stored pixel");
                IDirectDrawSurface_Unlock(image->source,NULL);
            }
        }
        bh_texture_context_dispose(&q);CHECK(!m.live,"native RGB staging and draw arenas dispose without leaks");
    }
    memset(&m,0,sizeof(m));context(&q,view,&m);q.formats[0].pixel.dwRGBBitCount=17;q.formats[0].pixel.dwFlags=DDPF_RGB;
    CHECK(!bh_texture_load(&q,"IMGRGB.PCX") && q.error==BH_TEXTURE_BACKEND && q.last_hresult!=DD_OK && !strcmp(q.errors,"\nALLOCERR:IMGRGB.PCX") && !m.live,
        "real native invalid pixel format failure preserves original allocation-error tag and closes source file");
    bh_texture_context_dispose(&q);remove("IMGRGB.PCX");bh_vfile_shutdown();
}
typedef struct Transform {unsigned long mode,color,palette_crc,indices_crc,mapped_crc;} Transform;
typedef struct Manifest {const char *name;unsigned long offset,size,width,height,pixels,pixels_crc,palette_crc;Transform transforms[4];} Manifest;
static const Manifest manifest[]={
#include "pcx_manifest.h"
};
static void real_assets(BHSceneView *view,const char *archive) {
    BHTextureContext q;Memory m={0,0,0};unsigned long i,j,loaded=0,rejected=0;unsigned char pixels[16384],palette[768];
    CHECK(context(&q,view,&m) && bh_vfile_mount(archive) && bh_texture_set_path(&q,"TEXTURES",2),"full original image owner over real archive");
    for(i=0;i<sizeof(manifest)/sizeof(manifest[0]);++i) {
        const Manifest *f=manifest+i;const char *name=strchr(f->name,'\\');BHTextureImage *image;DDSURFACEDESC d;PALETTEENTRY entries[256];
        name=name?name+1:f->name;image=bh_texture_load(&q,name);
        if(f->width!=f->height || (f->width!=32 && f->width!=64 && f->width!=128)) {
            CHECK(!image && q.error==BH_TEXTURE_FORMAT,"unsupported real asset stays explicit original catalog rejection");++rejected;continue;
        }
        CHECK(image && image->source && image->palette && image->runs && image->vertices,"complete native-backed cache owner for every supported real asset");if(!image)continue;
        memset(&d,0,sizeof(d));d.dwSize=sizeof(d);
        CHECK(IDirectDrawSurface_Lock(image->source,NULL,&d,DDLOCK_READONLY|DDLOCK_WAIT,NULL)==DD_OK,"actual real-asset native pixel storage");
        if(d.lpSurface) {
            for(j=0;j<f->height;++j)memcpy(pixels+j*f->width,(unsigned char *)d.lpSurface+j*d.lPitch,f->width);
            CHECK(crc(pixels,f->pixels)==f->transforms[0].mapped_crc,"all native mapped pixels match independently pinned PCX fixture CRC");
            IDirectDrawSurface_Unlock(image->source,NULL);
        }
        CHECK(IDirectDrawPalette_GetEntries(image->palette,0,0,256,entries)==DD_OK,"actual real native palette entries");
        for(j=0;j<256;++j){palette[j*3]=entries[j].peRed;palette[j*3+1]=entries[j].peGreen;palette[j*3+2]=entries[j].peBlue;}
        CHECK(crc(palette,768)==f->transforms[0].palette_crc,"native transformed palette matches independent full-fixture CRC");++loaded;
    }
    CHECK(loaded+rejected==459 && loaded>146,"all459 PCX assets classified through owner, beyond visible scene subset");
    printf("{\"image_owner_supported_assets\":%lu,\"catalog_rejected_assets\":%lu}\n",loaded,rejected);
    bh_texture_context_dispose(&q);CHECK(!m.live,"all real cache objects, arenas, palettes and surfaces released");bh_vfile_shutdown();
}
int bh_texture_image_checkpoint(const char *archive,unsigned long *count) {
    BHSceneView view;BHMathEnvironment fp;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real native image dependency");
    if(view.device){paths(&view);tables_and_failures(&view);rgb_formats(&view);if(archive)real_assets(&view,archive);}
    bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"texture-image-owner\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifndef BH_TEXTURE_IMAGE_LINKED
int main(int argc,char **argv){unsigned long count;return bh_texture_image_checkpoint(argc==2?argv[1]:NULL,&count);}
#endif
