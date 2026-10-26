#include "../src/bh_atlas_wide.h"
#include "../src/bh_atlas_text.h"
#include "../src/bh_message_panel.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<20)printf("FAIL atlas-wide %d: %s\n",__LINE__,s);}}while(0)
static unsigned short code_at(const unsigned char *p){unsigned short u;memcpy(&u,p,2);return u;}
static void metrics(void){
 BHAtlasImage atlas;BHAtlasPage page,before;unsigned short text[]={32,33,255,256,543,544,65535,10,13,0};unsigned short *long_text;unsigned long i,bits;long wrapped;
 memset(&atlas,0,sizeof(atlas));memset(&page,0,sizeof(page));page.atlas=&atlas;atlas.rectangle_count=512;
 atlas.rectangles[0].width=1;atlas.rectangles[1].width=2;atlas.rectangles[223].width=3;atlas.rectangles[224].width=4;atlas.rectangles[511].width=5;
 page.cursor_x=-23;page.cursor_y=71;page.line_height=-1;before=page;
 CHECK(bh_atlas_page_wide_width(&page,text)==15&&!memcmp(&page,&before,sizeof(page)),"full unsigned16 domain, last glyph543, skipped controls/unknowns, immutable cursor");
 atlas.rectangles[224].width=0xfffe;CHECK(bh_atlas_page_wide_width(&page,text)==9,"signed16metric high glyph width");
 CHECK(!bh_atlas_page_wide_width(NULL,text)&&!bh_atlas_page_wide_width(&page,NULL),"original missing metric arguments");
 long_text=(unsigned short*)malloc(70001*2);CHECK(long_text!=NULL,"long real16bit input allocation");
 if(long_text){for(i=0;i<70000;++i)long_text[i]=256;long_text[70000]=0;atlas.rectangles[224].width=32767;bits=70000UL*32767UL;memcpy(&wrapped,&bits,4);CHECK(bh_atlas_page_wide_width(&page,long_text)==wrapped&&wrapped<0,"signed returned total preserves32bit wrap");free(long_text);}
 atlas.rectangle_count=513;CHECK(!bh_atlas_page_wide_width(&page,text),"source rectangle-count guard");
}
static void queue_cases(void){
 BHTextQueue q,saved;BHTextCommand *command;BHAtlasPage page;unsigned short text[80];unsigned long i,j,index,previous;unsigned char expected[84];unsigned short word;long kind=BH_TEXT_COMMAND_WIDE;
 memset(&page,0,sizeof(page));for(i=0;i<79;++i)text[i]=(unsigned short)(256+i);text[79]=0;
 memset(&q,0xa5,sizeof(q));q.read=q.write=0x123456fe;q.error=BH_TEXT_QUEUE_OK;saved=q;
 CHECK(bh_text_queue_wide(&q,&page,text,0x1234ffff,0x98768001)==1&&q.write==0x123456ff&&q.read==0x123456fe,"actual producer retains high24 ring counter and lowbyte step");
 command=q.commands+254;memcpy(expected,&saved.commands[254],84);memcpy(expected,&kind,4);memcpy(expected+4,&command->page,4);word=0xffff;memcpy(expected+8,&word,2);word=0x8001;memcpy(expected+10,&word,2);memcpy(expected+12,text,58);word=0;memcpy(expected+70,&word,2);
 CHECK(command->page==&page&&!memcmp(command,expected,84),"all84 command bytes:29wide units, NUL, coords and untouched unused/next");
 for(i=0;i<256;++i)if(i!=254)CHECK(!memcmp(q.commands+i,saved.commands+i,84),"producer changes no other ring record");
 CHECK(command->position.xy[0]==-1&&command->position.xy[1]==-32767&&code_at(command->text+56)==284&&code_at(command->text+58)==0,"signed low16coordinates and exact29unit truncation");
 q.read=0x123456ff;text[0]=257;text[1]=0;saved=q;CHECK(bh_text_queue_wide(&q,NULL,text,7,9)&&q.write==0x12345600,"byte255 wraps without carry into high24");command=q.commands+255;
 CHECK(code_at(command->text)==257&&code_at(command->text+2)==0&&!memcmp(command->text+4,saved.commands[255].text+4,56)&&command->unused[0]==0xa5a5a5a5&&command->next==0xa5a5a5a5,"short input preserves prior tail instead of ANSI padding");
 bh_text_queue_init(&q);memset(q.commands,0x3c,sizeof(q.commands));text[0]=33;text[1]=0;
 for(i=0;i<255;++i)CHECK(bh_text_queue_wide(&q,NULL,text,i,-(long)i)==1,"all255 original queue slots fill");saved=q;
 CHECK(bh_text_queue_wide(&q,NULL,text,0,0)==0&&q.write==saved.write&&q.error==BH_TEXT_QUEUE_FULL&&!memcmp(q.commands,saved.commands,sizeof(q.commands)),"full ring restores whole writeword and retains every command byte");
 previous=q.write;CHECK(bh_text_queue_wide(&q,NULL,NULL,0,0)==-1&&q.write==previous&&q.error==BH_TEXT_QUEUE_ARGUMENT,"source NULL text rejection before reserve");
 CHECK(bh_text_queue_wide(NULL,NULL,text,0,0)==-1,"source NULL queue guard");
 /* Independent byte-range checks across untruncated input lengths0..29. */
 for(i=0;i<=29;++i){memset(&q,0x7b,sizeof(q));q.read=q.write=0;q.error=BH_TEXT_QUEUE_OK;for(j=0;j<i;++j)text[j]=(unsigned short)(0x200+j);text[i]=0;
  CHECK(bh_text_queue_wide(&q,NULL,text,0,0)==1,"all bounded wide copy lengths");for(j=0;j<i;++j)CHECK(code_at(q.commands[0].text+j*2)==text[j],"each16bit copy retains high byte");CHECK(code_at(q.commands[0].text+i*2)==0,"NUL immediately follows final unit");for(j=(i+1)*2;j<60;++j)CHECK(q.commands[0].text[j]==0x7b,"tail bytes never padded");}
 index=0;(void)index;
}
static int fixture(void){
 unsigned char *data;FILE *file;unsigned long x,y,g,c,r;int result;
 data=(unsigned char*)calloc(1,128+100*75+768);if(!data)return 0;data[0]=10;data[1]=2;data[2]=1;data[3]=8;data[8]=99;data[10]=74;data[65]=1;data[66]=100;
 memset(data+128,7,100*75);
 for(g=0;g<300;++g){c=g%20;r=g/20;for(y=1+r*5;y<4+r*5;++y)for(x=1+c*5;x<1+c*5+(g%3)+1;++x)data[128+y*100+x]=(unsigned char)(1+g%6);}
 for(g=1;g<=7;++g){data[128+7500+g*3]=(unsigned char)(17*g);data[128+7500+g*3+1]=(unsigned char)(23*g);data[128+7500+g*3+2]=(unsigned char)(31*g);}
 file=fopen("WIDE_ATLAS.PCX","wb");if(!file){free(data);return 0;}result=fwrite(data,1,128+7500+768,file)==128+7500+768;if(fclose(file))result=0;free(data);return result;
}
static int context(BHSceneView *view,BHTextureContext *texture,BHAtlasContext *atlas,unsigned long step){unsigned long i;DDPIXELFORMAT pixel;
 if(!bh_texture_context_init(texture,view->draw,view->device,NULL)||!bh_texture_formats(texture))return 0;
 texture->palette_mode=2;texture->slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;texture->frame=1;
 memset(&pixel,0,sizeof(pixel));pixel.dwSize=sizeof(pixel);pixel.dwFlags=DDPF_RGB;pixel.dwRGBBitCount=16;pixel.dwRBitMask=0xf800;pixel.dwGBitMask=0x7e0;pixel.dwBBitMask=31;
 for(i=0;i<9;++i)texture->formats[i].pixel=pixel;
 if(!bh_texture_bind_display(texture,&pixel))return 0;memset(atlas,0,sizeof(*atlas));return bh_atlas_context_init(atlas,texture,step);
}
static unsigned short pixel(const DDSURFACEDESC *d,unsigned long x,unsigned long y){unsigned short v;memcpy(&v,(unsigned char*)d->lpSurface+y*d->lPitch+x*2,2);return v;}
static unsigned short packed(unsigned long n){return (unsigned short)(((17*n)>>3)<<11|((23*n)>>2)<<5|((31*n)>>3));}
static void native_controls(BHAtlasContext *atlas,BHTextureContext *texture,BHAtlasPage *page){
 unsigned short text[]={256,331,32,33,10,257,0},one[]={256,0},empty[]={0};BHTextQueue q;DDSURFACEDESC lock;unsigned long x,y,processed;unsigned short value;BHTextureSlot *resident;BHAtlasRectangle saved;RECT src,dst;HRESULT result;long width;
 CHECK(page->atlas->rectangle_count==300,"real decoded atlas has300glyphs including codesabove255");
 CHECK(bh_atlas_page_clear(atlas,page,0),"real native page clear");bh_text_queue_init(&q);q.commands[0].next=0x87654321;
 CHECK(bh_text_queue_wide(&q,page,text,0,0)&&q.commands[0].kind==BH_TEXT_COMMAND_WIDE,"actual wide producer with real page");
 CHECK(bh_atlas_page_wide_width(page,text)==10&&bh_text_queue_execute_wide(q.commands)==0x87654321,"actual producer-to-executor->native highglyph blits returnscurrent next");
 CHECK(bh_atlas_page_clear(atlas,page,0),"clear actual native page before shared FIFO integration");page->cursor_x=15;page->cursor_y=19;processed=0;
 CHECK(bh_message_text_drain(&q,atlas,&processed)==1&&processed==1&&q.read==q.write&&q.error==BH_TEXT_QUEUE_OK&&q.commands[0].next==0x87654321,"actual producer->shared FIFO->wide executor drains one source record ignoring originalnext sentinel");
 CHECK(page->cursor_x==1&&page->cursor_y==3&&page->left==0&&page->line_height==3,"independent wide glyph/space/newline cursor anchors");
 memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);result=IDirectDrawSurface_Lock(page->surface,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL);CHECK(result==DD_OK&&lock.lpSurface,"read only owned native page pixels");
 if(result==DD_OK&&lock.lpSurface){for(y=0;y<32;++y)for(x=0;x<32;++x){value=0;if(y<3){if(x<3)value=packed(3);else if(x<6)value=packed(6);else if(x>=7&&x<9)value=packed(2);}else if(y<6&&x==0)value=packed(4);CHECK(pixel(&lock,x,y)==value,"independent pixel masks prove genuine16bit glyph selection not truncatedbyte lookup");}CHECK(IDirectDrawSurface_Unlock(page->surface,NULL)==DD_OK,"real native page unlock");}
 CHECK(bh_texture_preallocate(texture)&&bh_texture_resident(texture,page->image)!=0,"real resident image receives subsequent dirtiness");resident=page->image->resident;
 if(resident){resident->pixels_dirty=0;resident->palette_dirty=9;CHECK(!bh_atlas_page_wide_text(page,empty)&&resident->pixels_dirty==1&&resident->palette_dirty==9,"empty wide input dirties actual resident pixels only");resident->pixels_dirty=0;x=page->cursor_x;CHECK(!bh_atlas_page_wide_text(page,NULL)&&!resident->pixels_dirty&&page->cursor_x==(long)x,"null input retains actual native residency and cursor");}
 page->left=2;page->cursor_x=30;page->cursor_y=0;page->line_height=4;
 CHECK(bh_atlas_page_wide_text(page,one)==3&&page->cursor_x==5&&page->cursor_y==4&&page->line_height==3,"strict overflowing highglyph wraps using previous height");
 page->left=2;page->cursor_x=29;page->cursor_y=0;page->line_height=4;CHECK(bh_atlas_page_wide_text(page,one)==3&&page->cursor_x==32&&page->cursor_y==0,"equal right edge does not wrap");
 saved=page->atlas->rectangles[224];page->atlas->rectangles[224].x=0xffff;src.left=-1;src.top=(short)saved.y;src.right=2;src.bottom=src.top+3;dst.left=dst.top=0;dst.right=dst.bottom=3;
 result=IDirectDrawSurface_Blt(page->surface,&dst,page->atlas->surface,&src,DDBLT_WAIT,NULL);CHECK(result!=DD_OK,"actual native invalid source rectangle positive error control");
 page->left=page->cursor_x=page->cursor_y=0;page->line_height=7;if(resident)resident->pixels_dirty=0;width=bh_atlas_page_wide_text(page,one);
 CHECK(width==3&&page->cursor_x==3&&page->line_height==3&&(!resident||resident->pixels_dirty==1),"native blit failure still advances and dirties exactly as whole owner");page->atlas->rectangles[224]=saved;
}
static void archive_font(const char *archive,BHSceneView *view){BHTextureContext texture;BHAtlasContext atlas;BHAtlasPage *page;unsigned short text[]={65,0};long width;
 if(!archive)return;CHECK(bh_vfile_mount(archive),"mount pinned archive asimmutable font data");if(!context(view,&texture,&atlas,320)){CHECK(0,"real archive contexts");return;}
 page=bh_atlas_page_create(&atlas,"TEXTURES\\TVFONT.PCX",128);CHECK(page!=NULL,"whole source page constructor loads real font");if(page){width=(short)page->atlas->rectangles[33].width;CHECK(bh_atlas_page_wide_width(page,text)==width&&bh_atlas_page_wide_text(page,text)==width,"real archive font source owners compose with wholewide text");CHECK(bh_atlas_page_dispose(&atlas,page),"real font owning teardown");}bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&texture);
}
int bh_atlas_wide_checkpoint(const char *archive,unsigned long *count){BHSceneView view;BHTextureContext texture;BHAtlasContext atlas;BHAtlasPage *page;BHMathEnvironment env;
 checks=failures=0;CHECK(bh_math_environment_enter(&env),"actual PC53 native context");metrics();queue_cases();CHECK(fixture(),"synthetic300glyphPCX data");memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"actual offscreen DirectDraw/Direct3D2 context");
 if(view.device){if(context(&view,&texture,&atlas,100)){page=bh_atlas_page_create(&atlas,"WIDE_ATLAS.PCX",32);CHECK(page!=NULL,"whole source atlas/page resource creation");if(page){native_controls(&atlas,&texture,page);CHECK(bh_atlas_page_dispose(&atlas,page),"actual page/resource owning disposal");}bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&texture);}else CHECK(0,"actual native contexts");archive_font(archive,&view);}
 bh_scene_view_dispose(&view);bh_vfile_shutdown();remove("WIDE_ATLAS.PCX");bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"atlas-wide\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
