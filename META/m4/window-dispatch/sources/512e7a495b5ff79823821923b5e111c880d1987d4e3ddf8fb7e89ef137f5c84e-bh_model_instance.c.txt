#include "bh_model_instance.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>
static const BHMat34 identity={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};

static int fail(BHModelInstanceContext *c,BHModelInstanceError error) {
    if(c)c->error=error;
    return 0;
}
static int draw_fail(BHModelInstanceContext *c,const BHModelDrawContext *draw) {
    c->color=draw->color;
    c->faces_marked+=draw->faces_marked;
    c->polygons_emitted+=draw->polygons_emitted;
    return fail(c,BH_MODEL_INSTANCE_DRAW);
}

/* 0045aa60 uses the six input doubles directly for sphere tests. In
   particular, truncating the position to the model's float translation here
   changes visibility for centres close to a plane. */
static double distance_to(const BHViewPlane *p,double x,double y,double z) {
    return (((double)p->normal.x*x+(double)p->normal.y*y)+(double)p->normal.z*z)-p->offset;
}

int __cdecl bh_model_instance_draw(BHModelInstanceContext *c,
    double x,double y,double z,double rotate_y,double rotate_x,double rotate_z,
    float opacity) {
    unsigned long mask=63,bit,slot,next_frame;double depth,d;
    float reference_radius;int grayscale;
    BHMat34 world_to_model,model_camera;
    BHVec3 world_reference;
    BHViewCamera view;
    BHModelDrawContext draw;
    BHModelDrawLitContext lit;
    if(!c || !c->model || !c->cache || c->cache->model!=c->model || !c->camera ||
       !c->config || !c->clip || !c->emit ||
       !_finite(x) || !_finite(y) || !_finite(z) || !_finite(rotate_y) ||
       !_finite(rotate_x) || !_finite(rotate_z) || !_finite(opacity) ||
       !_finite(c->model->radius) || c->model->radius<0 ||
       !_finite(c->config->near_z) || !_finite(c->config->far_z))return fail(c,BH_MODEL_INSTANCE_ARGUMENT);
    if((c->light_config && c->light_config->enabled) || c->point_enabled) {
        if(!c->light_config || (c->point_enabled && (!c->lights || !c->light_reference)))
            return fail(c,BH_MODEL_INSTANCE_ARGUMENT);
    }
    c->error=BH_MODEL_INSTANCE_OK;c->drawn=0;
    reference_radius=c->model->radius;
    depth=distance_to(c->camera->planes,x,y,z);
    if(depth<(double)c->config->near_z-reference_radius ||
       (double)c->config->far_z+reference_radius<depth)return 1;
    if((double)c->config->near_z+reference_radius<depth)mask=0x3e;
    if(depth<(double)c->config->far_z-reference_radius)mask-=2;
    for(bit=4,slot=2;bit<=32;bit*=2,++slot) {
        d=distance_to(c->camera->planes+slot,x,y,z);
        if(reference_radius<d)return 1;
        if(d<-(double)reference_radius)mask-=bit;
    }
    d=(double)opacity*-255.0;
    if(!_finite(d) || d<-2147483648.0 || d>2147483647.0)return fail(c,BH_MODEL_INSTANCE_NUMERICAL);
    grayscale=(int)d;next_frame=c->frame+1;
    if(!next_frame)return fail(c,BH_MODEL_INSTANCE_NUMERICAL);
    c->model->transform=identity;
    bh_mat34_translate_local((float)x,(float)y,(float)z,&c->model->transform);
    bh_mat34_rotate_y((float)rotate_y,&c->model->transform);
    bh_mat34_rotate_x((float)rotate_x,&c->model->transform);
    bh_mat34_rotate_z((float)rotate_z,&c->model->transform);
    c->color=c->base_color-(unsigned long)grayscale*0x10101UL;
    c->opacity=opacity;c->frame=next_frame;
    bh_mat34_multiply(&c->camera->view,&c->model->transform,&model_camera);
    if(!bh_view_camera_build(c->config,&model_camera,&view))return fail(c,BH_MODEL_INSTANCE_CAMERA);
    if(!bh_model_draw_context_init(&draw,c->model,c->cache,&view,c->clip,c->frame,
         c->texture_mask,c->color,c->emit,c->emit_user))return fail(c,BH_MODEL_INSTANCE_DRAW);
    if(!c->light_config || (!c->light_config->enabled && !c->point_enabled)) {
        if(!bh_model_draw_unlit(&draw,0,mask))return draw_fail(c,&draw);
    }else {
        bh_mat34_inverse_rigid(&c->model->transform,&world_to_model);
        if(c->point_enabled) {
            /* The original transforms a fixed model-local reference to world
               space, then prepares each borrowed light using the inverse. */
            bh_mat34_transform_point(&c->model->transform,c->light_reference,&world_reference);
            c->lights->world_to_model=world_to_model;
            if(!bh_light_prepare(c->lights,&world_reference,reference_radius))return fail(c,BH_MODEL_INSTANCE_LIGHT);
        }
        if(!bh_model_draw_lit_context_init(&lit,&draw,c->lights,c->light_config,
            &world_to_model,c->point_enabled,opacity,c->base_color) ||
           !bh_model_draw_lit(&lit,0,mask))return draw_fail(c,&draw);
    }
    c->color=draw.color;
    c->last_camera_view=c->camera->view;
    c->faces_marked+=draw.faces_marked;
    c->polygons_emitted+=draw.polygons_emitted;
    c->drawn=1;
    return 1;
}
