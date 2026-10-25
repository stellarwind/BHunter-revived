#include "../src/bh_atlas_image.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL atlas-image %d: %s\n",line,label);}}
typedef struct Memory {unsigned long calls,live,fail_at;} Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory *m=(Memory *)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){if(p){--((Memory *)user)->live;free(p);}}
static unsigned char value(unsigned long x,unsigned long y,unsigned long pattern) {
    if(pattern==2)return (unsigned char)((y%2 && x%2)?1:7);
    if(y>=1 && y<=3 && x>=1 && x<=2)return 1;
    if(y>=1 && y<=3 && x>=6 && x<=8 && !(pattern==1 && y==1))return 1;
    if(y>=6 && y<=7 && x>=3 && x<=4)return 1;
    return 7;
}
static int fixture(const char *name,unsigned long width,unsigned long height,unsigned long pattern,int truncated) {
    unsigned char header[128],palette[768],b;unsigned long x,y;FILE *f=fopen(name,"wb");if(!f)return 0;
    memset(header,0,sizeof(header));header[0]=10;header[1]=2;header[2]=1;header[3]=8;header[8]=(unsigned char)(width-1);header[9]=(unsigned char)((width-1)>>8);header[10]=(unsigned char)(height-1);header[11]=(unsigned char)((height-1)>>8);
    /* Original header consumer intentionally ignores origin/planes/line stride. */
    header[4]=9;header[6]=13;header[65]=4;header[66]=99;
    memset(palette,0,sizeof(palette));palette[3]=43;palette[4]=72;palette[5]=101;palette[21]=12;palette[22]=15;palette[23]=18;
    if(fwrite(header,1,128,f)!=128){fclose(f);return 0;}
    b=0xc0;fwrite(&b,1,1,f); /* original C0 no-op RLE prefix */
    if(!truncated)for(y=0;y<height;++y)for(x=0;x<width;++x){b=value(x,y,pattern);if(fwrite(&b,1,1,f)!=1){fclose(f);return 0;}}
    if(fwrite(palette,1,768,f)!=768){fclose(f);return 0;}return !fclose(f);
}
static void pixel_format(DDPIXELFORMAT *p,unsigned long bits) {
    memset(p,0,sizeof(*p));p->dwSize=sizeof(*p);p->dwFlags=bits==8?DDPF_RGB|DDPF_PALETTEINDEXED8:DDPF_RGB;p->dwRGBBitCount=bits;
    if(bits==16){p->dwRBitMask=0xf800;p->dwGBitMask=0x7e0;p->dwBBitMask=0x1f;}
    else if(bits>8){p->dwRBitMask=0xff0000;p->dwGBitMask=0xff00;p->dwBBitMask=0xff;}
}
static int rectangle(const BHAtlasRectangle *r,unsigned long x,unsigned long y,unsigned long w,unsigned long h){return r->x==x && r->y==y && r->width==w && r->height==h;}
static void verify(BHAtlasImage *image,unsigned long bits,unsigned long mode) {
    DDSURFACEDESC lock;unsigned long x,y,j,n=bits/8,actual,expected,index;LPDIRECTDRAWPALETTE native_palette=NULL;
    CHECK(image->rectangle_count==3 && rectangle(image->rectangles,1,1,2,3) && rectangle(image->rectangles+1,6,1,3,3) && rectangle(image->rectangles+2,3,6,2,2),"independent three glyph rectangle geometry/order");
    CHECK(!strcmp(image->name,"ATLAS.BMP") && image->palette[3]==43 && image->palette[4]==72 && image->palette[5]==101,"literal filename retained and transformed palette snapshot");
    CHECK(image->palette[21]==((mode==1 || mode==3)?8:12),"current palette key policy affects stored atlas palette");
    memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);CHECK(IDirectDrawSurface_Lock(image->surface,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"read actual native decoded pixel surface");
    if(lock.lpSurface) {
        CHECK(lock.dwWidth==16 && lock.dwHeight==12 && lock.ddpfPixelFormat.dwRGBBitCount==bits,"original xmax+1/ymax+1 and requested native pixel format");
        for(y=0;y<12;++y)for(x=0;x<16;++x) {
            unsigned char *at=(unsigned char *)lock.lpSurface+y*lock.lPitch+x*n;
            index=value(x,y,0);actual=0;for(j=0;j<n;++j)actual|=(unsigned long)at[j]<<(j*8);
            if(bits==8)expected=(index==7 && (mode==1 || mode==3))?0:index;
            else if(index==1)expected=bits==16?0x2a4c:0x2b4865;
            else if(mode==1 || mode==3)expected=0;
            else expected=bits==16?0x0862:0x0c0f12;
            CHECK(actual==expected,"independent native pixel and palette-map control");
        }
        CHECK(IDirectDrawSurface_Unlock(image->surface,NULL)==DD_OK,"actual native read lock released");
    }
    if(bits==8)CHECK(IDirectDrawSurface_GetPalette(image->surface,&native_palette)==DDERR_NOPALETTEATTACHED && !native_palette,"original indexed atlas stores palette but does not attach a native palette");
    if(native_palette)IDirectDrawPalette_Release(native_palette);
}
int bh_atlas_image_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext q;BHAtlasContext c;BHAtlasImage *image,*alias;DDPIXELFORMAT pixel;BHMathEnvironment fp;
    BHSceneAllocator allocator;Memory memory;DDSURFACEDESC odd;unsigned long bits,mode,fail_at;int initialized;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    CHECK(sizeof(BHAtlasImage)==0x13ac && offsetof(BHAtlasImage,name)==4 && offsetof(BHAtlasImage,palette)==0x84 && offsetof(BHAtlasImage,pixel)==0x384 && offsetof(BHAtlasImage,rectangle_count)==0x3a4 && offsetof(BHAtlasImage,rectangles)==0x3a8 && offsetof(BHAtlasImage,surface)==0x13a8,"complete original cached atlas record ABI");
    CHECK(fixture("ATLAS.BMP",16,12,0,0) && fixture("ATLAS_ALT.PCX",16,12,1,0) && fixture("ATLAS_BAD.PCX",16,12,0,1) && fixture("ATLAS_MANY.PCX",64,64,2,0) && fixture("ATLAS_ODD.PCX",13,12,0,0),"real literal-name PCX fixtures and malformed/rectangle-limit controls");
    memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"actual native DirectDraw/Direct3D2 offscreen fixture");
    if(view.device)for(bits=8;bits<=32;bits+=8)for(mode=0;mode<4;++mode) {
        memset(&memory,0,sizeof(memory));allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
        initialized=bh_texture_context_init(&q,view.draw,view.device,&allocator);CHECK(initialized,"actual texture/native/allocator owner");if(!initialized)continue;
        q.palette_mode=(int)mode;q.palette_color=0x0c0f12;strcpy(q.prefix,"IGNORED\\");q.search=2;
        memset(&c,0,sizeof(c));CHECK(bh_atlas_context_init(&c,&q,16),"explicit independent original vertical scan step binding");pixel_format(&pixel,bits);
        image=bh_atlas_image_load(&c,"ATLAS.BMP",&pixel);CHECK(image && c.error==BH_TEXTURE_OK && c.last_hresult==DD_OK,"complete actual native8/16/24/32 atlas load");
        if(image) {
            verify(image,bits,mode);q.palette_mode=2;q.palette_color=0;c.scan_step=10000;
            alias=bh_atlas_image_load(&c,"atlas.bmp",NULL);CHECK(alias==image && c.images==image && !image->next,"literal case-insensitive cache identity precedes changed format/palette/scan state");
            CHECK(memory.live==1 && memory.calls==(bits==8?1:2),"only original atlas record persists; highcolor scratch indices released");
            c.scan_step=16;
            alias=bh_atlas_image_load(&c,"ATLAS_ALT.PCX",&pixel);CHECK(alias && alias->rectangle_count==3 && rectangle(alias->rectangles+1,6,1,3,3),"vertical scan examines actual rows when explicitly bound to matching pitch");
        }
        bh_atlas_context_dispose(&c);CHECK(!memory.live && !c.images,"actual native surface and record ownership disposal");
        memset(&c,0,sizeof(c));bh_atlas_context_init(&c,&q,0);
        image=bh_atlas_image_load(&c,"ATLAS_ALT.PCX",&pixel);CHECK(image && image->rectangle_count==2 && rectangle(image->rectangles,1,1,2,3),"original zero scan step repeats first band row rather than silently normalizing to atlas pitch");
        bh_atlas_context_dispose(&c);CHECK(!memory.live,"zero-step owner cleanup");
        memset(&c,0,sizeof(c));bh_atlas_context_init(&c,&q,10000);
        CHECK(!bh_atlas_image_load(&c,"ATLAS.BMP",&pixel) && c.error==BH_TEXTURE_TOPOLOGY && !c.images && !memory.live,"unsafe independent scan step rejects before out-of-bounds column reads");
        c.scan_step=16;CHECK(!bh_atlas_image_load(&c,"ATLAS_BAD.PCX",&pixel) && c.error==BH_TEXTURE_IO && !c.images && !memory.live,"truncated real RLE payload cleans native and owning allocations");
        CHECK(!bh_atlas_image_load(&c,"ATLAS.PCX",&pixel) && c.error==BH_TEXTURE_IO && !c.images,"no inferred BMP-to-PCX rewrite or prefix/search policy");
        c.scan_step=64;CHECK(!bh_atlas_image_load(&c,"ATLAS_MANY.PCX",&pixel) && c.error==BH_TEXTURE_TOPOLOGY && !c.images && !memory.live,"more than512 extracted rectangles safely rejects original record overflow");
        bh_atlas_context_dispose(&c);
        memset(&c,0,sizeof(c));bh_atlas_context_init(&c,&q,bits==8?16:13);
        image=bh_atlas_image_load(&c,"ATLAS_ODD.PCX",&pixel);
        CHECK(image && image->rectangle_count==3 && rectangle(image->rectangles,1,1,2,3) && rectangle(image->rectangles+2,3,6,2,2),"odd width respects actual indexed surface pitch versus tightly packed highcolor scan scratch");
        if(image){memset(&odd,0,sizeof(odd));odd.dwSize=sizeof(odd);CHECK(IDirectDrawSurface_GetSurfaceDesc(image->surface,&odd)==DD_OK && odd.dwWidth==13 && odd.lPitch>=(long)(13*(bits/8)),"actual native odd-width row storage available");}
        bh_atlas_context_dispose(&c);CHECK(!memory.live,"odd-pitch owner cleanup");
        bh_texture_context_dispose(&q);
    }
    if(view.device)for(fail_at=1;fail_at<=2;++fail_at) {
        memset(&memory,0,sizeof(memory));memory.fail_at=fail_at;allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
        initialized=bh_texture_context_init(&q,view.draw,view.device,&allocator);CHECK(initialized,"allocation-failure real native context");if(!initialized)continue;
        memset(&c,0,sizeof(c));bh_atlas_context_init(&c,&q,16);pixel_format(&pixel,16);
        CHECK(!bh_atlas_image_load(&c,"ATLAS.BMP",&pixel) && c.error==BH_TEXTURE_MEMORY && !memory.live && !c.images,"each real allocator failure releases native surface and partial record");
        bh_atlas_context_dispose(&c);bh_texture_context_dispose(&q);
    }
    if(view.device) {
        memset(&memory,0,sizeof(memory));allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
        initialized=bh_texture_context_init(&q,view.draw,view.device,&allocator);CHECK(initialized,"signed-mask real native context");
        if(initialized) {
            q.palette_mode=2;memset(&c,0,sizeof(c));bh_atlas_context_init(&c,&q,16);pixel_format(&pixel,32);
            pixel.dwRBitMask=0xff000000;pixel.dwGBitMask=0xff0000;pixel.dwBBitMask=0xff00;
            image=bh_atlas_image_load(&c,"ATLAS.BMP",&pixel);CHECK(!image && c.error==BH_TEXTURE_FORMAT && !memory.calls && !c.images,"bit31 RGB mask explicitly rejected before allocation; real backend does not support that profile");
            bh_atlas_context_dispose(&c);bh_texture_context_dispose(&q);CHECK(!memory.live,"signed-mask real ownership released");
        }
    }
    bh_scene_view_dispose(&view);bh_vfile_shutdown();remove("ATLAS.BMP");remove("ATLAS_ALT.PCX");remove("ATLAS_BAD.PCX");remove("ATLAS_MANY.PCX");remove("ATLAS_ODD.PCX");
    CHECK(bh_math_environment_supported(),"native atlas checks preserve FP environment");bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"atlas-image\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_ATLAS_IMAGE_STANDALONE
int main(void){unsigned long count;return bh_atlas_image_checkpoint(&count);}
#endif
