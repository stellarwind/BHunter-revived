#include "../src/bh_render_resources.h"
#include "../src/bh_entity_resource_draw.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) check((x)!=0,s,__LINE__)
static void check(int ok,const char *s,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL queued native %d: %s\n",line,s);}}
static BHTextureImage *__cdecl glow_lookup(unsigned long handle,void *user){return handle==77?(BHTextureImage *)user:NULL;}
int bh_queued_resource_native_checkpoint(const char *archive,unsigned long *count){
 BHMathEnvironment fp;BHSceneView view;BHTextureContext textures;BHModelContext models;BHRenderResources resources;
 BHMainCamera camera;BHClipContext clip;BHModelInstanceContext prototype;BHDrawResourceContext draw;BHDrawSubmitContext submit;
 BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;BHLightConfig light;
 BHSpriteBillboardContext billboard;BHScreenClipContext screen;BHDrawBillboardSubmitContext billboard_submit;BHTextureImage *glow;
 BHEntityPool pool;BHEntity *entity;BHEntityResourceDrawContext effect;BHDrawQueue queue;
 const BHDrawResource *resource;unsigned long handle=0,index,next,steps,dispatched,glows,mode,prior;float phase=0;int live=0;
 const char *names[]={"queued-entity-model.bmp","queued-entity-unbound.bmp"};
 checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
 memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&resources,0,sizeof(resources));memset(&material,0,sizeof(material));
 memset(&pool,0,sizeof(pool));memset(&queue,0,sizeof(queue));memset(&camera,0,sizeof(camera));memset(&light,0,sizeof(light));
 if(!archive)goto done;
 CHECK(bh_vfile_mount(archive),"real archive data mounted");
 CHECK(bh_scene_view_init(&view,96,96),"real native offscreen target");if(!view.device)goto done;
 CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL),"real native texture context");if(!textures.device)goto done;live=1;
 textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
 CHECK(bh_texture_formats(&textures) && bh_texture_set_path(&textures,"TEXTURES",2),"real texture formats/path");
 CHECK(bh_view_config_init(&view.projection,96,96) && bh_main_camera_settings(&view.projection,60,1,100000) && bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0),"recovered main camera");view.original_depth=1;
 CHECK(bh_clip_init(&clip,1,100000,0,96,0,96),"real model clipper");
 memset(&prototype,0,sizeof(prototype));memset(&native,0,sizeof(native));native.textures=&textures;native.near_z=1;
 prototype.camera=&camera;prototype.config=&view.projection;prototype.clip=&clip;prototype.light_config=&light;
 prototype.texture_mask=0xffffffffUL;prototype.base_color=0xff000000UL;prototype.emit=bh_native_primitive_emit;prototype.emit_user=&native;
 CHECK(bh_model_context_init(&models,&textures,NULL) && bh_render_resources_init(&resources,&models,&prototype,NULL),"source-owned model resource construction");
 handle=bh_render_resources_load_model(&resources,"BSP\\PU_9A.BSP");resource=bh_render_resources_lookup(handle,&resources);
 CHECK(handle && resource && resource->tag==BH_RESOURCE_TAG_MODEL,"real original pickup model becomes owned render descriptor");if(!resource)goto done;
 glow=bh_texture_load(&textures,"flare_yl.bmp");CHECK(glow!=NULL,"original 004f00b0 image identity loads through recovered BMP-to-PCX alias");if(!glow)goto done;
 CHECK(bh_screen_clip_init(&screen,1,100000,0,96,0,96),"real billboard clip binding");
 memset(&billboard,0,sizeof(billboard));billboard.camera=&camera;billboard.config=&view.projection;billboard.clip=&screen;
 billboard.base_color=0xff000000UL;billboard.emit=bh_native_primitive_emit;billboard.emit_user=&native;
 billboard_submit.draw=&billboard;billboard_submit.lookup=glow_lookup;billboard_submit.lookup_user=glow;
 textures.error=BH_TEXTURE_OK;
 bh_render_material_initialize(&material,&textures,view.d3d);
 CHECK(material.material && material.handle && !material.failures && bh_render_state_init(&state,&textures),"real native material and render-state owners");
 CHECK(bh_entities_init(&pool,1,NULL) && bh_draw_queue_init(&queue,12,NULL),"real entity/queue allocation owners");
 entity=bh_entity_acquire(&pool);CHECK(entity!=NULL,"real entity acquisition");if(!entity)goto done;
 entity->position.x=entity->position.y=0;entity->position.z=resource->data.model.instance->model->radius*6+4;
 memcpy(entity->payload+0x18,&phase,4);
 memset(&draw,0,sizeof(draw));draw.camera=&camera;draw.config=&view.projection;
 submit.draw=&draw;submit.lookup=bh_render_resources_lookup;submit.lookup_user=&resources;effect.queue=&queue;effect.glow_image=77;
 for(mode=0;mode<2;++mode){unsigned long binding=mode?handle+100:handle;
  effect.glow_image=mode?78:77;
  memcpy(entity->payload+0x30,&binding,4);CHECK(bh_draw_queue_begin(&queue),"begin source queue");
  CHECK(bh_entity_resource_draw_special(&effect,entity)==1,"real entity fields produce two model commands and a glow record");
  CHECK(bh_scene_view_begin(&view) && bh_render_mode(&state,0),"begin actual native scene");prior=native.polygons;index=0;steps=dispatched=glows=0;
  while(index!=BH_DRAW_END && steps++<queue.count){BHDrawCommand *command;
   CHECK(index<queue.count,"bounded source queue link");if(index>=queue.count)break;command=queue.commands+index;next=command->next;
   if(command->kind==BH_DRAW_MODEL_RESOURCE){int ok=bh_draw_submit_execute(&submit,command,&next);CHECK(ok==(mode==0),"real queued consumer resolves descriptor or reports absent binding");++dispatched;}
   else if(command->kind==BH_DRAW_BILLBOARD){++glows;CHECK(bh_draw_submit_billboard_execute(&billboard_submit,command,&next)==(mode==0),"queued glow consumer resolves original image or reports absent binding");}
   else if(command->kind==BH_DRAW_STATE)CHECK(bh_render_mode(&state,command->words[0]),"real queue header state transition");
   index=next;
  }
  CHECK(index==BH_DRAW_END && dispatched==2 && glows==1,"full source chain consumes both model callbacks and glow callback");
  CHECK(mode?native.polygons==prior:native.polygons>prior,"bound resource emits real polygons; missing binding emits none");
  CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,names[mode]),"real queue flush, rasterization and bitmap readback");
  CHECK(mode?!view.pixels:view.pixels>0,"actual entity/model pixels and unbound-resource blank control");
 }
done:
 bh_draw_queue_dispose(&queue);bh_entities_dispose(&pool);bh_render_resources_dispose(&resources);bh_render_material_dispose(&material);
 if(live)bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);if(archive)bh_vfile_shutdown();
 CHECK(bh_math_environment_supported(),"queued native composition preserves PC53");bh_math_environment_leave(&fp);*count=checks;
 printf("{\"scenario\":\"queued-resource-native\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}
