#include "../src/bh_menu_row.h"
#include "../src/bh_message_panel.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL menu-row %d: %s\n",__LINE__,s);}}while(0)
typedef struct Memory{unsigned long live,calls;int deny;unsigned long fail_at;}Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory*m=(Memory*)user;void*p;++m->calls;if(m->deny||m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory*m=(Memory*)user;CHECK(m->live>0,"actual allocator balance");--m->live;free(p);}
static void __cdecl on_refresh(void *user,long index){unsigned long *count=(unsigned long*)user;*count+=(unsigned long)index;}
static unsigned short unit(const unsigned char*p){unsigned short n;memcpy(&n,p,2);return n;}
static int fixture(void){unsigned char *data;FILE*f;unsigned long g,x,y,c,r;int ok;
 _mkdir("textures");data=(unsigned char*)calloc(1,128+7500+768);if(!data)return 0;data[0]=10;data[1]=2;data[2]=1;data[3]=8;data[8]=99;data[10]=74;data[65]=1;data[66]=100;memset(data+128,7,7500);
 for(g=0;g<300;++g){c=g%20;r=g/20;for(y=1+r*5;y<4+r*5;++y)for(x=1+c*5;x<1+c*5+(g%3)+1;++x)data[128+y*100+x]=(unsigned char)(1+g%6);}
 for(g=1;g<=7;++g){data[7628+g*3]=(unsigned char)(17*g);data[7628+g*3+1]=(unsigned char)(23*g);data[7628+g*3+2]=(unsigned char)(31*g);}
 f=fopen("textures\\tvfont.pcx","wb");if(!f){free(data);return 0;}ok=fwrite(data,1,8396,f)==8396;if(fclose(f))ok=0;free(data);return ok;
}
static int context(BHSceneView*v,BHTextureContext*t,BHAtlasContext*a,Memory*m,unsigned long step){BHSceneAllocator allocator={allocate,release,m};DDPIXELFORMAT pixel;unsigned long i;
 if(!bh_texture_context_init(t,v->draw,v->device,&allocator)||!bh_texture_formats(t))return 0;t->palette_mode=2;t->slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
 memset(&pixel,0,sizeof(pixel));pixel.dwSize=32;pixel.dwFlags=DDPF_RGB;pixel.dwRGBBitCount=16;pixel.dwRBitMask=0xf800;pixel.dwGBitMask=0x7e0;pixel.dwBBitMask=31;for(i=0;i<9;++i)t->formats[i].pixel=pixel;
 if(!bh_texture_bind_display(t,&pixel))return 0;memset(a,0,sizeof(*a));return bh_atlas_context_init(a,t,step);
}
static void wide(const BHTextCommand*c,const unsigned short *expected,unsigned long n,long x,long y){unsigned long i;CHECK(c->kind==BH_TEXT_COMMAND_WIDE&&c->position.xy[0]==x&&c->position.xy[1]==y,"exact centered wide row command");for(i=0;i<n;++i)CHECK(unit(c->text+i*2)==expected[i],"exact original title glyph conversion");CHECK(unit(c->text+n*2)==0,"converted row terminator");}
static void native_pixels(BHAtlasPage*page){DDSURFACEDESC d;HRESULT hr;unsigned long x,y,lit=0;unsigned short value;
 memset(&d,0,sizeof(d));d.dwSize=sizeof(d);hr=IDirectDrawSurface_Lock(page->surface,NULL,&d,DDLOCK_WAIT|DDLOCK_READONLY,NULL);CHECK(hr==DD_OK&&d.lpSurface,"actual constructor page native readback");if(hr==DD_OK&&d.lpSurface){for(y=0;y<128;++y)for(x=0;x<128;++x){memcpy(&value,(unsigned char*)d.lpSurface+y*d.lPitch+x*2,2);if(value)++lit;}CHECK(lit>0,"queued constructor labels reach actual source-loaded native page pixels");CHECK(IDirectDrawSurface_Unlock(page->surface,NULL)==DD_OK,"actual page unlock");}}
static void synthetic(BHSceneView *view){BHTextureContext texture;BHAtlasContext atlas;Memory memory={0,0,0};BHTextQueue queue;BHMenuRows rows;BHMenuRow *slots[2],*row,*custom;unsigned long processed,held,counter=0;const unsigned char title[]="A0 a\xc4\xd6\xdc\xe4\xf6\xfc!\nZ";unsigned char body[]="one\n\ntwo\n",later[]="Go\n",skip[]="Skip\nBody";unsigned short expected[10]={257,283,256,293,319,320,321,322,323,324},last[]={282};unsigned char long_title[257];
 CHECK(context(view,&texture,&atlas,&memory,100),"actual native texture/atlas resource context");bh_text_queue_init(&queue);CHECK(bh_menu_rows_init(&rows,&atlas,&queue,slots,2),"bounded explicit menu row owner");
 row=bh_menu_row_create(&rows,0x12345678,title,body,0xabcdef01,0x98765432,NULL);CHECK(row&&rows.count==1&&row==slots[0],"whole row constructor publishes real record and native page");if(!row)goto done;
 CHECK(sizeof(*row)==28&&row->identity==0x12345678&&row->first==0xabcdef01&&row->second==0x98765432&&row->title==title&&!row->refresh&&row->image==row->page->image&&row->page->size==128,"all seven original metadata words and exact128page/image identity");
 CHECK(queue.write==6&&queue.commands[0].kind==BH_TEXT_COMMAND_CLEAR&&queue.commands[0].position.color==0,"clear plus original two title/three body lines queued");
 wide(queue.commands+1,expected,10,54,9);wide(queue.commands+2,last,1,63,27);
 CHECK(queue.commands[3].kind==BH_TEXT_COMMAND_TEXT&&!strcmp((char*)queue.commands[3].text,"one")&&queue.commands[3].position.xy[0]==62&&queue.commands[3].position.xy[1]==48,"first centered ASCII bodyline aftertitle");
 CHECK(queue.commands[4].kind==BH_TEXT_COMMAND_TEXT&&!queue.commands[4].text[0]&&queue.commands[4].position.xy[0]==64&&queue.commands[4].position.xy[1]==60,"consecutive newline preservesempty bodyline");
 CHECK(queue.commands[5].kind==BH_TEXT_COMMAND_TEXT&&!strcmp((char*)queue.commands[5].text,"two")&&queue.commands[5].position.xy[1]==72,"trailing newline does not queueextra bodyline");
 CHECK(body[3]==0&&body[4]==0&&body[8]==0,"original writable body separators mutated inplace");
 CHECK(!bh_menu_rows_dispose(&rows),"source queued-page lifetime guard");processed=0;CHECK(bh_message_text_drain(&queue,&atlas,&processed)==1&&processed==6,"actual shared FIFO executes constructed ANSI/wide labels");native_pixels(row->page);
 bh_text_queue_init(&queue);CHECK(bh_menu_row_refresh(&rows,0,NULL,later)&&row->title==title&&later[2]==0&&queue.write==4,"refresh retains stored title onNULL and splitswritable newbody");CHECK(bh_message_text_drain(&queue,&atlas,&processed)==1&&processed==4,"real refreshed row queued path");
 bh_text_queue_init(&queue);CHECK(bh_menu_row_refresh(&rows,0,(const unsigned char*)"42",NULL)&&queue.write==2&&!strcmp((const char*)row->title,"42"),"refresh replaces actual borrowed title");{unsigned short numbers[]={287,285};wide(queue.commands+1,numbers,2,63,21);}CHECK(bh_message_text_drain(&queue,&atlas,&processed)==1,"updatedtitle drained");
 memory.deny=1;held=memory.live;CHECK(!bh_menu_row_create(&rows,9,NULL,NULL,0,0,NULL)&&rows.count==1&&memory.live==held&&rows.error==2,"actual row allocation failure retains owner/native resources");memory.deny=0;
 memory.fail_at=memory.calls+2;held=memory.live;CHECK(!bh_menu_row_create(&rows,10,NULL,NULL,0,0,NULL)&&rows.count==1&&memory.live==held&&rows.error==3,"actual downstream page allocation failure frees new row and retains prior whole owner");memory.fail_at=0;
 custom=bh_menu_row_create(&rows,17,(const unsigned char*)"Deferred",skip,3,4,on_refresh);CHECK(custom&&rows.count==2&&queue.read==queue.write&&!strcmp((char*)skip,"Skip\nBody")&&custom->refresh==on_refresh,"optional real borrowed refresh bypasses initialpainting/bodymutation");if(custom){custom->refresh(&counter,7);CHECK(counter==7&&custom->page->atlas==row->page->atlas,"borrowed real callback identity and shared actual font cache");}
 CHECK(!bh_menu_row_create(&rows,18,NULL,NULL,0,0,NULL)&&rows.count==2,"source bounded row capacity");
 memset(long_title,'A',256);long_title[256]=0;CHECK(!bh_menu_row_refresh(&rows,0,long_title,NULL)&&!strcmp((const char*)row->title,"42")&&queue.read==queue.write,"source prevalidation rejects original256unit stackoverflow withoutrow/queue mutation");
 bh_text_queue_init(&queue);while(bh_text_queue_count(&queue)<255)bh_text_queue_text(&queue,NULL,"",0,0);later[0]='G';later[1]='o';later[2]=10;later[3]=0;
 CHECK(bh_menu_row_refresh(&rows,0,NULL,later)&&rows.dropped==3&&rows.count==2&&later[2]==0,"full queue dropsclear/title/body individually but continuesbodymutation");CHECK(bh_message_text_drain(&queue,&atlas,&processed)==1&&processed==255,"actual fullring drains before rowlifetime teardown");
 CHECK(bh_menu_rows_dispose(&rows)&&!slots[0]&&!slots[1],"complete owned row/page disposal after FIFOdrain");
done:bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&texture);CHECK(!memory.live,"all actual cache/page/record allocations released");}
static void real_font(const char *archive,BHSceneView *view){BHTextureContext texture;BHAtlasContext atlas;Memory memory={0,0,0};BHTextQueue queue;BHMenuRows rows;BHMenuRow *slots[1],*row;unsigned long processed;
 if(!archive)return;CHECK(bh_vfile_mount(archive),"real immutable archive font binding");if(!context(view,&texture,&atlas,&memory,320)){CHECK(0,"actual archived font resource context");return;}bh_text_queue_init(&queue);CHECK(bh_menu_rows_init(&rows,&atlas,&queue,slots,1),"real font menu owner");row=bh_menu_row_create(&rows,1,(const unsigned char*)"GAME",NULL,0,0,NULL);CHECK(row&&row->page->atlas->rectangle_count>250,"whole constructor createsactual archivedTVFONTpage with highglyph title");if(row){CHECK(bh_message_text_drain(&queue,&atlas,&processed)==1&&processed==2,"real archived menu title reaches nativeFIFO");native_pixels(row->page);CHECK(bh_menu_rows_dispose(&rows),"real archived menu owner disposal");}bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&texture);CHECK(!memory.live,"real archive menu resourcesreleased");}
int bh_menu_row_checkpoint(const char *archive,unsigned long *count){BHSceneView view;BHMathEnvironment env;checks=failures=0;CHECK(bh_math_environment_enter(&env),"actual numerical profile");CHECK(fixture(),"actual private300glyphfont fixture");memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"actual offscreen source native context");if(view.device)synthetic(&view);remove("textures\\tvfont.pcx");if(view.device)real_font(archive,&view);bh_scene_view_dispose(&view);bh_vfile_shutdown();bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"menu-row\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;}
