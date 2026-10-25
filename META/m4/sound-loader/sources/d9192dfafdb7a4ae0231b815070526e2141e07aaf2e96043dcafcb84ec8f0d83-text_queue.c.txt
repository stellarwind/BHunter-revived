#include "../src/bh_text_queue.h"
#include "../src/bh_atlas_text.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL text-queue: %s\n",s);}} while(0)
static void records(void) {
    BHTextQueue q;BHTextCommand expected;unsigned long i,processed,old_read,old_write;
    char long_text[100];
    bh_text_queue_init(&q);
    CHECK(sizeof(BHTextCommand)==84 && offsetof(BHTextCommand,text)==12 &&
        offsetof(BHTextCommand,next)==80 && bh_text_queue_count(&q)==0,"original84byte layout and fresh owner");
    memset(q.commands,0xa5,sizeof(q.commands));q.read=0x123456fa;q.write=0x789abcfa;
    expected=q.commands[250];expected.kind=BH_TEXT_COMMAND_TEXT;expected.page=NULL;
    expected.position.xy[0]=-1;expected.position.xy[1]=-32768;
    memset(expected.text,0,sizeof(expected.text));memcpy(expected.text,"short",5);
    CHECK(bh_text_queue_text(&q,NULL,"short",65535,32768)==1 && q.write==0x789abcfb &&
        q.read==0x123456fa && !memcmp(q.commands+250,&expected,sizeof(expected)),
        "text producer narrows signed coordinates pads59byte region and preserves unused words/next");
    memset(long_text,'x',sizeof(long_text));long_text[99]=0;
    CHECK(bh_text_queue_text(&q,NULL,long_text,-65537,65536)==1 &&
        q.commands[251].position.xy[0]==-1 && q.commands[251].position.xy[1]==0 &&
        q.commands[251].text[58]=='x' && q.commands[251].text[59]==0,"long text truncates at59 and forces last NUL");
    expected=q.commands[252];expected.kind=BH_TEXT_COMMAND_CLEAR;expected.page=NULL;expected.position.color=0xfedcba98;
    CHECK(bh_text_queue_clear(&q,NULL,0xfedcba98)==1 && !memcmp(q.commands+252,&expected,sizeof(expected)),
        "clear producer retains text padding unused bytes and next");
    CHECK(bh_text_queue_count(&q)==3 && bh_text_queue_drain(&q,NULL,&processed)==1 && processed==3 &&
        q.read==0x123456fd && q.write==0x789abcfd,"FIFO drain uses low-byte indices while retaining both high24 words");
    for(i=0;i<255;++i)CHECK(bh_text_queue_clear(&q,NULL,i)==1,"fill all255 usable ring slots including wrap");
    old_read=q.read;old_write=q.write;expected=q.commands[q.write&255];
    CHECK(bh_text_queue_clear(&q,NULL,99)==0 && q.read==old_read && q.write==old_write &&
        bh_text_queue_count(&q)==255 && !memcmp(&expected,q.commands+(q.write&255),sizeof(expected)),
        "full ring restores write word and retains rejected record");
    CHECK(bh_text_queue_text(&q,NULL,"rejected",0,0)==0 && q.write==old_write,
        "text producer same one-slot-empty full boundary");
    CHECK(bh_text_queue_drain(&q,NULL,&processed)==1 && processed==255 && !bh_text_queue_count(&q) &&
        q.read==0x123456fc && q.write==0x789abcfc,"full wrapped ring consumes exactly255 commands");
    CHECK(bh_text_queue_drain(&q,NULL,&processed)==1 && processed==0,"empty drain has no consumer calls");
    expected=q.commands[q.write&255];old_write=q.write;
    CHECK(bh_text_queue_text(&q,NULL,NULL,0,0)==-1 && q.error==BH_TEXT_QUEUE_ARGUMENT &&
        q.write==old_write && !memcmp(&expected,q.commands+(q.write&255),sizeof(expected)),
        "source null-string guard does not reserve a record");
    q.commands[q.read&255].kind=0;q.write=(q.write&0xffffff00)|((q.write+1)&255);
    CHECK(bh_text_queue_drain(&q,NULL,&processed)==-1 && processed==0 && q.error==BH_TEXT_QUEUE_KIND,
        "source consumer rejects unknown semantic selector without consuming it");
    CHECK(bh_text_queue_text(NULL,NULL,"x",0,0)==-1 && bh_text_queue_clear(NULL,NULL,0)==-1 &&
        bh_text_queue_drain(NULL,NULL,&processed)==-1,"source absent-owner guards");
}
static int fixture(void) {
    unsigned char data[128+32*8+768];unsigned long x,y;FILE *f;
    memset(data,0,sizeof(data));data[0]=10;data[1]=2;data[2]=1;data[3]=8;data[8]=31;data[10]=7;
    for(y=0;y<8;++y)for(x=0;x<32;++x)
        data[128+y*32+x]=(unsigned char)(y>=1 && y<=3 && x>=2 && x<=5?1:
            (y>=1 && y<=3 && x>=10 && x<=13?2:7));
    data[128+32*8+3]=43;data[128+32*8+4]=72;data[128+32*8+5]=101;
    data[128+32*8+6]=255;
    f=fopen("TEXT_QUEUE_ATLAS.PCX","wb");if(!f)return 0;
    if(fwrite(data,1,sizeof(data),f)!=sizeof(data)) {fclose(f);return 0;}
    return fclose(f)==0;
}
static unsigned long pixel(LPDIRECTDRAWSURFACE surface,unsigned long x,unsigned long y) {
    DDSURFACEDESC lock;unsigned long value=0,i,bytes;unsigned char *p;
    memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
    if(IDirectDrawSurface_Lock(surface,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)!=DD_OK)return 0xffffffffu;
    bytes=(lock.ddpfPixelFormat.dwRGBBitCount+7)/8;p=(unsigned char*)lock.lpSurface+y*lock.lPitch+x*bytes;
    for(i=0;i<bytes;++i)value|=(unsigned long)p[i]<<(8*i);
    IDirectDrawSurface_Unlock(surface,NULL);return value;
}
static void native_consumers(void) {
    BHSceneView view;BHTextureContext textures;BHAtlasContext atlas;BHAtlasPage *page;
    BHTextQueue queue;BHTextCommand command;unsigned long processed,glyph,actual,width;
    int initialized;DDSURFACEDESC held;
    CHECK(fixture(),"actual two-glyph atlas PCX fixture");memset(&view,0,sizeof(view));
    CHECK(bh_scene_view_init(&view,64,64),"real native offscreen texture backend");
    if(view.device) {
        initialized=bh_texture_context_init(&textures,view.draw,view.device,NULL);
        CHECK(initialized && bh_texture_formats(&textures),"real recovered texture owner and formats");
        if(initialized) {
            textures.palette_mode=2;textures.frame=1;textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
            memset(&atlas,0,sizeof(atlas));CHECK(bh_atlas_context_init(&atlas,&textures,32),"actual atlas ownership binding");
            page=bh_atlas_page_create(&atlas,"TEXT_QUEUE_ATLAS.PCX",32);
            CHECK(page && page->atlas->rectangle_count==2,"real atlas rectangles and native page constructor");
            if(page) {
                width=page->atlas->rectangles[1].width;
                glyph=pixel(page->atlas->surface,page->atlas->rectangles[1].x,page->atlas->rectangles[1].y);
                bh_text_queue_init(&queue);
                CHECK(bh_text_queue_clear(&queue,page,0)==1 && bh_text_queue_text(&queue,page,"!!",2,3)==1,
                    "queue actual clear then text consumers");
                CHECK(bh_text_queue_drain(&queue,&atlas,&processed)==1 && processed==2 &&
                    page->cursor_x==(long)(2+width*2) && page->cursor_y==3 && page->left==2,
                    "FIFO execution calls real cursor and complete glyph owner");
                actual=pixel(page->surface,2,3);
                CHECK(actual==glyph && glyph!=0 && glyph!=0xffffffffu && pixel(page->surface,2+width,3)==glyph &&
                    pixel(page->surface,0,0)==0,"actual DirectDraw glyph blits copy both queued glyphs after clear");
                memset(&command,0,sizeof(command));command.page=page;command.position.xy[0]=1;
                command.position.xy[1]=2;command.text[0]='!';command.next=0x12345678;
                CHECK(bh_text_queue_execute_text(&command)==0x12345678 && page->cursor_x==(long)(1+width) &&
                    page->cursor_y==2 && pixel(page->surface,1,2)==glyph,
                    "text consumer returns original next after real signed-coordinate cursor and glyph copy");
                command.position.color=0;command.next=0xfedcba98;
                CHECK(bh_text_queue_execute_clear(&atlas,&command)==0xfedcba98 &&
                    pixel(page->surface,1,2)==0 && page->cursor_x==0 && page->cursor_y==0,
                    "clear consumer returns next after actual native clear and cursor reset");
                command.position.xy[0]=-32768;command.position.xy[1]=-32768;
                CHECK(bh_text_queue_execute_text(&command)==0xfedcba98 &&
                    page->left==-32768 && page->cursor_x==(long)(-32768+(long)width) && page->cursor_y==-32768,
                    "text consumer sign-extends negative16-bit coordinates despite ignored native blit failure");
                memset(&held,0,sizeof(held));held.dwSize=sizeof(held);
                CHECK(IDirectDrawSurface_Lock(page->surface,NULL,&held,0,NULL)==DD_OK,
                    "actual held native page lock for failed clear consumption");
                if(held.lpSurface) {
                    CHECK(bh_text_queue_clear(&queue,page,0)==1 &&
                        bh_text_queue_drain(&queue,&atlas,&processed)==1 && processed==1 &&
                        textures.error==BH_TEXTURE_BACKEND && queue.error==BH_TEXT_QUEUE_OK &&
                        !page->cursor_x && !page->cursor_y,
                        "ring consumes real failed native clear and preserves original cursor-reset poststate");
                    CHECK(IDirectDrawSurface_Unlock(page->surface,NULL)==DD_OK,"release real held clear fixture lock");
                }
                CHECK(bh_atlas_page_dispose(&atlas,page),"real native page lifecycle cleanup");
            }
            bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&textures);
        }
    }
    bh_scene_view_dispose(&view);bh_vfile_shutdown();remove("TEXT_QUEUE_ATLAS.PCX");
}
int bh_text_queue_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 1;
    records();native_consumers();bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"text-queue\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_TEXT_QUEUE_STANDALONE
int main(void) {return bh_text_queue_checkpoint(NULL);}
#endif
