#include "../src/bh_target_draw.h"
#include "../src/bh_fp.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<15)printf("FAIL target-draw %lu: %s\n",checks,s);}}while(0)
static float value(const unsigned long *word){float f;memcpy(&f,word,4);return f;}
static int closef(float a,float b){return fabs((double)a-b)<.0001;}
typedef struct Fixture {BHDrawQueue queue;BHTargetDrawContext context;BHMainCamera camera;BHViewConfig view;BHEntity entity,player;BHTargetDrawState state;} Fixture;
static void fixture(Fixture *f,unsigned long capacity){float basis[9]={1,0,0,0,1,0,0,0,1};BHVec3 angles={.1f,.2f,.3f};memset(f,0,sizeof(*f));
 CHECK(bh_draw_queue_init(&f->queue,capacity,NULL),"actual nine-bucket draw queue");f->camera.projected.m[0][0]=100;f->camera.projected.m[1][1]=100;f->camera.projected.m[2][2]=1;f->camera.projected.m[0][2]=320;f->camera.projected.m[1][2]=240;f->camera.focal=100;
 f->view.near_z=1;f->view.far_z=100;f->view.width=f->view.right=640;f->view.height=f->view.bottom=480;
 f->context.queue=&f->queue;f->context.camera=&f->camera;f->context.view=&f->view;f->context.player=&f->player;f->context.atlas_image=17;f->context.marker_image=19;f->context.screen_width=640;
 memcpy(f->entity.opaque0,basis,36);memcpy((unsigned char*)&f->entity+0x3c,&angles,12);f->entity.position.z=10;
 f->state.number=123;f->state.resource=37;f->state.owner=&f->player;f->state.local_anchor.x=1;f->state.local_anchor.y=2;f->state.local_anchor.z=3;bh_target_draw_state_write(&f->entity,&f->state);
}
static void finish(Fixture *f){CHECK(bh_draw_queue_dispose(&f->queue),"actual queue ownership disposed");}
static void queue_contract(void){Fixture f;BHEntity before;BHDrawCommand *c;unsigned long i;float basis[9]={0,1,0,-1,0,0,0,0,1};
 fixture(&f,32);before=f.entity;CHECK(bh_target_draw(&f.context,&f.entity)==1&&f.queue.count==14,"whole model/background/three-digit path");
 CHECK(!memcmp(&before,&f.entity,sizeof(before)),"whole draw leaves every target byte unmodified");c=f.queue.commands+9;
 CHECK(c->kind==BH_DRAW_MODEL_RESOURCE&&c->resource==37&&value(c->words)==1&&value(c->words+1)==2&&value(c->words+2)==13,"real model queue receives translated identity-basis anchor");
 CHECK(value(c->words+3)==.1f&&value(c->words+4)==.2f&&value(c->words+5)==.3f&&value(c->words+6)==1,"exact model angles and unit modulation");
 c=f.queue.commands+10;CHECK(c->kind==BH_DRAW_RECT&&c->resource==17&&value(c->words)==320&&value(c->words+1)==216&&value(c->words+2)==64&&value(c->words+3)==14,"positive background geometry uses unshifted target projection");
 CHECK(value(c->words+6)==.00390625f&&value(c->words+7)==.4921875f&&value(c->words+8)==.50390625f&&value(c->words+9)==.1015625f&&value(c->words+10)==1.01f,"background UV pairs and exact near+.01 depth");
 for(i=0;i<3;++i){c=f.queue.commands+11+i;CHECK(value(c->words)==308+(float)i*8&&value(c->words+1)==264&&value(c->words+2)==8&&value(c->words+3)==12,"independently centered digit geometry and8-unit advance");CHECK(value(c->words+6)==.06640625f+(float)i*.0625f&&value(c->words+7)==.0546875f&&value(c->words+8)==.81640625f&&value(c->words+9)==.0859375f,"digit1/2/3 atlas UVs retained");}
 CHECK(f.queue.commands[1].next==9&&f.queue.commands[9].next==2&&f.queue.commands[8].next==10,"model bucket0 and overlay bucket8 retain real chain order");
 bh_draw_queue_begin(&f.queue);memcpy(f.entity.opaque0,basis,36);f.state.number=-123;bh_target_draw_state_write(&f.entity,&f.state);CHECK(bh_target_draw(&f.context,&f.entity)==1&&value(f.queue.commands[9].words)==-2&&value(f.queue.commands[9].words+1)==1,"column basis rotates anchor before model submission");
 CHECK(value(f.queue.commands[10].words+8)==.59765625f&&value(f.queue.commands[11].words+8)==.91015625f,"negative numbers select both exact alternate atlas rows");
 bh_draw_queue_begin(&f.queue);f.state.number=LONG_MIN;bh_target_draw_state_write(&f.entity,&f.state);CHECK(bh_target_draw(&f.context,&f.entity)==1&&f.queue.count==19&&value(f.queue.commands[11].words+6)<0,"signed minimum NEG wraps, preserves8-place negative digit UV without C overflow");
 bh_draw_queue_begin(&f.queue);f.state.number=2147483647L;bh_target_draw_state_write(&f.entity,&f.state);CHECK(bh_target_draw(&f.context,&f.entity)==1&&f.queue.count==19&&value(f.queue.commands[11].words+6)>1,"whole original8-place division retains multi-digit first quotient for large signed number");
 finish(&f);
}
static void paths_and_capacity(void){Fixture f;unsigned long i,j;const float xs[4]={-37,37,-37,37},ys[4]={-26.5f,-26.5f,26.5f,26.5f};BHDrawCommand *m;
 fixture(&f,32);f.state.owner=&f.entity;bh_target_draw_state_write(&f.entity,&f.state);CHECK(bh_target_draw(&f.context,&f.entity)==1&&f.queue.count==10,"foreign owner queues model only");
 bh_draw_queue_begin(&f.queue);f.state.owner=&f.player;f.state.number=0;bh_target_draw_state_write(&f.entity,&f.state);CHECK(bh_target_draw(&f.context,&f.entity)==1&&f.queue.count==10,"zero number queues model only");
 f.state.number=7;f.state.flags=4;bh_target_draw_state_write(&f.entity,&f.state);
 for(i=0;i<4;++i){bh_draw_queue_begin(&f.queue);f.entity.position.x=xs[i];f.entity.position.y=ys[i];CHECK(bh_target_draw(&f.context,&f.entity)==1&&f.queue.count==13,"whole projected-marker path at inclusive screen-margin corner");m=f.queue.commands+11;CHECK(m->resource==19&&value(m->words+2)==80&&value(m->words+3)==40,"all exact marker margin equalities retain local80x40 marker");}
 bh_draw_queue_begin(&f.queue);f.entity.position.x=38;f.entity.position.y=0;CHECK(bh_target_draw(&f.context,&f.entity)==1,"right margin fallback draw");m=f.queue.commands+11;CHECK(value(m->words)==320&&value(m->words+1)==27&&value(m->words+2)==100&&value(m->words+3)==50,"out-of-margin projected marker falls back to320,27 at100x50");
 bh_draw_queue_begin(&f.queue);f.entity.position.z=.5f;CHECK(bh_target_draw(&f.context,&f.entity)==1&&f.queue.count==11,"near rejection retains model and required fallback marker only");m=f.queue.commands+10;CHECK(m->resource==19&&value(m->words)==320&&value(m->words+1)==27&&value(m->words+9)==.218f,"rejected projection marker retains complete UV rectangle");
 bh_draw_queue_begin(&f.queue);f.state.flags=0;bh_target_draw_state_write(&f.entity,&f.state);CHECK(bh_target_draw(&f.context,&f.entity)==1&&f.queue.count==10,"near rejection without marker flag retains only model");
 bh_draw_queue_begin(&f.queue);f.entity.position.x=f.entity.position.y=0;f.entity.position.z=10;f.context.screen_width=320;CHECK(bh_target_draw(&f.context,&f.entity)==1,"half-width exact scale binding");CHECK(value(f.queue.commands[10].words+2)==32&&value(f.queue.commands[11].words+2)==4,"width/640 scales both background and digits");finish(&f);
 for(i=9;i<15;++i){fixture(&f,i);j=f.queue.count;CHECK(bh_target_draw(&f.context,&f.entity)==(i<14?0:1)&&f.queue.count==(i<14?i:14),"capacity saturation retains every successful full-owner prefix");CHECK(f.queue.count>=j,"exhaustion never rolls back real prior commands");finish(&f);}
}
typedef struct NativeFixture {BHDrawResource resource;BHTextureImage *image;} NativeFixture;
static const BHDrawResource *__cdecl lookup_model(unsigned long handle,void *u){NativeFixture *f=(NativeFixture*)u;return handle==37?&f->resource:NULL;}
static BHTextureImage *__cdecl lookup_image(unsigned long handle,void *u){NativeFixture *f=(NativeFixture*)u;return handle==17||handle==19?f->image:NULL;}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
static unsigned short pixel(BHSceneView *v,unsigned long x,unsigned long y){DDSURFACEDESC d;unsigned short color=0;memset(&d,0,sizeof(d));d.dwSize=sizeof(d);if(IDirectDrawSurface_Lock(v->color,NULL,&d,DDLOCK_WAIT|DDLOCK_READONLY,NULL)!=DS_OK)return 0;color=*(unsigned short*)((unsigned char*)d.lpSurface+y*d.lPitch+x*2);IDirectDrawSurface_Unlock(v->color,NULL);return color;}
static void native_render(void){Fixture f;NativeFixture binding;BHSceneView view;BHTextureContext textures;BHRenderMaterial material;BHRenderState render;BHNativePrimitive native;BHPcx pcx;unsigned char pixels[1024];
 BHModel model;BHBspVertex vertices[3];BHBspEdge edges[3];BHBspFace face;BHBspNode node;BHBspLeaf leaf;BHTextureImage *images[1];BHModelDrawCache cache;BHClipContext clip;BHScreenClipContext screen_clip;BHModelInstanceContext instance;BHDrawResourceContext draw;BHDrawSubmitContext submit;BHPickupDrawContext rect;
 unsigned long i,mode,next;unsigned short label[2],body[2];
 memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&cache,0,sizeof(cache));memset(&binding,0,sizeof(binding));memset(&native,0,sizeof(native));
 fixture(&f,32);CHECK(bh_scene_view_init(&view,640,480),"private real native offscreen D3D2 target without window/game/input loop");if(!view.device)goto done;
 CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL)&&bh_texture_formats(&textures),"real native texture interfaces and formats");textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
 memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[3]=pcx.palette[4]=pcx.palette[5]=255;
 binding.image=bh_texture_stage(&textures,"TARGETWHITE.PCX",&pcx,0,0);CHECK(binding.image!=NULL,"real white synthetic atlas/model image staged");if(!binding.image)goto done;binding.image->flags=1;images[0]=binding.image;
 CHECK(!bh_render_material_initialize(&material,&textures,view.d3d)&&!material.failures&&bh_render_state_init(&render,&textures),"complete native material/state owners");
 CHECK(bh_view_config_init(&f.view,640,480)&&bh_main_camera_settings(&f.view,60,1,100)&&bh_main_camera_pose(&f.view,&f.camera,0,0,0,0,0,0),"actual source camera owner builds native model/overlay projection");view.original_depth=1;
 memset(&model,0,sizeof(model));memset(vertices,0,sizeof(vertices));memset(edges,0,sizeof(edges));memset(&face,0,sizeof(face));memset(&node,0,sizeof(node));memset(&leaf,0,sizeof(leaf));
 model.radius=2;model.vertices=vertices;model.vertex_count=3;model.edges=edges;model.edge_count=3;model.faces=&face;model.face_count=1;model.nodes=&node;model.node_count=1;model.leaves=&leaf;model.leaf_count=1;model.images=images;model.material_count=1;
 vertices[0].x=-1;vertices[0].y=-1;vertices[1].y=1;vertices[2].x=1;vertices[2].y=-1;
 for(i=0;i<3;++i){edges[i].vertex=i;edges[i].next=i==2?-1:(long)i+1;edges[i].u=edges[i].v=.5f;node.minimum[i]=-1;node.maximum[i]=1;}
 node.normal[2]=1;node.front_node=node.back_node=-1;node.front_leaf=node.back_leaf=0;leaf.next=-1;
 CHECK(bh_model_draw_cache_init(&cache,&model,NULL)&&bh_clip_init(&clip,1,100,0,640,0,480)&&bh_screen_clip_init(&screen_clip,1,100,0,640,0,480),"actual model cache and complete model/rectangle clippers");
 native.textures=&textures;native.near_z=1;memset(&instance,0,sizeof(instance));instance.model=&model;instance.cache=&cache;instance.camera=&f.camera;instance.config=&f.view;instance.clip=&clip;instance.texture_mask=1;instance.base_color=0x80000000UL;instance.emit=bh_native_primitive_emit;instance.emit_user=&native;
 binding.resource.tag=BH_RESOURCE_TAG_MODEL;binding.resource.data.model.instance=&instance;memset(&draw,0,sizeof(draw));draw.camera=&f.camera;draw.config=&f.view;submit.draw=&draw;submit.lookup=lookup_model;submit.lookup_user=&binding;
 memset(&rect,0,sizeof(rect));rect.camera=&f.camera;rect.view=&f.view;rect.clip=&screen_clip;rect.emit=bh_native_primitive_emit;rect.emit_user=&native;
 f.state.local_anchor.x=4;f.state.local_anchor.y=f.state.local_anchor.z=0;f.state.flags=0;memset((unsigned char*)&f.entity+0x3c,0,12);
 for(mode=0;mode<2;++mode){f.state.owner=mode?&f.entity:&f.player;bh_target_draw_state_write(&f.entity,&f.state);bh_draw_queue_begin(&f.queue);
  CHECK(bh_target_draw(&f.context,&f.entity)==1,"whole target owner queues native positive/owner-negative control");CHECK(bh_scene_view_begin(&view),"begin actual private offscreen scene");
  CHECK(bh_draw_submit_execute(&submit,f.queue.commands+9,&next)&&instance.drawn&&instance.polygons_emitted==mode+1,"actual model queue consumer traverses real BSP/face/clip/native owners");
  for(i=10;i<f.queue.count;++i)CHECK(bh_pickup_rect_execute(&rect,lookup_image,&binding,f.queue.commands+i,&next),"actual queued overlay rectangle consumer and native emitter");
  CHECK(binding.image->vertex_count==(mode?3:19)&&binding.image->run_count==(mode?1:5),"whole native queue contains actual model triangle and every overlay quad");
  CHECK(bh_render_mode(&render,0)&&!binding.image->vertex_count&&!binding.image->run_count,"actual native residency/mode barrier consumes all fans through DrawPrimitive");
  CHECK(IDirect3DDevice2_EndScene(view.device)==D3D_OK,"end actual native scene without a UI image/screenshot");view.begun=0;label[mode]=pixel(&view,320,216);body[mode]=pixel(&view,542,240);
 }
 CHECK(label[0]!=0&&label[1]==0,"native offscreen pixels prove complete visible overlay versus wrong-owner blank control");CHECK(body[0]!=0&&body[1]!=0,"actual offset model remains visible in both controls");
done:
 bh_model_draw_cache_dispose(&cache);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);finish(&f);
}
int __cdecl bh_target_draw_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;queue_contract();paths_and_capacity();native_render();bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"target-draw\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_TARGET_DRAW_STANDALONE
int main(void){unsigned long count;return bh_target_draw_checkpoint(&count);}
#endif
