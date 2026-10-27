#include "bh_horizon.h"
#include "bh_round.h"
#include "bh_texture_draw.h"
#include <math.h>
#include <float.h>
#include <string.h>
static void error(BHHorizonContext *c,BHHorizonError e){if(c->error==BH_HORIZON_OK)c->error=e;}
static int native(BHHorizonContext *c,HRESULT h) {
    ++c->state->native_calls;c->state->last_hresult=h;
    if(FAILED(h)){++c->state->failures;error(c,BH_HORIZON_BACKEND);return 0;}return 1;
}
static void state(BHHorizonContext *c,D3DRENDERSTATETYPE type,unsigned long value) {
    native(c,IDirect3DDevice2_SetRenderState(c->state->textures->device,type,value));
}
int __cdecl bh_horizon_init(BHHorizonContext *c,const BHMainCamera *camera,BHAttributeClipContext *clip,
    BHRenderState *render,BHHorizonColor *colors,BHHorizonEmit emit,void *user) {
    if(!c || !camera || !clip || !render || !render->textures || !emit ||
        (render->textures->software && (!colors || colors->textures!=render->textures)))return 0;
    memset(c,0,sizeof(*c));c->camera=camera;c->clip=clip;c->state=render;c->colors=colors;c->emit=emit;c->user=user;c->color_mode=1;return 1;
}
/* UV normally consumes retained world sums before their binary32 stores.
   First inner X and last inner Z instead reload the stored local coordinate. */
static void vertex(BHClipVertex *v,const BHMat34 *projected,double x,double y,double z,float inverse_scale,
    int reload_x,int reload_z,unsigned long alpha) {
    BHVec3 world;bh_round_float(&world.x,x);bh_round_float(&world.y,y);bh_round_float(&world.z,z);
    bh_round_float(&v->u,(reload_x?(double)world.x:x)*inverse_scale);
    bh_round_float(&v->v,(reload_z?(double)world.z:z)*inverse_scale);
    bh_mat34_transform_point(projected,&world,&v->point);v->unknown28=alpha;v->projected=0;
}
static void polygon(BHHorizonContext *c,BHTextureImage *image,BHClipVertex **v,unsigned long n,int gradient) {
    BHClipVertex *ring;unsigned long i;
    for(i=0;i<n;++i){v[i]->next=v[(i+1)%n];v[i]->previous=v[(i+n-1)%n];}
    ++c->attempted;ring=bh_attribute_clip_polygon(c->clip,v[0],1);
    if(!ring){if(c->clip->error!=BH_CLIP_OK){if(!c->first_clip_error){c->first_clip_error=c->clip->error;c->failed_polygon=c->attempted;}error(c,BH_HORIZON_CLIP);}return;}
    if(!c->emit(ring,image,gradient,c->rgb,c->user))error(c,BH_HORIZON_EMIT);else ++c->emitted;
}
int __cdecl bh_horizon_draw(BHHorizonContext *c,BHTextureImage *image,float texture_scale,float height,
    float outer_radius,float inner_radius,unsigned long color) {
    BHTextureContext *q;BHTextureImage *im,*slow,*fast;BHClipVertex v[9],*ring[4];
    BHVec3 inner[2],outer[2];float sine,cosine,negative_sine,ratio_stored,factor[2][3],inverse_scale;
    double ratio,sx,sz,x,z;unsigned long old_depth,i,j;long signed_color=(long)color;
    if(!c || !c->camera || !c->clip || !c->state || !(q=c->state->textures) || !c->emit || !image ||
        (!q->software && !q->device) || (q->software && (!c->colors || c->colors->textures!=q)) ||
        !_finite(texture_scale) || !texture_scale || !_finite(height) || !_finite(outer_radius) || !outer_radius || !_finite(inner_radius))return 0;
    c->error=BH_HORIZON_OK;c->first_clip_error=BH_CLIP_OK;c->failed_polygon=0;c->attempted=c->emitted=0;
    for(i=0;i<12;++i)if(!_finite(((const float *)&c->camera->projected)[i])){error(c,BH_HORIZON_ARGUMENT);return 0;}
    if(!_finite(c->camera->position.x) || !_finite(c->camera->position.z) || !_finite(c->camera->angles.x)){error(c,BH_HORIZON_ARGUMENT);return 0;}
    if(!q->software){slow=fast=q->images;while(fast && fast->next){slow=slow->next;fast=fast->next->next;if(slow==fast){error(c,BH_HORIZON_ARGUMENT);return 0;}}}
    old_depth=c->state->depth;
    bh_round_float(&sine,sin((double)c->camera->angles.x));bh_round_float(&cosine,cos((double)c->camera->angles.x));
    negative_sine=-sine;
    if(!bh_render_depth(c->state,2))error(c,BH_HORIZON_BACKEND);
    c->rgb[0]=(float)((unsigned long)(signed_color/65536L)&255UL);
    c->rgb[1]=(float)((unsigned long)(signed_color/256L)&255UL);c->rgb[2]=(float)(color&255UL);
    if(q->software && !bh_horizon_color_prepare(c->colors,c->color_mode,color))error(c,BH_HORIZON_COLOR);
    ratio=(double)height/outer_radius;bh_round_float(&ratio_stored,ratio);
    bh_round_float(&inverse_scale,1.0/texture_scale);
    sx=(double)sine-cosine;sz=(double)cosine-negative_sine;
    bh_round_float(&factor[0][0],sx);factor[0][1]=ratio_stored;bh_round_float(&factor[0][2],sz);
    bh_round_float(&inner[0].x,sx*inner_radius);bh_round_float(&inner[0].y,ratio*inner_radius);bh_round_float(&inner[0].z,sz*inner_radius);
    sx=(double)cosine+sine;sz=(double)negative_sine+cosine;
    bh_round_float(&factor[1][0],sx);factor[1][1]=ratio_stored;bh_round_float(&factor[1][2],sz);
    bh_round_float(&inner[1].x,sx*inner_radius);bh_round_float(&inner[1].y,ratio*inner_radius);bh_round_float(&inner[1].z,sz*inner_radius);
    for(i=0;i<2;++i){bh_round_float(&outer[i].x,(double)factor[i][0]*outer_radius);bh_round_float(&outer[i].y,(double)factor[i][1]*outer_radius);bh_round_float(&outer[i].z,(double)factor[i][2]*outer_radius);}
    memset(v,0,sizeof(v));x=c->camera->position.x;z=c->camera->position.z;
    vertex(v,&c->camera->projected,x,-height,z,inverse_scale,0,0,0);
    for(i=0;i<4;++i){j=i&1;
        vertex(v+1+i,&c->camera->projected,i<2?x+inner[j].x:x-inner[j].x,
            i<2?(double)-height+inner[j].y:(double)-height-inner[j].y,
            i<2?z+inner[j].z:z-inner[j].z,inverse_scale,i==0,i==3,0);
        vertex(v+5+i,&c->camera->projected,i<2?x+outer[j].x:x-outer[j].x,
            i<2?(double)-height+outer[j].y:(double)-height-outer[j].y,
            i<2?z+outer[j].z:z-outer[j].z,inverse_scale,0,0,0x3f800000UL);
    }
    if(!q->software){state(c,D3DRENDERSTATE_FOGENABLE,1);state(c,D3DRENDERSTATE_FOGCOLOR,color);}
    for(i=0;i<4;++i){ring[0]=v+1+i;ring[1]=v+1+(i+1)%4;ring[2]=v;polygon(c,image,ring,3,!q->software);}
    for(i=0;i<4;++i){ring[0]=v+5+i;ring[1]=v+5+(i+1)%4;ring[2]=v+1+(i+1)%4;ring[3]=v+1+i;polygon(c,image,ring,4,1);}
    if(!q->software){q->frame=(long)((unsigned long)q->frame+1);++c->state->barriers;
        for(im=q->images;im;im=im->next)if(!bh_texture_draw_flush(q,im,NULL))error(c,BH_HORIZON_BACKEND);
        native(c,IDirect3DDevice2_EndScene(q->device));native(c,IDirect3DDevice2_BeginScene(q->device));}
    if(!bh_render_depth(c->state,old_depth))error(c,BH_HORIZON_BACKEND);
    if(!q->software)state(c,D3DRENDERSTATE_FOGENABLE,0);
    return c->error==BH_HORIZON_OK;
}
int __cdecl bh_horizon_native_emit(BHClipVertex *ring,BHTextureImage *image,int gradient,const float rgb[3],void *user) {
    BHHorizonNative *n=(BHHorizonNative *)user;BHTextureContext *q;BHClipVertex *p;D3DTLVERTEX *vertices;
    unsigned long count=0,i;float scalar;double fog;int ok;
    if(!n || !(q=n->textures) || q->software || !_finite(n->near_z) || n->near_z<=0 || !q->allocator.allocate || !q->allocator.release || !ring || !image || !gradient)return 0;
    p=ring;do{if(!p || !p->next || !p->previous || p->next->previous!=p || p->previous->next!=p || ++count>2048)return 0;
        memcpy(&scalar,&p->unknown28,4);fog=(double)scalar*-255.0;
        if(!_finite(p->sx) || !_finite(p->sy) || !_finite(p->point.z) || p->point.z<=0 || !_finite(p->u) || !_finite(p->v) || !_finite(fog) || fog<-2147483648.0 || fog>2147483647.0)return 0;
        p=p->next;}while(p!=ring);if(count<3)return 0;
    vertices=(D3DTLVERTEX *)q->allocator.allocate(count*sizeof(*vertices),q->allocator.user);if(!vertices)return 0;
    memset(vertices,0,count*sizeof(*vertices));p=ring;
    for(i=0;i<count;++i){vertices[i].sx=p->sx;vertices[i].sy=p->sy;bh_round_float(&vertices[i].sz,(double)n->near_z/p->point.z);bh_round_float(&vertices[i].rhw,1.0/p->point.z);
        memcpy(&scalar,&p->unknown28,4);fog=(double)scalar*-255.0;
        vertices[i].color=0xffffffUL;vertices[i].specular=0xff000000UL+(unsigned long)(long)fog*0x01000000UL;
        vertices[i].tu=p->u;vertices[i].tv=p->v;p=p->next;}
    ok=bh_texture_draw_polygon(q,image,vertices,count);q->allocator.release(vertices,q->allocator.user);
    if(ok){++n->polygons;n->vertices+=count;}return ok;
}
