#include "../src/bh_flash_flare.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_render_execute.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<15)printf("FAIL flash-flare %lu: %s\n",checks,s);}}while(0)
typedef struct Capture{BHTextureImage images[4];unsigned long count,ids[20],colors[20];float centers[20][2],sizes[20];BHDrawCommand *mutate;}Capture;
static BHTextureImage *__cdecl lookup(unsigned long id,void*u){Capture*c=(Capture*)u;return id>=1&&id<=4?c->images+id-1:NULL;}
static int __cdecl capture(BHClipVertex*ring,BHTextureImage*image,unsigned long color,void*u){Capture*c=(Capture*)u;BHClipVertex*p=ring;unsigned long n=0,i=c->count++;float x=0,y=0,lo=100000,hi=-100000;
 if(i>=20)return 0;do{x+=p->sx;y+=p->sy;if(p->sx<lo)lo=p->sx;if(p->sx>hi)hi=p->sx;++n;p=p->next;}while(p!=ring&&n<13);
 c->ids[i]=(unsigned long)(image-c->images)+1;c->colors[i]=color;c->centers[i][0]=x/n;c->centers[i][1]=y/n;c->sizes[i]=hi-lo;if(c->mutate)c->mutate->next=73;return 1;
}
static void setup(BHFlareSubmit*c,BHScreenQuadSubmit*s,BHScreenQuadContext*d,BHScreenClipContext*clip,BHViewConfig*v,BHMainCamera*camera,Capture*cap){
 memset(c,0,sizeof(*c));memset(s,0,sizeof(*s));memset(d,0,sizeof(*d));memset(v,0,sizeof(*v));memset(camera,0,sizeof(*camera));memset(cap,0,sizeof(*cap));
 camera->projected.m[0][0]=camera->projected.m[1][1]=camera->projected.m[2][2]=1;camera->focal=320;v->near_z=1;v->left=v->top=-10000;v->right=v->bottom=10000;
 CHECK(bh_screen_clip_init(clip,1,1000,-10000,10000,-10000,10000),"actual screen clip source owner");d->view=v;d->clip=clip;d->emit=capture;d->emit_user=cap;d->base_color=0x80000000UL;
 s->draw=d;s->lookup=lookup;s->lookup_user=cap;c->camera=camera;c->view=v;c->width=640;c->height=480;c->screen=s;
}
static float number(const unsigned long*w){float x;memcpy(&x,w,4);return x;}
static void pure(void){BHFlareSubmit c;BHScreenQuadSubmit s;BHScreenQuadContext d;BHScreenClipContext clip;BHViewConfig v;BHMainCamera camera;Capture cap;BHDrawQueue q;
 BHVec3 p={320,240,1};BHDrawCommand*command;unsigned long next,i,ids[14]={1,2,3,4,4,3,3,4,4,3,3,4,3,4};float before,nan;unsigned long bits=0x7fc01234UL;
 setup(&c,&s,&d,&clip,&v,&camera,&cap);CHECK(bh_draw_queue_init(&q,12,NULL),"actual source command queue");
 memset(q.commands+9,0xa5,sizeof(BHDrawCommand));CHECK(bh_flare_queue(&q,&p,.5f,32000,1,2,3,4,0)==1,"whole producer reserves real command");command=q.commands+9;
 CHECK(q.commands[7].next==9&&q.commands[7].resource==9&&command->next==8,"fixed original0x250 tail maps to bucket7/header7, preserving global next");
 CHECK(command->kind==BH_DRAW_FLARE&&!memcmp(command->words,&p,12)&&number(command->words+3)==.5f&&number(command->words+4)==32000&&number(command->words+5)==0,"complete raw scalar producer payload");
 CHECK(command->words[6]==1&&command->words[7]==2&&command->words[8]==3&&command->words[9]==4&&command->words[10]==0xa5a5a5a5UL,"four texture IDs and unrelated command bytes retained");
 CHECK(bh_flare_execute(&c,command,&next)&&next==8&&cap.count==14&&c.attempted==14&&c.emitted==14,"whole14-row consumer invokes real projection and actual screenquad owner for every row");
 CHECK(number(command->words+4)==100,"consumer mutates normalized command strength");
 for(i=0;i<14;++i)CHECK(cap.ids[i]==ids[i]&&cap.centers[i][0]==320&&cap.centers[i][1]==240,"all immutable table texture selectors and center-axis placement");
 CHECK(fabs(cap.sizes[0]-160)<.0001&&fabs(cap.sizes[1]-300)<.0001&&fabs(cap.sizes[2]-150)<.0001&&fabs(cap.sizes[13]-40)<.0001,"negative first size uses projected world size; positive table sizes scale640 viewport");
 cap.count=0;CHECK(bh_flare_execute(&c,command,&next)&&number(command->words+4)==.3125f&&cap.count==14,"second execution divides already-mutated strength again");
 bh_draw_queue_begin(&q);cap.count=0;CHECK(bh_flare_queue(&q,&p,.5f,32000,1,2,0,0,0)==1&&bh_flare_execute(&c,q.commands+9,&next)&&cap.count==2,"absent third/fourth IDs clamp original intensity to2 rows");
 bh_draw_queue_begin(&q);cap.count=0;CHECK(bh_flare_queue(&q,&p,.5f,32000,1,2,3,0,0)==1&&q.commands[9].words[9]==3&&bh_flare_execute(&c,q.commands+9,&next)&&cap.count==14,"fourth zero falls back to third before consumer texture-hole decision");
 bh_draw_queue_begin(&q);cap.count=0;p.x=400;CHECK(bh_flare_queue(&q,&p,.5f,32000,1,2,3,4,0)==1&&bh_flare_execute(&c,q.commands+9,&next),"off-center whole consumer");
 CHECK(cap.centers[0][0]==400&&cap.centers[1][0]==240&&fabs(cap.centers[2][0]-456)<.0001,"positive/negative/1.7 table positions follow view-center line");
 bh_draw_queue_begin(&q);cap.count=0;p.x=320;bh_flare_queue(&q,&p,.5f,1.6f,1,2,3,4,0);
 CHECK(bh_flare_execute(&c,q.commands+9,&next)&&cap.count==6&&cap.colors[5]==0x800b0b0bUL,"fractional5.25 table budget draws sixth row with fractionally weighted alpha through actual color conversion");
 bh_draw_queue_begin(&q);cap.count=0;c.width=641;c.height=479;p.x=320;p.y=239;bh_flare_queue(&q,&p,.5f,32000,1,2,3,4,0);
 CHECK(bh_flare_execute(&c,q.commands+9,&next)&&cap.count==14&&cap.centers[0][0]==320&&cap.centers[0][1]==239&&fabs(cap.sizes[1]-300.46875)<.0001,"odd viewport uses truncated integer halves and full integer width scaling");
 c.width=640;c.height=480;p.y=240;
 bh_draw_queue_begin(&q);cap.count=0;p.x=320;p.z=.5f;bh_flare_queue(&q,&p,.5f,37,1,2,3,4,0);CHECK(bh_flare_execute(&c,q.commands+9,&next)&&!cap.count&&number(q.commands[9].words+4)==37,"projection rejection leaves strength untouched");
 bh_draw_queue_begin(&q);p.z=1;bh_flare_queue(&q,&p,.5f,.1f,1,2,3,4,0);before=number(q.commands[9].words+4);CHECK(bh_flare_execute(&c,q.commands+9,&next)&&!cap.count&&number(q.commands[9].words+4)<before,"depth-strength rejection retains normalization mutation");
 bh_draw_queue_begin(&q);p.x=900;bh_flare_queue(&q,&p,.5f,32000,1,2,3,4,0);CHECK(bh_flare_execute(&c,q.commands+9,&next)&&!cap.count&&number(q.commands[9].words+4)==100,"radial-height rejection also retains normalization");
 bh_draw_queue_begin(&q);p.x=320;memcpy(&nan,&bits,4);bh_flare_queue(&q,&p,.5f,nan,1,2,3,4,0);CHECK(bh_flare_execute(&c,q.commands+9,&next)&&!cap.count&&_isnan(number(q.commands[9].words+4)),"unordered strength performs no table iteration");
 bh_draw_queue_begin(&q);cap.count=0;bh_flare_queue(&q,&p,.5f,32000,99,2,3,4,0);CHECK(!bh_flare_execute(&c,q.commands+9,&next)&&c.failures==1&&cap.count==13&&next==8,"missing image binding reports failure but continues complete table and publishes next");
 bh_draw_queue_begin(&q);cap.count=0;bh_flare_queue(&q,&p,.5f,32000,1,2,0,0,0);cap.mutate=q.commands+9;CHECK(bh_flare_execute(&c,q.commands+9,&next)&&next==73,"current link read happens after actual draw callbacks");cap.mutate=NULL;
 bh_draw_queue_begin(&q);for(i=0;i<3;++i)CHECK(bh_flare_queue(&q,&p,.5f,1,1,2,3,4,0)==1,"producer fills real capacity");CHECK(!bh_flare_queue(&q,&p,.5f,1,1,2,3,4,0)&&q.error==BH_DRAW_FULL,"full real queue drops entire command");
 CHECK(bh_draw_queue_dispose(&q),"real source queue disposal");
}
static BHTextureImage *__cdecl native_lookup(unsigned long id,void*u){return id==1?(BHTextureImage*)u:NULL;}
static unsigned long pixel(const char*path,unsigned long x,unsigned long y){FILE*f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER n;unsigned char b[3];unsigned long stride;
 if(!f)return 0xffffffffUL;if(fread(&h,sizeof(h),1,f)!=1||fread(&n,sizeof(n),1,f)!=1||n.biBitCount!=24){fclose(f);return 0xffffffffUL;}stride=(n.biWidth*3+3)&~3UL;
 if(fseek(f,h.bfOffBits+(n.biHeight-1-y)*stride+x*3,SEEK_SET)||fread(b,1,3,f)!=3){fclose(f);return 0xffffffffUL;}fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];}
static void native_test(void){BHSceneView view;BHTextureContext textures;BHTextureImage*image=NULL;BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;BHScreenClipContext clip;BHFrameDisplay display;BHTextQueue text;BHRenderExecute execute;
 BHScreenQuadContext draw;BHScreenQuadSubmit screen;BHFlareSubmit flare;BHMainCamera camera;BHPcx pcx;BHDrawQueue queue;BHVec3 p={0,0,0};unsigned char pixels[1024];unsigned long next,first,second;int pass;
 memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&native,0,sizeof(native));memset(&queue,0,sizeof(queue));
 CHECK(bh_scene_view_init(&view,640,480),"actual private native offscreen target");if(!view.device)goto done;
 CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL)&&bh_texture_formats(&textures),"real texture format/cache owners");textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
 memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
 image=bh_texture_stage(&textures,"FLARE-WHITE.PCX",&pcx,0,0);CHECK(image&&image->source&&image->palette,"actual uploaded white texture");if(!image)goto done;
 CHECK(!bh_render_material_initialize(&material,&textures,view.d3d)&&!material.failures&&bh_render_state_init(&state,&textures),"actual native material and blend state");view.original_depth=1;
 memset(&camera,0,sizeof(camera));CHECK(bh_view_config_init(&view.projection,640,480)&&bh_main_camera_settings(&view.projection,90,.125,100)&&bh_main_camera_pose(&view.projection,&camera,0,0,-10,0,0,0),"actual projected camera owners");
 CHECK(bh_screen_clip_init(&clip,.125f,100,0,640,0,480)&&bh_draw_queue_init(&queue,10,NULL),"actual source queue/screen clip");memset(&draw,0,sizeof(draw));draw.view=&view.projection;draw.clip=&clip;draw.emit=bh_native_primitive_emit;draw.emit_user=&native;draw.base_color=0x80000000UL;native.textures=&textures;native.near_z=.125f;
 screen.draw=&draw;screen.lookup=native_lookup;screen.lookup_user=image;memset(&flare,0,sizeof(flare));flare.camera=&camera;flare.view=&view.projection;flare.width=640;flare.height=480;flare.screen=&screen;
 memset(&display,0,sizeof(display));display.textures=&textures;display.auxiliary=view.depth;display.color=view.color;bh_text_queue_init(&text);memset(&execute,0,sizeof(execute));execute.display=&display;execute.text=&text;execute.draw=&queue;execute.state=&state;execute.view=&view.projection;execute.camera=&camera;execute.flare=&flare;
 first=second=0xffffffffUL;
 for(pass=0;pass<2;++pass){const char*name=pass?"flare-reject-offscreen.bmp":"flare-draw-offscreen.bmp";bh_draw_queue_begin(&queue);p.z=pass?-20:0;
  CHECK(bh_flare_queue(&queue,&p,8,1000,1,1,0,0,0)==1&&bh_scene_view_begin(&view),"real flare producer on actual native target");
  CHECK(bh_render_execute(&execute)&&execute.kinds[BH_DRAW_FLARE]==1&&execute.draw_calls==10&&state.mode==3,"actual complete frame executor consumes flare in original additive bucket7 and flushes real native tail");
  CHECK(bh_scene_view_end(&view,name),"whole consumer through actual screenquad/native emitter, full frame barrier and readback");
  if(pass)second=pixel(name,320,240);else first=pixel(name,320,240);remove(name);
 }
 CHECK(first!=0xffffffffUL&&(first&0xffffffUL)!=0&&second==0,"independent center pixel positive and behind-camera blank control");CHECK(native.polygons>=1,"real native polygon conversion was invoked");
done:bh_draw_queue_dispose(&queue);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
}
static void entity(void){BHEntityPool pool;BHDrawQueue q;BHFlashFlareContext c;BHEntity*e;BHDrawCommand*p;
 CHECK(bh_entities_init(&pool,2,NULL)&&bh_draw_queue_init(&q,10,NULL),"actual flash entity/queue ownership");memset(&c,0,sizeof(c));c.pool=&pool;c.queue=&q;c.textures[0]=1;c.textures[1]=2;c.textures[2]=3;c.textures[3]=4;e=bh_entity_acquire(&pool);e->position.x=1;e->position.y=2;e->position.z=3;
 CHECK(bh_flash_flare_bind(&c,e,.05f,25,.5f)&&e->step==bh_script_flash_tick&&e->refresh==bh_flash_flare_refresh,"explicit field binding installs actual lifetime and complete refresh owners");
 e->refresh(e,NULL);p=q.commands+9;CHECK(!memcmp(p->words,&e->position,12)&&number(p->words+3)==.5f&&number(p->words+4)==12.5f&&number(p->words+5)==0&&p->words[6]==1&&p->words[9]==4,"whole refresh computes exact(1-life*10)*strength and forwards actual field/resource binding");
 CHECK(bh_entities_dispose(&pool)&&bh_draw_queue_dispose(&q),"actual flash bindings disposed");}
int __cdecl bh_flash_flare_checkpoint(unsigned long*count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;pure();entity();native_test();bh_math_environment_leave(&fp);if(count)*count=checks;printf("{\"scenario\":\"flash-flare\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_FLASH_FLARE_STANDALONE
int main(void){return bh_flash_flare_checkpoint(NULL);}
#endif
