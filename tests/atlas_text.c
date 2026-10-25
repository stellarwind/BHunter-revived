#include "../src/bh_atlas_text.h"
#include "../src/bh_atlas_glyph.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL atlas text %d: %s\n",line,label);}}
static int fixture(void) {
    unsigned char data[128+192+768];unsigned long x,y;FILE *file;
    memset(data,0,sizeof(data));data[0]=10;data[1]=2;data[2]=1;data[3]=8;data[8]=15;data[10]=11;
    for(y=0;y<12;++y)for(x=0;x<16;++x)data[128+y*16+x]=(unsigned char)(
        (y>=1 && y<=3 && ((x>=1 && x<=2)||(x>=6 && x<=8))) || (y>=6 && y<=7 && x>=3 && x<=4)?1:7);
    data[320+3]=43;data[320+4]=72;data[320+5]=101;data[320+21]=12;data[320+22]=15;data[320+23]=18;
    file=fopen("TEXT_ATLAS.PCX","wb");if(!file)return 0;
    if(fwrite(data,1,sizeof(data),file)!=sizeof(data)){fclose(file);return 0;}return !fclose(file);
}
static int context(BHSceneView *view,BHTextureContext *texture,BHAtlasContext *atlas,unsigned long step) {
    unsigned long i;DDPIXELFORMAT pixel;
    if(!bh_texture_context_init(texture,view->draw,view->device,NULL) || !bh_texture_formats(texture))return 0;
    texture->palette_mode=2;texture->slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;texture->frame=1;
    memset(&pixel,0,sizeof(pixel));pixel.dwSize=sizeof(pixel);pixel.dwFlags=DDPF_RGB;pixel.dwRGBBitCount=16;
    pixel.dwRBitMask=0xf800;pixel.dwGBitMask=0x7e0;pixel.dwBBitMask=0x1f;
    for(i=0;i<9;++i)texture->formats[i].pixel=pixel;
    if(!bh_texture_bind_display(texture,&pixel))return 0;
    memset(atlas,0,sizeof(*atlas));return bh_atlas_context_init(atlas,texture,step);
}
static unsigned short pixel(const DDSURFACEDESC *lock,unsigned long x,unsigned long y) {
    unsigned short value;memcpy(&value,(const unsigned char*)lock->lpSurface+y*lock->lPitch+x*2,2);return value;
}
static void expected_pixels(BHAtlasPage *page) {
    DDSURFACEDESC lock;unsigned long x,y;unsigned short expected;
    memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
    CHECK(IDirectDrawSurface_Lock(page->surface,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"read actual DirectDraw glyph destination");
    if(lock.lpSurface) {
        for(y=0;y<32;++y)for(x=0;x<32;++x) {
            expected=((x<3 && y<3)||(x>=3 && x<5 && y<2)||(x<3 && y>=3 && y<6))?0x2a4c:0;
            CHECK(pixel(&lock,x,y)==expected,"independent page pixels prove actual glyph0-excluded native blits and newline placement");
        }
        CHECK(IDirectDrawSurface_Unlock(page->surface,NULL)==DD_OK,"actual glyph destination unlock");
    }
}
static void metrics(void) {
    BHAtlasPage page,before;BHAtlasImage atlas;unsigned char high[]={33,10,34,13,255,32,0};
    unsigned char *long_text;unsigned long i,bits;long expected;
    memset(&page,0,sizeof(page));memset(&atlas,0,sizeof(atlas));page.atlas=&atlas;atlas.rectangle_count=3;
    atlas.rectangles[0].width=2;atlas.rectangles[1].width=3;atlas.rectangles[2].width=5;
    page.cursor_x=71;page.cursor_y=-9;page.line_height=17;before=page;
    CHECK(bh_atlas_page_text_width(&page,high)==10 && !memcmp(&page,&before,sizeof(page)),"width ignores newline control and unsupported high byte without cursor mutation");
    atlas.rectangles[1].width=0xfffe;
    CHECK(bh_atlas_page_text_width(&page,(const unsigned char*)"! !")==-2,"font widths sign-extend16bit including spaces");
    CHECK(!bh_atlas_page_text_width(0,high) && !bh_atlas_page_text_width(&page,0),"original null width cases");
    long_text=(unsigned char*)malloc(70001);CHECK(long_text!=NULL,"bounded long metric fixture allocation");
    if(long_text) {
        for(i=0;i<70000;++i)long_text[i]=33;long_text[70000]=0;atlas.rectangles[1].width=32767;
        bits=70000UL*32767UL;memcpy(&expected,&bits,4);
        CHECK(bh_atlas_page_text_width(&page,long_text)==expected && expected<0,"text total retains original32bit wrapped signed return");free(long_text);
    }
    CHECK(!bh_atlas_page_image(0) && !bh_atlas_page_image(&page),"image getter returns borrowed null image identity");
}
static void native_controls(BHAtlasContext *atlas,BHTextureContext *texture,BHAtlasPage *page) {
    BHAtlasRectangle saved;BHTextureSlot *resident;long before;RECT source,destination;HRESULT result;
    CHECK(page->atlas->rectangle_count==3 && page->atlas->rectangles[0].width==2 && page->atlas->rectangles[1].width==3 && page->atlas->rectangles[2].width==2,"actual decoded three-glyph atlas metrics");
    CHECK(bh_atlas_page_clear(atlas,page,0),"real page clear before glyph copy");
    CHECK(bh_atlas_page_text_width(page,(const unsigned char*)"!\" \n! ")==12 && bh_atlas_page_text(page,(const unsigned char*)"!\" \n! ")==12,"original draw return sums all advances across lines");
    CHECK(page->cursor_x==5 && page->cursor_y==3 && page->left==0 && page->line_height==3,"glyph space and newline cursor contract");expected_pixels(page);
    CHECK(bh_atlas_page_image(page)==page->image,"image getter publishes exact real native page image");
    CHECK(bh_texture_preallocate(texture) && bh_texture_resident(texture,page->image)!=0,"real native residency for glyph dirty contract");resident=page->image->resident;
    if(resident) {
        resident->pixels_dirty=0;resident->palette_dirty=7;
        CHECK(bh_atlas_page_text(page,(const unsigned char*)"")==0 && resident->pixels_dirty==1 && resident->palette_dirty==7,"empty nonnull text marks resident pixels dirty preserving palette state");
        resident->pixels_dirty=0;before=page->cursor_x;
        CHECK(!bh_atlas_page_text(page,0) && !resident->pixels_dirty && page->cursor_x==before,"null text does not dirty or move cursor");
    }
    page->left=2;page->cursor_x=30;page->cursor_y=0;page->line_height=4;
    CHECK(bh_atlas_page_text(page,(const unsigned char*)"!")==3 && page->cursor_x==5 && page->cursor_y==4 && page->line_height==3,"strict glyph overflow wraps to left using previous line height");
    page->left=2;page->cursor_x=29;page->cursor_y=0;page->line_height=4;
    CHECK(bh_atlas_page_text(page,(const unsigned char*)"!")==3 && page->cursor_x==32 && page->cursor_y==0 && page->line_height==4,"glyph exactly at right edge does not wrap");
    page->left=2;page->cursor_x=31;page->cursor_y=0;page->line_height=4;
    CHECK(bh_atlas_page_text(page,(const unsigned char*)" ")==2 && page->cursor_x==33 && page->cursor_y==0,"spaces advance beyond right edge without wrapping");
    page->left=5;page->cursor_x=9;page->cursor_y=10;page->line_height=7;
    CHECK(!bh_atlas_page_text(page,(const unsigned char*)"\n\n") && page->cursor_x==5 && page->cursor_y==24 && page->line_height==7,"newlines retain line height until subsequent glyph or leading space");
    saved=page->atlas->rectangles[1];page->atlas->rectangles[1].x=0xffff;
    source.left=-1;source.top=1;source.right=2;source.bottom=4;destination.left=0;destination.top=0;destination.right=3;destination.bottom=3;
    result=IDirectDrawSurface_Blt(page->surface,&destination,page->atlas->surface,&source,DDBLT_WAIT,NULL);
    CHECK(result!=DD_OK,"actual native invalid signed source rectangle is rejected positive control");
    bh_atlas_page_cursor(page,0,0);if(resident)resident->pixels_dirty=0;
    CHECK(bh_atlas_page_text(page,(const unsigned char*)"!")==3 && page->cursor_x==3 && page->line_height==3 && (!resident || resident->pixels_dirty==1),"ignored native blit failure still updates original cursor total and dirty flag");
    page->atlas->rectangles[1]=saved;page->atlas->rectangles[1].width=0xfffe;page->atlas->rectangles[1].height=0xfffd;
    bh_atlas_page_cursor(page,0,0);
    CHECK(bh_atlas_page_text(page,(const unsigned char*)"!")==-2 && page->cursor_x==-2 && page->line_height==0,"signed negative glyph width and height keep original failed-blit cursor behavior");
    page->atlas->rectangles[1]=saved;saved=page->atlas->rectangles[0];page->atlas->rectangles[0].width=32767;
    page->cursor_x=0x7ffffff0;page->left=0;page->cursor_y=0;page->line_height=0;
    CHECK(bh_atlas_page_text(page,(const unsigned char*)" ")==32767 && page->cursor_x<0,"cursor addition retains32bit wrap without native blit");
    page->atlas->rectangles[0]=saved;
}
static void real_font(const char *archive,BHSceneView *view) {
    BHTextureContext texture;BHAtlasContext atlas;BHAtlasPage *page;DDSURFACEDESC source,destination;BHAtlasRectangle glyph;unsigned long x,y;long width;
    if(!archive)return;
    CHECK(bh_vfile_mount(archive),"mount pinned archive as immutable font data");
    if(!context(view,&texture,&atlas,320)){CHECK(0,"actual archive font native contexts");return;}
    page=bh_atlas_page_create(&atlas,"TEXTURES\\TVFONT.PCX",128);CHECK(page!=NULL,"actual decoded archiveTVFONT page construction");
    if(page) {
        CHECK(page->atlas->rectangle_count>33,"real archive font contains glyphA");
        if(page->atlas->rectangle_count>33) {
            glyph=page->atlas->rectangles[33];width=(short)glyph.width;
            CHECK(bh_atlas_page_clear(&atlas,page,0),"archive font clear");bh_atlas_page_cursor(page,8,8);
            CHECK(bh_atlas_page_text_width(page,(const unsigned char*)"A")==width && bh_atlas_page_text(page,(const unsigned char*)"A")==width,"real font metrics compose actual native glyph copy");
            memset(&source,0,sizeof(source));source.dwSize=sizeof(source);memset(&destination,0,sizeof(destination));destination.dwSize=sizeof(destination);
            CHECK(IDirectDrawSurface_Lock(page->atlas->surface,NULL,&source,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"read actual archive atlas native glyph pixels");
            CHECK(IDirectDrawSurface_Lock(page->surface,NULL,&destination,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"read copied archive glyph native page pixels");
            if(source.lpSurface && destination.lpSurface)for(y=0;y<glyph.height;++y)for(x=0;x<glyph.width;++x)
                CHECK(pixel(&destination,8+x,8+y)==pixel(&source,glyph.x+x,glyph.y+y),"actual archive glyph pixels equal recovered native page copy");
            if(destination.lpSurface)IDirectDrawSurface_Unlock(page->surface,NULL);if(source.lpSurface)IDirectDrawSurface_Unlock(page->atlas->surface,NULL);
        }
        CHECK(bh_atlas_page_dispose(&atlas,page),"archive font page lifetime closes");
    }
    bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&texture);
}
int bh_atlas_text_checkpoint(const char *archive,unsigned long *count) {
    BHSceneView view;BHTextureContext texture;BHAtlasContext atlas;BHAtlasPage *page;BHMathEnvironment environment;
    checks=0;failures=0;if(!bh_math_environment_enter(&environment))return 1;metrics();
    CHECK(fixture(),"actual glyph PCX fixture");memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real DirectDraw/Direct3D2 native surfaces");
    if(view.device) {
        if(context(&view,&texture,&atlas,16)) {
            page=bh_atlas_page_create(&atlas,"TEXT_ATLAS.PCX",32);CHECK(page!=NULL,"real decoded glyph atlas and page owners");
            if(page){native_controls(&atlas,&texture,page);CHECK(bh_atlas_page_dispose(&atlas,page),"native glyph page lifetime closes");}
            bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&texture);
        } else CHECK(0,"native glyph contexts");
        real_font(archive,&view);
    }
    CHECK(!bh_atlas_page_text(0,(const unsigned char*)"!"),"original null draw page");
    bh_scene_view_dispose(&view);bh_vfile_shutdown();remove("TEXT_ATLAS.PCX");bh_math_environment_leave(&environment);if(count)*count=checks;
    printf("{\"scenario\":\"atlas-text\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_ATLAS_TEXT_STANDALONE
int main(int argc,char **argv){unsigned long count;return bh_atlas_text_checkpoint(argc>1?argv[1]:NULL,&count);}
#endif
