#include "../src/bh_atlas_glyph.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<15)printf("FAIL atlas-clear-cursor %d: %s\n",line,label);}}
static int fixture(void) {
    unsigned char data[128+192+768];unsigned long x,y;FILE *f;
    memset(data,0,sizeof(data));data[0]=10;data[1]=2;data[2]=1;data[3]=8;data[8]=15;data[10]=11;
    for(y=0;y<12;++y)for(x=0;x<16;++x)data[128+y*16+x]=(unsigned char)((y>=1 && y<=3 && x>=2 && x<=5)?1:7);
    data[323]=10;data[324]=20;data[325]=30;data[326]=20;data[327]=20;data[328]=30;data[341]=12;data[342]=15;data[343]=18;
    f=fopen("GLYPH_ATLAS.PCX","wb");if(!f)return 0;if(fwrite(data,1,sizeof(data),f)!=sizeof(data)){fclose(f);return 0;}return !fclose(f);
}
static void pixel_format(DDPIXELFORMAT *p,unsigned long bits) {
    memset(p,0,sizeof(*p));p->dwSize=sizeof(*p);p->dwFlags=bits==8?DDPF_RGB|DDPF_PALETTEINDEXED8:DDPF_RGB;p->dwRGBBitCount=bits;
    if(bits==16){p->dwRBitMask=0xf800;p->dwGBitMask=0x7e0;p->dwBBitMask=0x1f;}
    else if(bits>8){p->dwRBitMask=0xff0000;p->dwGBitMask=0xff00;p->dwBBitMask=0xff;}
}
static void pixels(LPDIRECTDRAWSURFACE surface,unsigned long bits,unsigned long expected) {
    DDSURFACEDESC lock;unsigned long x,y,i,actual;
    memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
    CHECK(IDirectDrawSurface_Lock(surface,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"read actual native cleared texture surface");
    if(lock.lpSurface){for(y=0;y<32;++y)for(x=0;x<32;++x){const unsigned char *at=(const unsigned char *)lock.lpSurface+y*lock.lPitch+x*(bits/8);actual=0;for(i=0;i<bits/8;++i)actual|=(unsigned long)at[i]<<(i*8);CHECK(actual==expected,"every original size-square pixel uses actual native row pitch");}CHECK(IDirectDrawSurface_Unlock(surface,NULL)==DD_OK,"native verification lock released");}
}
int bh_atlas_glyph_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext q;BHAtlasContext atlas;BHAtlasPage *page;BHMathEnvironment fp;
    DDSURFACEDESC held;DDPIXELFORMAT saved_pixel;unsigned long bits,expected,flags;HRESULT busy;int initialized;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    CHECK(bh_atlas_page_clear(NULL,NULL,0),"original null clear no-op");bh_atlas_page_cursor(NULL,-7,9);
    CHECK(fixture(),"real native clear/nearest palette fixture");memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"actual offscreen RGB native backend");
    if(view.device)for(bits=8;bits<=32;bits+=8) {
        initialized=bh_texture_context_init(&q,view.draw,view.device,NULL);CHECK(initialized && bh_texture_formats(&q),"real texture format/native owner");if(!initialized)continue;
        q.palette_mode=2;q.frame=1;q.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;pixel_format(&q.formats[0].pixel,bits);
        memset(&atlas,0,sizeof(atlas));CHECK(bh_atlas_context_init(&atlas,&q,16),"real recovered atlas owner binding");
        page=bh_atlas_page_create(&atlas,"GLYPH_ATLAS.PCX",32);CHECK(page!=NULL,"actual recovered page constructor8/16/24/32");
        if(page) {
            bh_atlas_page_cursor(page,-17,23);CHECK(page->left==-17 && page->cursor_x==-17 && page->cursor_y==23 && !page->line_height,"cursor accepts raw negative coordinates and sets original left origin");
            page->line_height=19;bh_atlas_page_cursor(page,71,-99);CHECK(page->left==71 && page->cursor_x==71 && page->cursor_y==-99 && !page->line_height,"cursor replaces position/origin and resets line height");
            expected=bits==8?1:(bits==16?0x2a4c:0x2b4865);
            CHECK(bh_atlas_page_clear(&atlas,page,bits==8?0x0f141e:0x2b4865) && page->packed_color==expected,"earliest Manhattan palette tie or independent native RGB conversion");
            CHECK(!page->left && !page->cursor_x && !page->cursor_y && !page->line_height,"clear resets every original cursor field");pixels(page->surface,bits,expected);
            if(bits==8) {
                /* Same full32bit color deliberately keeps cached conversion,
                   even when atlas palette changes underneath it. */
                page->atlas->palette[3]=255;page->atlas->palette[4]=255;page->atlas->palette[5]=255;
                CHECK(bh_atlas_page_clear(&atlas,page,0x0f141e) && page->packed_color==1,"identical color retains original cached palette conversion");
                CHECK(bh_atlas_page_clear(&atlas,page,0xaa0f141e) && page->packed_color==2 && page->last_color==0xaa0f141e,"full32bit color cache detects high-byte change despite same RGB");pixels(page->surface,bits,2);
            }
            if(bits==32) {
                saved_pixel=page->atlas->pixel;page->atlas->pixel.dwRBitMask=0xff000000;page->atlas->pixel.dwGBitMask=0xff0000;page->atlas->pixel.dwBBitMask=0xff00;
                CHECK(bh_atlas_page_clear(&atlas,page,0xff0000) && page->packed_color==0x01000000,"original signed mask-division loop retained for bit31 RGB mask");pixels(page->surface,bits,0x01000000);page->atlas->pixel=saved_pixel;
            }
            if(bits<=16) {
                CHECK(bh_texture_preallocate(&q) && bh_texture_resident(&q,page->image)!=0 && page->image->resident,"real residency slot for original dirty propagation");
                if(page->image->resident) {
                    page->image->resident->pixels_dirty=0;flags=page->image->flags;
                    memset(&held,0,sizeof(held));held.dwSize=sizeof(held);
                    CHECK(IDirectDrawSurface_Lock(page->surface,NULL,&held,0,NULL)==DD_OK,"hold real native lock to exercise original failed-Lock poststate");
                    if(held.lpSurface) {
                        bh_atlas_page_cursor(page,17,29);page->line_height=33;
                        CHECK(!bh_atlas_page_clear(&atlas,page,0x14201e) && q.error==BH_TEXTURE_BACKEND,"actual native busy lock error is observable");busy=q.last_hresult;
                        CHECK(busy!=DD_OK && !page->left && !page->cursor_x && !page->cursor_y && !page->line_height && page->last_color==0x14201e && page->image->resident->pixels_dirty==1 && page->image->flags==flags,"failed Lock still updates color cache, resets cursor and marks only resident pixel dirty flag");
                        CHECK(IDirectDrawSurface_Unlock(page->surface,NULL)==DD_OK,"release actual held busy fixture lock");
                        printf("{\"native_busy_hresult\":%ld,\"pixel_bits\":%lu}\n",(long)busy,bits);
                    }
                    CHECK(bh_atlas_page_clear(&atlas,page,0x14201e) && q.error==BH_TEXTURE_OK && page->image->resident->pixels_dirty==1,"successful retry uses cached conversion and still invalidates real residency");
                    expected=bits==8?2:0x1103;pixels(page->surface,bits,expected);
                    CHECK(bh_texture_resident(&q,page->image)!=0 && !page->image->resident->pixels_dirty,"real residency consumes clear invalidation and updates actual slot");pixels(page->image->resident->surface,bits,expected);
                }
            }
            CHECK(bh_atlas_page_dispose(&atlas,page) && !q.images,"explicit page lifetime adapter after clear/cursor ownership");
        }
        bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&q);
    }
    bh_scene_view_dispose(&view);bh_vfile_shutdown();remove("GLYPH_ATLAS.PCX");CHECK(bh_math_environment_supported(),"native clear/cursor operations preserve FP profile");bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"atlas-clear-cursor\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_ATLAS_GLYPH_STANDALONE
int main(void){unsigned long count;return bh_atlas_glyph_checkpoint(&count);}
#endif
