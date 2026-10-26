#include "../src/bh_input_text.h"
#include "../src/bh_menu_row.h"
#include "../src/bh_message_panel.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <direct.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL input-text %d: %s\n",__LINE__,s);}}while(0)
typedef struct Memory{unsigned long live,calls;int deny;unsigned long fail_at;}Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory*m=(Memory*)user;void*p;++m->calls;if(m->deny||m->calls==m->fail_at)return NULL;p=malloc(n);if(p){++m->live;memset(p,0xa5,n);}return p;}
static void __cdecl release(void *p,void *user){Memory*m=(Memory*)user;CHECK(m->live>0,"allocator actual balance");--m->live;free(p);}
static unsigned long word(const BHEntity*e,unsigned long offset){unsigned long v;memcpy(&v,(const unsigned char*)e+offset,4);return v;}
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
typedef struct CaptureFixture {BHEntityPool pool;BHPlayerInputOwner inputs;BHPlayerInputList*list;BHInputSnapshot snapshot;BHPlayerInputState state;BHSoundQueue sound;BHMenuSettings settings;BHMenuNetworkRecord record;BHMenuSettingsMenu menu;BHMenuSettingsRow row,*rowptr[2];BHInputTextCapture capture;BHConfig config;BHInputProfile *current;long pending;BHMenuProfileContext profile;BHTextureContext texture;BHAtlasContext atlas;BHTextQueue text;BHMenuRows rows;BHMenuRow*slots[2];char buffer[100];Memory memory;unsigned long refreshes;}CaptureFixture;
static void __cdecl row_bridge(BHMenuSettingsMenu*m,long index){CaptureFixture*f=(CaptureFixture*)m->user;++f->refreshes;
 CHECK(!f->capture.input->flags&&f->capture.commit==BH_TEXT_COMMIT_NONE&&f->capture.entity,"real commit/cancel refresh sees mode0 and cleared commit before entity release");
 CHECK(bh_menu_row_refresh(&f->rows,index,NULL,(char*)"COMMITTED"),"real optional refresh bridge delegates actual row owner");}
static int setup(CaptureFixture*f,BHSceneView*view,BHInputTextCommit commit,const char*initial,long limit){BHSceneAllocator a;BHSoundResource*r;unsigned long i,done;
 memset(f,0,sizeof(*f));a.allocate=allocate;a.release=release;a.user=&f->memory;
 if(!bh_entities_init(&f->pool,2,&a)||!bh_player_input_owner_init(&f->inputs,&a)||!bh_sound_queue_init(&f->sound,NULL,NULL,&a))return 0;
 f->list=bh_player_input_create(&f->inputs,0);if(!f->list)return 0;for(i=0;i<255;++i)bh_player_input_bind_key(f->list,i,"",0,0,(unsigned short)i);
 r=bh_sound_resource_allocate(&f->sound);if(!r)return 0;r->streaming=1;r->base_frequency=100;
 if(!bh_config_init(&f->config,NULL)||!bh_config_load(&f->config,"missing-input-text-private.cfg"))return 0;f->current=f->config.profiles;f->profile.config=&f->config;f->profile.current=&f->current;f->profile.pending_selection=&f->pending;f->pending=7;
 if(!context(view,&f->texture,&f->atlas,&f->memory,100))return 0;bh_text_queue_init(&f->text);if(!bh_menu_rows_init(&f->rows,&f->atlas,&f->text,f->slots,2))return 0;
 for(i=0;i<2;++i)if(!bh_menu_row_create(&f->rows,i,(const unsigned char*)"TEXT",NULL,0,0,NULL))return 0;
 if(bh_message_text_drain(&f->text,&f->atlas,&done)!=1)return 0;
 f->row.refresh=row_bridge;f->rowptr[0]=f->rowptr[1]=&f->row;f->menu.rows=f->rowptr;f->menu.row_count=2;f->menu.selection=1;f->menu.user=f;f->settings.selected_network=&f->record;
 strcpy(f->buffer,initial);f->capture.pool=&f->pool;f->capture.input=f->list;f->capture.snapshot=&f->snapshot;f->capture.state=&f->state;f->capture.settings=&f->settings;f->capture.menu=&f->menu;f->capture.profile=&f->profile;f->capture.rows=&f->rows;f->capture.sound=&f->sound;f->capture.sound_resource=r->index;f->capture.buffer=f->buffer;f->capture.capacity=sizeof(f->buffer);f->capture.length=strlen(initial);f->capture.limit=limit;f->capture.commit=commit;return 1;
}
static int evaluate(CaptureFixture*f){return bh_input_evaluate(f->list,&f->snapshot,0);}
static void next_frame(CaptureFixture*f){memcpy(f->snapshot.previous,f->snapshot.current,255);CHECK(evaluate(f),"actual cached input packet evaluation");}
static void press(CaptureFixture*f,unsigned long key){f->snapshot.current[key]=1;CHECK(evaluate(f)&&bh_entities_step(&f->pool),"cached key event through actual installed text entity step");}
static void start(CaptureFixture*f){CHECK(evaluate(f)&&bh_input_text_begin(&f->capture)&&bh_entities_step(&f->pool)&&word(f->capture.entity,0x1bc)==1,"whole text phase0 primes actual transitions");}
static void pixels(BHAtlasPage*page){DDSURFACEDESC d;unsigned long x,y,n=0;unsigned short value;memset(&d,0,sizeof(d));d.dwSize=sizeof(d);
 CHECK(IDirectDrawSurface_Lock(page->surface,NULL,&d,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"actual textbox native page readback");if(d.lpSurface){for(y=0;y<128;++y)for(x=0;x<128;++x){memcpy(&value,(char*)d.lpSurface+y*d.lPitch+x*2,2);if(value)++n;}CHECK(n>0,"actual text capture row paints nonzero native pixels");CHECK(IDirectDrawSurface_Unlock(page->surface,NULL)==DD_OK,"actual textbox native unlock");}}
static void finish(CaptureFixture*f){unsigned long done;if(f->capture.entity)CHECK(bh_input_text_cancel(&f->capture),"source explicit cancel releases actual capture entity");CHECK(bh_message_text_drain(&f->text,&f->atlas,&done)==1,"actual pending textbox commands drain before page destruction");CHECK(bh_menu_rows_dispose(&f->rows),"actual textbox pages disposed");bh_atlas_context_dispose(&f->atlas);bh_texture_context_dispose(&f->texture);bh_config_dispose(&f->config);CHECK(bh_sound_queue_dispose(&f->sound),"actual source sound queue disposal without playback");bh_player_input_owner_dispose(&f->inputs);CHECK(bh_entities_dispose(&f->pool)&&!f->memory.live,"all real input/text/entity resources released");}
static void editing(BHSceneView*view){CaptureFixture f;unsigned long sounds,done;BHSoundVoice*v;
 CHECK(setup(&f,view,BH_TEXT_COMMIT_ADDRESS,"",20),"all actual text editing owners");f.state.values[255]=0x12345678;f.snapshot.current[70]=1;start(&f);CHECK(!f.capture.sounds&&!f.capture.length&&f.state.values[70]==2&&f.state.values[255]==0x12345678,"held key primed without append; index255 excluded");
 next_frame(&f);press(&f,32);CHECK(!f.capture.length&&!strcmp(f.capture.display,"_")&&f.capture.sounds==1,"leading space rejected but base sound and textbox refresh retained");
 next_frame(&f);press(&f,65);CHECK(!strcmp(f.buffer,"A")&&!strcmp(f.capture.display,"A_")&&f.capture.sounds==3,"actual MapVirtualKeyA mode2 accepts ASCII and adds second success sound");
 next_frame(&f);press(&f,8);CHECK(!f.capture.length&&!f.buffer[0]&&f.capture.sounds==5,"nonempty backspace has base and .7 sounds");
 next_frame(&f);f.snapshot.current[8]=0;CHECK(evaluate(&f)&&bh_entities_step(&f.pool),"real backspace release packet");next_frame(&f);press(&f,8);CHECK(!f.capture.length&&f.capture.sounds==8,"empty backspace emits base .7 and .5 sounds");
 v=bh_sound_voice(&f.sound,f.capture.last_voice);CHECK(v&&v->frequency==50&&!v->positioned&&!v->moving,"last real error voice .5 pitch with no playback");
 next_frame(&f);strcpy(f.buffer,"abcdefghijklmnopqrst");f.capture.length=20;sounds=f.capture.sounds;press(&f,66);CHECK(!strcmp(f.buffer,"ABCDEFGHIJKLMNOPQRST")&&!strcmp(f.capture.display,"DEFGHIJKLMNOPQRST_")&&f.capture.sounds==sounds+3,"full buffer retains bytes, uppercases whole string, paints last17 plus underscore and two error sounds");
 CHECK(bh_message_text_drain(&f.text,&f.atlas,&done)==1&&done>0,"actual text editing draw queue drains");pixels(f.slots[1]->page);finish(&f);
}
static void commits(BHSceneView*view){CaptureFixture f;unsigned long method,count;for(method=1;method<=3;++method){CHECK(setup(&f,view,(BHInputTextCommit)method,"newname   ",method==1?15:30),"actual profile/address/phone commit owners");count=f.config.profile_count;start(&f);press(&f,13);CHECK(f.capture.entity&&word(f.capture.entity,0x1bc)==2&&!f.refreshes&&f.list->flags==2,"Return defers commit/release while repainting current capture");next_frame(&f);CHECK(bh_entities_step(&f.pool)&&!f.capture.entity&&!f.list->flags&&f.capture.commit==BH_TEXT_COMMIT_NONE&&!strcmp(f.buffer,"NEWNAME"),"final trim, full flush, input mode0, cleared commit and real entity release");
 if(method==1)CHECK(f.config.profile_count==count+1&&!strcmp((char*)f.current+12,"NEWNAME")&&!f.pending&&f.refreshes==2&&bh_config_valid(&f.config),"actual profile callback clones through real owning config and refreshes row0 then selected row");
 else CHECK(!strcmp((char*)f.record.bytes+(method==2?0x7c:0x1c),"NEWNAME")&&f.refreshes==1,"actual network text callback writes correct record then real row refresh");finish(&f);}
 CHECK(setup(&f,view,BH_TEXT_COMMIT_ADDRESS,"cancel",20),"Escape cancellation owners");start(&f);f.snapshot.current[13]=1;f.snapshot.current[27]=1;CHECK(evaluate(&f)&&bh_entities_step(&f.pool)&&word(f.capture.entity,0x1bc)==3&&f.capture.repaints==2,"phase1 continues scanning after Return; later Escape overrides phase and both repaint");next_frame(&f);CHECK(bh_entities_step(&f.pool)&&!f.capture.entity&&!f.record.bytes[0x7c]&&f.refreshes==1,"Escape clears commit and refreshes selected actual row without committing buffer");finish(&f);
 CHECK(setup(&f,view,BH_TEXT_COMMIT_PROFILE,"   ",15),"all-space profile completion owners");count=f.config.profile_count;start(&f);press(&f,13);next_frame(&f);CHECK(bh_entities_step(&f.pool)&&!f.capture.entity&&!f.capture.length&&!f.buffer[0]&&f.config.profile_count==count&&f.pending==7&&f.refreshes==1,"all trailing spaces trim to empty; actual empty profile commit refreshes only selected row");finish(&f);
}
static void mapping(BHSceneView*view){CaptureFixture f;unsigned long key,mapped,done;CHECK(setup(&f,view,BH_TEXT_COMMIT_PHONE,"x",30),"native mapping-index behavior fixture");start(&f);
 for(key=33;key<255;++key){mapped=MapVirtualKeyA(key,2);if(mapped>key&&mapped<254)break;}
 if(key<255){f.snapshot.current[key]=1;f.snapshot.current[key+1]=1;CHECK(evaluate(&f)&&bh_entities_step(&f.pool)&&f.capture.repaints==1&&f.state.values[key+1]==1,"actual translated character replaces scan index, skipping intervening newly-down key");}else CHECK(1,"current native keyboard has no increasing mapping-index example; basic mode2 translation exercised");
 f.capture.sound_resource=0xffff;next_frame(&f);f.snapshot.current[8]=1;CHECK(evaluate(&f)&&!bh_input_text_step(&f.capture)&&f.capture.repaints>0,"actual sound failure retains text and repaint side effects");CHECK(bh_message_text_drain(&f.text,&f.atlas,&done)==1,"failure continuation real textbox commands");finish(&f);
}
int __cdecl bh_input_text_checkpoint(unsigned long*count){BHMathEnvironment fp;BHSceneView view;checks=failures=0;CHECK(bh_math_environment_enter(&fp),"actual arithmetic context");CHECK(MapVirtualKeyA(65,2)==65,"native mode2 ASCII translation without input polling/capture");CHECK(fixture(),"private actual menu font");memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"actual offscreen native text context");if(view.device){editing(&view);commits(&view);mapping(&view);}bh_scene_view_dispose(&view);remove("textures\\tvfont.pcx");bh_vfile_shutdown();bh_math_environment_leave(&fp);if(count)*count=checks;printf("{\"scenario\":\"input-text\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"physical_input_calls\":0,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_INPUT_TEXT_STANDALONE
int main(void){return bh_input_text_checkpoint(NULL);}
#endif
