#include "bh_software_raster.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <float.h>
#include <string.h>
#include <stddef.h>

typedef char raster_screen_layout[offsetof(BHClipVertex,sx)==0x20&&offsetof(BHClipVertex,sy)==0x24?1:-1];
typedef char raster_uv_layout[offsetof(BHClipVertex,u)==0x18&&offsetof(BHClipVertex,v)==0x1c?1:-1];
typedef struct RasterEdge {BHClipVertex *below;float x,u,v,dx,du,dv,dz;double z;} RasterEdge;
typedef struct RasterGradient {long z,u,v;} RasterGradient;
static int fail(BHSoftwareRaster *c,BHRasterError e){if(c)c->error=e;return 0;}
static float stored(double x){float f;bh_round_float(&f,x);return f;}
static long add32(long a,long b){return (long)((unsigned long)a+(unsigned long)b);}
static long sub32(long a,long b){return (long)((unsigned long)a-(unsigned long)b);}
static int integer(BHSoftwareRaster *c,double x,long *v){
    if(!_finite(x)||x< -2147483648.0||x>2147483647.0)return fail(c,BH_RASTER_NUMERICAL);
    *v=(long)x;return 1;
}
static int gradient(BHSoftwareRaster *c,BHClipVertex *left,BHClipVertex *right,
    BHClipVertex *la,BHClipVertex *ra,int bottom,RasterGradient *out){
    double d,divisor,dy_left,dy_right,z,nu,nv;float rounded_d;
    double scale=(double)(c->texture_width-1);
    if(la==ra){
        dy_left=(double)left->sy-la->sy;dy_right=stored((double)right->sy-ra->sy);
        if(!bottom)dy_left=stored(dy_left);
        d=((double)right->sx-ra->sx)*dy_left-((double)left->sx-la->sx)*dy_right;
        rounded_d=stored(d);
        z=(((double)c->near_z/right->point.z-(double)c->near_z/ra->point.z)*dy_left-
           ((double)c->near_z/left->point.z-(double)c->near_z/la->point.z)*dy_right)*16777216.0/d;
        nu=((double)right->u-ra->u)*dy_left-((double)left->u-la->u)*dy_right;
        nv=((double)right->v-ra->v)*dy_left-((double)left->v-la->v)*dy_right;
        divisor=rounded_d;
    }else{
        d=(double)ra->sx-la->sx;divisor=bottom?d:(double)stored(d);
        z=((double)c->near_z/ra->point.z-(double)c->near_z/la->point.z)*16777216.0/divisor;
        nu=(double)ra->u-la->u;nv=(double)ra->v-la->v;
    }
    if(!integer(c,z,&out->z)||!integer(c,(nu*scale)*65536.0/divisor,&out->u)||
       !integer(c,(nv*scale)*65536.0/divisor,&out->v))return 0;
    return 1;
}
static BHClipVertex *walk(BHClipVertex *p,long y,int reverse,int below){
    unsigned long i=0;
    do{p=reverse?p->previous:p->next;if(++i>1024)return NULL;}
    while(below?(long)p->sy<=y:(long)p->sy>=y);
    return p;
}
static void left_edge(BHSoftwareRaster *c,RasterEdge *edge,long y){
    BHClipVertex *b=edge->below,*a=b->previous;double inv=1.0/((double)b->sy-a->sy);
    float inverse=stored(inv),delta=stored((double)(y+1)-a->sy);
    double scale=(double)(c->texture_width-1),bu=((double)a->u*scale)*65536.0;
    double bv=((double)a->v*scale)*65536.0,bz=((double)c->near_z/a->point.z)*16777216.0;
    double dx=((double)b->sx-a->sx)*inverse;
    double du=((((double)b->u*scale)*65536.0)-bu)*inv;
    double dv=((((double)b->v*scale)*65536.0)-bv)*inverse;
    double dz=(((double)c->near_z/b->point.z)*16777216.0-bz)*inverse;
    edge->dx=stored(dx);edge->du=stored(du);edge->dv=stored(dv);edge->dz=stored(dz);
    edge->x=stored(dx*delta+a->sx);edge->u=stored(du*delta+stored(bu));
    edge->v=stored(dv*delta+stored(bv));edge->z=dz*delta+stored(bz);
}
static void right_edge(RasterEdge *edge,long y){
    BHClipVertex *b=edge->below,*a=b->next;
    edge->dx=stored(((double)b->sx-a->sx)*(1.0/((double)b->sy-a->sy)));
    edge->x=stored(((double)(y+1)-a->sy)*edge->dx+a->sx);
}
static int run(BHSoftwareRaster *c,BHClipVertex *ring,int packed,int masked,int shaded,int blend,int policy){
    BHClipVertex *p,*minimum,*maximum,*a,*b;RasterEdge left,right;RasterGradient top,end,step;
    const unsigned char *shade_i;const unsigned short *shade_p;unsigned long n=0,i,mask,log=0,parity,texel,mapped;
    long first,last,y,x,count,u,v,z,rows;unsigned short depth,*depth_row,*destination;
    unsigned char *row;double sum=0,scalar;float value;
    if(!c||!ring||!c->pixels||!c->depth||!c->texels||!c->image||!c->tables||
       !c->image->dither||(!c->image->screen&&packed&&!shaded&&!blend)||
       c->tables->bytes_per_pixel!=(unsigned long)(packed?2:1)||!c->width||!c->height||
       c->width>4096||c->height>4096||!_finite(c->near_z)||c->near_z<=0||
       c->texture_width<2||c->texture_width>2048||(c->texture_width&(c->texture_width-1))||
       (c->pitch>=0?c->pitch:-(double)c->pitch)<c->width*(packed?2:1)||
       (c->depth_pitch>=0?c->depth_pitch:-(double)c->depth_pitch)<c->width*2||
       (c->depth_pitch&1)||!bh_math_environment_supported())return fail(c,BH_RASTER_ARGUMENT);
    c->error=BH_RASTER_OK;c->spans=c->visited=c->written=0;
    if(c->shade_offset>=131072-255||(blend&&c->blend_plane>=8))return fail(c,BH_RASTER_ARGUMENT);
    p=minimum=maximum=ring;
    do{
        if(!p||!p->next||!p->previous||p->next->previous!=p||p->previous->next!=p||++n>1024)return fail(c,BH_RASTER_TOPOLOGY);
        if(!_finite(p->sx)||!_finite(p->sy)||!_finite(p->u)||!_finite(p->v)||!_finite(p->point.z)||p->point.z<=0)return fail(c,BH_RASTER_NUMERICAL);
        if(p->sx<0||p->sx>c->width||p->sy<0||p->sy>c->height)return fail(c,BH_RASTER_BOUNDS);
        if(p->sy<minimum->sy)minimum=p;if(p->sy>maximum->sy)maximum=p;
        if(c->attribute){memcpy(&value,&p->unknown28,4);if(!_finite(value))return fail(c,BH_RASTER_NUMERICAL);sum+=value;}
        p=p->next;
    }while(p!=ring);
    if(n<3)return fail(c,BH_RASTER_TOPOLOGY);
    first=(long)minimum->sy;last=(long)maximum->sy;if(first>=last)return 1;
    shade_i=c->tables->shade_indexed?c->tables->shade_indexed+c->shade_offset:NULL;
    shade_p=c->tables->shade_native?c->tables->shade_native+c->shade_offset:NULL;
    if(c->attribute){
        scalar=255.0-(sum/n)*255.0;if(!integer(c,scalar,&x))return 0;
        if(x<0)x=0;else if(x>255)x=255;c->attribute_row=(unsigned long)x;
        if(!c->gradient)return fail(c,BH_RASTER_ARGUMENT);
        if(packed){if(!c->gradient->packed)return fail(c,BH_RASTER_ARGUMENT);shade_p=c->gradient->packed+c->attribute_row*256;}
        else{if(!c->gradient->indexed)return fail(c,BH_RASTER_ARGUMENT);shade_i=c->gradient->indexed+c->attribute_row*256;}
    }
    if(shaded&&((packed&&!blend)?!shade_p:!shade_i))return fail(c,BH_RASTER_ARGUMENT);
    if(blend&&(packed?!c->tables->blend_native:!c->tables->blend_indexed))return fail(c,BH_RASTER_ARGUMENT);
    a=walk(minimum,first,0,1);b=walk(minimum,first,1,1);if(!a||!b)return fail(c,BH_RASTER_TOPOLOGY);
    if(!gradient(c,a,b,a->previous,b->next,0,&top))return 0;
    a=walk(maximum,last,1,0);b=walk(maximum,last,0,0);if(!a||!b)return fail(c,BH_RASTER_TOPOLOGY);
    /* Bottom gradients use the upper vertices as their edge tips. */
    if(!gradient(c,a,b,a->next,b->previous,1,&end))return 0;
    rows=last-first;step.z=sub32(end.z,top.z)/rows;step.u=sub32(end.u,top.u)/rows;step.v=sub32(end.v,top.v)/rows;
    memset(&left,0,sizeof(left));memset(&right,0,sizeof(right));left.below=right.below=minimum;
    mask=c->texture_width-1;for(i=c->texture_width;i>1;i>>=1)++log;
    for(y=first;y<last;y++){
        if((long)left.below->sy==y){left.below=walk(left.below,y,0,1);if(!left.below)return fail(c,BH_RASTER_TOPOLOGY);left_edge(c,&left,y);}
        else{left.x=stored((double)left.x+left.dx);left.u=stored((double)left.u+left.du);left.v=stored((double)left.v+left.dv);left.z+=left.dz;}
        if((long)right.below->sy==y){right.below=walk(right.below,y,1,1);if(!right.below)return fail(c,BH_RASTER_TOPOLOGY);right_edge(&right,y);}
        else right.x=stored((double)right.x+right.dx);
        if(!_finite(left.x)||!_finite(right.x))return fail(c,BH_RASTER_NUMERICAL);
        if(right.x<left.x)return 1;
        if(!integer(c,left.x,&x)||!integer(c,(double)right.x-x,&count)||!integer(c,left.u,&u)||!integer(c,left.v,&v)||!integer(c,left.z,&z))return 0;
        if(x<0||count<0||(unsigned long)x>c->width||(unsigned long)count>c->width-(unsigned long)x)return fail(c,BH_RASTER_BOUNDS);
        row=c->pixels+y*c->pitch;depth_row=(unsigned short*)((unsigned char*)c->depth+y*c->depth_pitch);
        parity=((unsigned long)y&1)*512+((unsigned long)x&1)*256;++c->spans;
        for(i=0;i<(unsigned long)count;i++){
            ++c->visited;depth=(unsigned short)((unsigned long)z>>8);
            if(policy==2||depth_row[x]<=depth){
                texel=c->texels[((unsigned long)(u>>16)&mask)+(((unsigned long)(v>>(16-log))&(mask<<log)))];
                if(!masked||texel){
                    if(packed&&!shaded&&!blend){destination=(unsigned short*)row;destination[x]=c->image->screen[parity+texel];++c->written;if(policy!=1)depth_row[x]=depth;}
                    else{
                        mapped=c->image->dither[parity+texel];
                        if(shaded&&(!packed||blend))mapped=shade_i[mapped];
                        if(!blend){
                            if(packed)((unsigned short*)row)[x]=shade_p[mapped];else row[x]=(unsigned char)mapped;
                            ++c->written;if(policy!=1)depth_row[x]=depth;
                        }else if(mapped){
                            if(packed){
                                unsigned long color=((unsigned short*)row)[x],outer=0,j;
                                for(j=0;j<3;j++)outer+=(color&c->tables->rgb332_mask[j])<<((unsigned long)c->tables->rgb332_shift[j]&31);
                                ((unsigned short*)row)[x]=c->tables->blend_native[c->blend_plane*65536+(outer&65535)+mapped];
                            }else row[x]=c->tables->blend_indexed[c->blend_plane*65536+(unsigned long)row[x]*256+mapped];
                            ++c->written;
                        }
                    }
                }
            }
            z=add32(z,top.z);u=add32(u,top.u);v=add32(v,top.v);++x;parity^=256;
        }
        top.z=add32(top.z,step.z);top.u=add32(top.u,step.u);top.v=add32(top.v,step.v);
    }
    return 1;
}
#define KERNEL(P,N,B,M,S,L,Z) int __cdecl bh_raster_##P##_##N(BHSoftwareRaster *c,BHClipVertex *v){return run(c,v,B,M,S,L,Z);}
#define FAMILY(P,B) \
 KERNEL(P,plain_test_write,B,0,0,0,0) KERNEL(P,plain_test,B,0,0,0,1) KERNEL(P,plain_write,B,0,0,0,2) \
 KERNEL(P,masked_test_write,B,1,0,0,0) KERNEL(P,masked_test,B,1,0,0,1) KERNEL(P,masked_write,B,1,0,0,2) \
 KERNEL(P,shade_test_write,B,0,1,0,0) KERNEL(P,shade_test,B,0,1,0,1) KERNEL(P,shade_write,B,0,1,0,2) \
 KERNEL(P,masked_shade_test_write,B,1,1,0,0) KERNEL(P,masked_shade_test,B,1,1,0,1) KERNEL(P,masked_shade_write,B,1,1,0,2) \
 KERNEL(P,blend,B,1,0,1,1) KERNEL(P,shade_blend,B,1,1,1,1)
FAMILY(indexed,0)
FAMILY(packed,1)
#undef FAMILY
#undef KERNEL
