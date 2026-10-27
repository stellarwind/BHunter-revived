#include "bh_scene_view.h"
#include "bh_primitive_dispatch.h"
#include "bh_blocks.h"
#include "bh_geometry.h"
#include "bh_rotation.h"
#include "bh_fp.h"
#include "bh_texture.h"
#include "bh_model.h"
#include "bh_model_path.h"
#include "bh_main_camera.h"
#include "bh_render_material.h"
#include "bh_render_state.h"
#include "bh_model_draw.h"
#include "bh_native_primitive.h"
#include "bh_model_draw_lit.h"
#include "bh_model_instance.h"
#include "bh_draw_resource.h"
#include "bh_draw_submit.h"
#include "bh_render_resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Source-only static checkpoint. Real textures and diagnostic camera; no
   script/entity construction, material fidelity or gameplay admission. */

static unsigned long rejected_capture_faces;
static int capture_emit(BHClipVertex *v,BHTextureImage *image,unsigned long color,void *user){
    if(!bh_primitive_emit((BHPrimitiveDispatch*)user,image,v,color))++rejected_capture_faces;
    return 1; /* Capture-only diagnostic: report rejected faces, never game admission. */
}
static int save_capture(const char *name,const unsigned char *pixels,unsigned long w,unsigned long h){
    BITMAPFILEHEADER file;BITMAPINFOHEADER info;unsigned long x,y,stride=(w*3+3)&~3UL;unsigned char *row;FILE *out;
    memset(&file,0,sizeof(file));memset(&info,0,sizeof(info));file.bfType=0x4d42;file.bfOffBits=54;file.bfSize=54+stride*h;
    info.biSize=40;info.biWidth=w;info.biHeight=h;info.biPlanes=1;info.biBitCount=24;info.biSizeImage=stride*h;
    row=(unsigned char*)calloc(stride,1);out=fopen(name,"wb");if(!row||!out)return 0;
    fwrite(&file,14,1,out);fwrite(&info,40,1,out);
    for(y=h;y;y--){for(x=0;x<w;x++){unsigned long index=pixels[(y-1)*w+x];row[x*3]=(unsigned char)((index%6)*255/5);row[x*3+1]=(unsigned char)(((index/6)%7)*255/6);row[x*3+2]=(unsigned char)(((index/42)%6)*255/5);}fwrite(row,1,stride,out);}
    free(row);return fclose(out)==0;
}

int main(int argc,char **argv) {
    BHPrimitiveDispatch primitive;BHSoftwareRaster cpu_frame;BHSoftwareTables cpu_tables;BHHorizonColor cpu_gradient;unsigned char *cpu_pixels=NULL;unsigned short *cpu_depth=NULL;

    BHBlockLibrary library;BHSceneLevel level;BHGeometry *geometry=NULL;unsigned char *shell=NULL;
    BHSceneView view;BHTextureContext textures;int textures_live=0;BHModelContext model_context;BHRenderResources resources;unsigned long *handles=NULL;BHDrawQueue queue;BHDrawSubmitContext submit;
    BHMathEnvironment environment;BHVFile *file=NULL;
    BHFaceInfo face;BHMainCamera main_camera;
    BHClipContext clip;BHNativePrimitive native;BHModelInstanceContext instance;BHDrawResourceContext resource_draw;BHVec3 light_reference={0,0,0};
    BHLightConfig light_config;BHLightContext lights;BHLight point_light;
    BHRenderMaterial native_material;BHRenderState native_state;
    BHVec3 camera={-60,-17,60};unsigned long i,j,k,initialized=0,faces=0,max_vertices=0,shells=0;
    double yaw=3.141592653589793,pitch=0,roll=0;
    long model;int ok=0,fp=0,lighting_test=0;float angle;unsigned long texture_count=0,textured_faces=0;
    memset(&cpu_tables,0,sizeof(cpu_tables));memset(&cpu_gradient,0,sizeof(cpu_gradient));memset(&cpu_frame,0,sizeof(cpu_frame));
    if(argc!=3 && argc!=9 && argc!=10){printf("usage: bhunter-scene-snapshot archive output.bmp [x y z yaw-deg pitch-deg roll-deg [red|green|point]]\n");return 2;}
    if(argc>=9) {
        camera.x=(float)atof(argv[3]);camera.y=(float)atof(argv[4]);camera.z=(float)atof(argv[5]);
        yaw=atof(argv[6])*0.017453292519943295;pitch=atof(argv[7])*0.017453292519943295;roll=atof(argv[8])*0.017453292519943295;
    }
    if(argc==10){if(!_stricmp(argv[9],"red"))lighting_test=1;else if(!_stricmp(argv[9],"green"))lighting_test=2;else if(!_stricmp(argv[9],"point"))lighting_test=3;else return 2;}
    memset(&resources,0,sizeof(resources));memset(&queue,0,sizeof(queue));
    memset(&view,0,sizeof(view));memset(&native_material,0,sizeof(native_material));memset(&main_camera,0,sizeof(main_camera));bh_blocks_init(&library,NULL);bh_scene_level_init(&level,NULL);
    fp=bh_math_environment_enter(&environment);if(!fp)goto done;
    memset(&lights,0,sizeof(lights));memset(&point_light,0,sizeof(point_light));memset(&light_config,0,sizeof(light_config));
    if(lighting_test) {
        /* Explicit diagnostic lights, not recovered level/script setup. The
           complete original configuration/preparation/evaluator/traversal
           owners consume them and produce actual native modulation colors. */
        if(!bh_light_configure(&light_config,1,0,1,0,lighting_test==3?0:.85f,lighting_test==3?.06f:.12f,
            lighting_test==1?0xff5050UL:0x50ff50UL,0xffffffUL))goto done;
        if(lighting_test==3){point_light.type=3;point_light.color=0x80bfffUL;point_light.intensity=1;point_light.range=300;point_light.position=camera;lights.lights=&point_light;}
    }
    if(!bh_vfile_mount(argv[1]) || !bh_blocks_load(&library,"BSP\\BLOCKS.TXT","BSP\\",BH_BSP_PREFIX_ONLY))goto done;
    file=bh_vfile_open("TEST.LEV","rb");if(!file || !bh_scene_level_read(&level,file))goto done;
    bh_vfile_close(file);file=NULL;
    geometry=(BHGeometry *)calloc(library.count,sizeof(*geometry));if(!geometry)goto done;
    shell=(unsigned char *)calloc(library.count,1);if(!shell)goto done;
    for(i=0;i<library.count;++i) {
        if(!bh_geometry_init(geometry+i,&library.blocks[i].bsp,NULL))goto done;++initialized;
        memset(&face,0,sizeof(face));
        for(j=0;j<library.blocks[i].bsp.counts[3];++j) {
            if(!bh_geometry_face(geometry+i,j,&face))goto done;
            if(!_stricmp(face.texture,"PORTAL.GIF"))shell[i]=1;
            if(max_vertices<face.vertices)max_vertices=face.vertices;
        }
    }
    if(max_vertices>1024 || max_vertices<3)goto done;
    if(!bh_scene_view_init(&view,1024,576) || !bh_view_config_init(&view.projection,1024,576))goto done;
    if(!bh_main_camera_settings(&view.projection,60,1,100000) ||
        !bh_main_camera_pose(&view.projection,&main_camera,camera.x,camera.y,camera.z,yaw,pitch,roll))goto done;
    view.original_depth=1;
    if(!bh_texture_context_init_software(&textures,view.draw,NULL))goto done;textures_live=1;
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    if(!bh_texture_formats(&textures))goto done;
    if(!bh_texture_set_path(&textures,"TEXTURES",2))goto done;
    if(!bh_model_context_init(&model_context,&textures,NULL) || !bh_model_path_set(&model_context,"BSP\\",BH_BSP_PREFIX_ONLY))goto done;
    if(!bh_clip_init(&clip,view.projection.near_z,view.projection.far_z,view.projection.left,view.projection.right,view.projection.top,view.projection.bottom))goto done;
    memset(&instance,0,sizeof(instance));instance.camera=&main_camera;instance.config=&view.projection;instance.clip=&clip;
    instance.lights=&lights;instance.light_config=&light_config;instance.light_reference=&light_reference;instance.point_enabled=lighting_test==3;
    instance.emit=capture_emit;instance.emit_user=&primitive;instance.texture_mask=0xffffffffUL;instance.base_color=0xff000000UL;
    if(!bh_render_resources_init(&resources,&model_context,&instance,NULL))goto done;
    handles=(unsigned long *)calloc(library.count,sizeof(*handles));if(!handles)goto done;
    for(i=0;i<library.count;++i)if(shell[i]) {
        handles[i]=bh_render_resources_load_model(&resources,library.blocks[i].name);if(!handles[i])goto done;
    }
    /* Individual image failures are explicitly retained as NULL by the model
       owner. They are not a model-load error or a fabricated replacement. */
    textures.error=BH_TEXTURE_OK;
    {BHTextureImage *image;for(image=textures.images;image;image=image->next)++texture_count;}
    /* The static diagnostic retains earlier NULL-image diagnostics. The
       recovered initializer returns those preexisting strings after doing all
       native work; require its actual material/handle/calls instead of treating
       an already recorded image miss as a new native initialization failure. */
    if(!bh_render_state_init(&native_state,&textures))goto done;
    cpu_pixels=(unsigned char*)calloc(1024*576,1);cpu_depth=(unsigned short*)calloc(1024*576,2);if(!cpu_pixels||!cpu_depth)goto done;
    if(!bh_software_tables_initialize(&cpu_tables,&textures,1,NULL)||!bh_horizon_color_init(&cpu_gradient,&textures)||!bh_horizon_color_prepare(&cpu_gradient,1,0x204060))goto done;
    cpu_frame.pixels=cpu_pixels;cpu_frame.depth=cpu_depth;cpu_frame.pitch=1024;cpu_frame.depth_pitch=2048;cpu_frame.width=1024;cpu_frame.height=576;cpu_frame.near_z=1;cpu_frame.tables=&cpu_tables;cpu_frame.gradient=&cpu_gradient;
    if(!bh_primitive_dispatch_init(&primitive,&native_state,&cpu_frame,0,1024,0,576))goto done;
    if(!bh_render_mode(&native_state,0))goto done;
    memset(&resource_draw,0,sizeof(resource_draw));resource_draw.camera=&main_camera;resource_draw.config=&view.projection;
    submit.draw=&resource_draw;submit.lookup=bh_render_resources_lookup;submit.lookup_user=&resources;
    if(level.placement_count>65526 || !bh_draw_queue_init(&queue,level.placement_count+9,NULL))goto done;
    for(i=0;i<level.placement_count;++i) {
        BHScenePlacement *placement=level.placements+i;BHVec3 position,angles;
        model=bh_blocks_find(&library,placement->name);if(model<0)goto done;
        /* This first view draws navigation shell models only. Non-shell
           placements include invisible sensor/script proxy meshes as well as
           visible props; their actual draw ownership remains unrecovered. */
        if(!shell[model])continue;++shells;
        angle=(float)((placement->flags&3UL)*1.5707963267948966);
        position.x=(float)placement->x;position.y=(float)placement->y;position.z=(float)placement->z;
        angles.x=angle;angles.y=angles.z=0;
        if(bh_draw_submit_resource(&queue,0,handles[model],&position,&angles,1)!=1)goto done;
    }
    memset(&native,0,sizeof(native));native.textures=&textures;native.near_z=view.projection.near_z;
    /* Static source checkpoint: real resource callbacks and state headers are
       consumed in recovered bucket-chain order. Entity scheduling is separate. */
    {unsigned long index=0,steps=0,next;
        while(index!=BH_DRAW_END){BHDrawCommand *command;
            if(index>=queue.count || ++steps>queue.count)goto done;command=queue.commands+index;next=command->next;
            if(command->kind==BH_DRAW_MODEL_RESOURCE){if(!bh_draw_submit_execute(&submit,command,&next))goto done;}
            else if(command->kind==BH_DRAW_STATE){if(!bh_render_mode(&native_state,command->words[0]))goto done;}
            else if(command->kind!=BH_DRAW_NOP)goto done;
            index=next;
        }
    }
    view.triangles=primitive.split.leaves;for(i=0;i<1024*576;i++)if(cpu_pixels[i])++view.pixels;
    for(i=0;i<library.count;++i)if(handles[i]){const BHDrawResource *resource=bh_render_resources_lookup(handles[i],&resources);
        faces+=resource->data.model.instance->faces_marked;textured_faces+=resource->data.model.instance->polygons_emitted;}
    if(!bh_render_mode(&native_state,0))goto done;
    if(!save_capture(argv[2],cpu_pixels,1024,576)||!view.triangles||!view.pixels)goto done;
    ok=1;
done:
    printf("{\"scenario\":\"source-scene-snapshot\",\"passed\":%s,\"models\":%lu,\"placements\":%lu,\"shell_placements\":%lu,\"faces\":%lu,\"triangles\":%lu,\"nonblack_pixels\":%lu,\"hresult\":%ld,\"unapplied_scripts\":%lu,\"textures\":%lu,\"textured_faces\":%lu,\"camera\":[%.8g,%.8g,%.8g],\"forward\":[%.8g,%.8g,%.8g],\"lighting_test\":%d,\"textured\":true,\"original_code_required\":false}\n",
        ok?"true":"false",library.count,level.placement_count,shells,faces,view.triangles,view.pixels,(long)view.error,library.unapplied_scripts,texture_count,textured_faces,
        camera.x,camera.y,camera.z,main_camera.inverse.m[0][2],main_camera.inverse.m[1][2],main_camera.inverse.m[2][2],lighting_test);
    for(i=0;i<initialized;++i)bh_geometry_dispose(geometry+i);free(geometry);
    bh_draw_queue_dispose(&queue);bh_render_resources_dispose(&resources);free(handles);
    printf("capture-only rejected faces: %lu; source software polygons: %lu\n",rejected_capture_faces,primitive.polygons);
    bh_horizon_color_dispose(&cpu_gradient);bh_software_tables_dispose(&cpu_tables);free(cpu_pixels);free(cpu_depth);
    bh_render_material_dispose(&native_material);
    if(textures_live)bh_texture_context_dispose(&textures);
    bh_scene_view_dispose(&view);free(shell);
    if(file)bh_vfile_close(file);bh_scene_level_dispose(&level);bh_blocks_dispose(&library);bh_vfile_shutdown();
    if(fp)bh_math_environment_leave(&environment);return ok?0:1;
}
