#include "../src/bh_model_draw_lit.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<18)printf("FAIL model draw lit %d: %s\n",line,label);}}
typedef struct LitFixture {BHModel model;BHBspVertex vertices[3];BHBspEdge edges[9];BHBspFace faces[3];BHBspNode nodes[3];BHBspLeaf leaves[3];BHTextureImage images[3],*image[3];BHViewCamera view;BHClipContext clip;BHModelDrawCache cache;BHModelDrawContext draw;BHModelDrawLitContext lit;BHLightConfig directional;BHLightContext lights;BHLight ambient;BHMat34 inverse;unsigned long count,order[8],color[8];} LitFixture;
/* Isolated capture only; root supplies real native emitter in composition. */
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user) {
    LitFixture *f=(LitFixture *)user;BHClipVertex *p=ring;unsigned long i,n=0;
    for(i=0;i<3;++i)if(image==f->images+i)break;CHECK(i<3,"real face owner supplies original image binding");
    do{CHECK(p->next->previous==p && p->previous->next==p,"lit output from real clipper has reciprocal ring");++n;p=p->next;}while(p!=ring && n<20);
    CHECK(n==3 && p==ring,"lit face complete source-built ring");if(f->count<8){f->order[f->count]=i;f->color[f->count++]=color;}return 1;
}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
static int fixture(LitFixture *f) {
    unsigned long i,j;memset(f,0,sizeof(*f));f->model.vertex_count=3;f->model.vertices=f->vertices;f->model.edge_count=9;f->model.edges=f->edges;f->model.face_count=3;f->model.faces=f->faces;
    f->model.node_count=3;f->model.nodes=f->nodes;f->model.leaf_count=3;f->model.leaves=f->leaves;f->model.material_count=3;f->model.images=f->image;
    f->vertices[0].x=-1;f->vertices[0].y=-1;f->vertices[1].x=1;f->vertices[1].y=-1;f->vertices[2].y=1;
    for(i=0;i<3;++i){f->vertices[i].z=10;f->image[i]=f->images+i;f->images[i].flags=1;f->faces[i].material=i;f->faces[i].first_edge=i*3;f->leaves[i].face=i;f->leaves[i].next=-1;
        f->nodes[i].front_leaf=f->nodes[i].back_leaf=i;f->nodes[i].front_node=f->nodes[i].back_node=-1;f->nodes[i].normal[i]=1;
        for(j=0;j<3;++j){f->nodes[i].minimum[j]=(float)((int)i-1);f->nodes[i].maximum[j]=(float)((int)i+1);f->edges[i*3+j].vertex=j;f->edges[i*3+j].next=j==2?-1:(long)(i*3+j+1);f->edges[i*3+j].u=(float)j*.5f;f->edges[i*3+j].v=(float)j*.25f;}}
    f->nodes[0].front_node=1;f->nodes[0].back_node=2;identity(&f->view.projected);identity(&f->inverse);identity(&f->lights.world_to_model);
    f->view.position.x=f->view.position.y=f->view.position.z=2;for(i=0;i<6;++i)f->view.planes[i].offset=100;
    f->ambient.type=f->ambient.prepared_type=1;f->ambient.color=0x123456;f->ambient.intensity=.5f;f->ambient.effective_intensity=.5f;f->lights.lights=&f->ambient;
    bh_light_configure(&f->directional,1,-1,-2,-3,.5f,.125f,0x204080,0x102030);
    if(!bh_light_prepare(&f->lights,&f->view.position,3) || !bh_clip_init(&f->clip,1,100,-10,10,-10,10) || !bh_model_draw_cache_init(&f->cache,&f->model,NULL))return 0;
    if(!bh_model_draw_context_init(&f->draw,&f->model,&f->cache,&f->view,&f->clip,1,1,0,capture,f))return 0;
    return bh_model_draw_lit_context_init(&f->lit,&f->draw,&f->lights,&f->directional,&f->inverse,1,1,0xff000000UL);
}
static unsigned long clamp(double x){long n=(long)x;return n<0?0:n>255?255:(unsigned long)n;}
static unsigned long color_oracle(const LitFixture *f,const BHBspNode *n) {
    unsigned long r=0,g=0,b=0;double dot,side,dynamic;
    if(f->lit.point_enabled){dynamic=f->ambient.effective_intensity*f->lit.scale;r=clamp(((f->ambient.color>>16)&255)*dynamic);g=clamp(((f->ambient.color>>8)&255)*dynamic);b=clamp((f->ambient.color&255)*dynamic);}
    if(f->directional.enabled){side=n->normal[0]*f->view.position.x+n->normal[1]*f->view.position.y+n->normal[2]*f->view.position.z+n->plane;
        dot=(n->normal[0]*f->lit.local_direction.x+n->normal[1]*f->lit.local_direction.y+n->normal[2]*f->lit.local_direction.z)*f->lit.scale;
        if(side<0)dot=-dot;if(dot<0)dot=0;
        r=clamp(dot*f->directional.directional[0]+r+f->directional.ambient[0]);g=clamp(dot*f->directional.directional[1]+g+f->directional.ambient[1]);b=clamp(dot*f->directional.directional[2]+b+f->directional.ambient[2]);}
    return f->lit.base_color+((r<<16)|(g<<8)|b);
}
static int visible(const BHBspNode *n,const BHViewCamera *view,unsigned long *mask) {
    unsigned long slot,bit,corner,k;double minimum,maximum,p[3],d;
    for(slot=0,bit=1;slot<6;++slot,bit*=2)if(*mask&bit){minimum=1e100;maximum=-1e100;for(corner=0;corner<8;++corner){for(k=0;k<3;++k)p[k]=(corner&(1<<k))?n->maximum[k]:n->minimum[k];d=p[0]*view->planes[slot].normal.x+p[1]*view->planes[slot].normal.y+p[2]*view->planes[slot].normal.z-view->planes[slot].offset;if(d<minimum)minimum=d;if(d>maximum)maximum=d;}
        if(minimum>=0)return 0;if(maximum<0)*mask&=(0xffffUL^bit);}return 1;
}
static void oracle(LitFixture *f,long node,unsigned long mask,unsigned long *order,unsigned long *colors,unsigned long *count) {
    const BHBspNode *n;double side;if(node<0)return;n=f->nodes+node;if(!visible(n,&f->view,&mask))return;
    side=n->normal[0]*f->view.position.x+n->normal[1]*f->view.position.y+n->normal[2]*f->view.position.z+n->plane;
    oracle(f,side<0?n->front_node:n->back_node,mask,order,colors,count);order[*count]=node;colors[(*count)++]=color_oracle(f,n);
    oracle(f,side<0?n->back_node:n->front_node,mask,order,colors,count);
}
static void varied(void) {
    LitFixture f;unsigned long n,mask,i,j,order[3],colors[3],count;float nx,ny,nz;
    CHECK(fixture(&f),"lit fixture links actual config/preparation/evaluation and face/clip owners");
    for(n=0;n<64;++n){f.view.position.x=(n&1)?-2:2;f.view.position.y=(n&2)?-2:2;f.view.position.z=(n&4)?-2:2;
        f.directional.enabled=(n&8)!=0;f.lit.point_enabled=(n&16)!=0;f.lit.scale=(n&32)?1.5f:.5f;
        for(i=0;i<6;++i){nx=(n&(1UL<<(i%3)))?-1.0f:1.0f;ny=(n&(1UL<<((i+1)%4)))?-.5f:.5f;nz=(n&(1UL<<((i+2)%5)))?-2.0f:2.0f;
            f.view.planes[i].normal.x=nx;f.view.planes[i].normal.y=ny;f.view.planes[i].normal.z=nz;f.view.planes[i].offset=(float)((int)((n+i)%11)-5);}
        for(mask=0;mask<64;++mask){bh_model_draw_cache_reset(&f.cache);f.draw.frame=n*64+mask+1;f.count=0;count=0;oracle(&f,0,mask,order,colors,&count);
            CHECK(bh_model_draw_lit(&f.lit,0,mask),"complete lit traversal/all-mask source fixture succeeds");CHECK(f.count==count,"independent corner/BSP oracle lit emission count");
            for(j=0;j<count && j<f.count;++j){CHECK(f.order[j]==order[j],"lit painter/leaf order remains original BSP order");CHECK(f.color[j]==colors[j],"independent directional/ambient/dynamic byte color oracle");}}
    }bh_model_draw_cache_dispose(&f.cache);
}
static void controls(void) {
    LitFixture f;BHLight point;BHVec3 reference;
    CHECK(fixture(&f),"isolated lighting controls fixture");f.nodes[0].front_node=f.nodes[0].back_node=-1;f.nodes[0].normal[0]=f.nodes[0].normal[1]=0;f.nodes[0].normal[2]=1;
    f.view.position.z=-2;f.count=0;CHECK(bh_model_draw_lit(&f.lit,0,0) && f.count==1 && f.color[0]==color_oracle(&f,f.nodes),"negative eye side negates directional dot");
    bh_model_draw_cache_reset(&f.cache);f.draw.frame=2;f.view.position.z=2;f.count=0;CHECK(bh_model_draw_lit(&f.lit,0,0) && f.color[0]==color_oracle(&f,f.nodes),"nonnegative eye side keeps directional dot before clamp");
    f.directional.directional[0]=1000;f.directional.directional[1]=-1000;f.directional.directional[2]=0;f.directional.ambient[0]=0;f.directional.ambient[1]=0;f.directional.ambient[2]=12.99f;f.view.position.z=-2;f.draw.frame=3;f.count=0;
    CHECK(bh_model_draw_lit(&f.lit,0,0) && f.color[0]==color_oracle(&f,f.nodes),"actual signed truncation then low/high channel clamps");
    f.directional.enabled=0;f.lit.point_enabled=0;f.lit.base_color=0xfffffff0UL;f.draw.frame=4;f.count=0;CHECK(bh_model_draw_lit(&f.lit,0,0) && f.color[0]==0xfffffff0UL,"both lighting modes disabled preserve additive base color exactly");
    f.lit.point_enabled=1;f.ambient.color=0x102030;f.ambient.effective_intensity=1;f.draw.frame=5;f.count=0;
    CHECK(bh_model_draw_lit(&f.lit,0,0) && f.color[0]==0x00102020UL,"base color addition wraps32bits without replacing alpha or OR packing");
    memset(&point,0,sizeof(point));point.type=3;point.color=0x804020;point.intensity=.5f;point.range=0;point.position.x=0;point.position.y=0;point.position.z=8;f.lights.lights=&point;reference.x=reference.y=reference.z=0;
    CHECK(bh_light_prepare(&f.lights,&reference,1),"real model-local point light preparation");f.lit.base_color=0;f.draw.frame=6;f.count=0;f.view.position.z=2;
    CHECK(bh_model_draw_lit(&f.lit,0,0) && f.color[0]==0x402010,"point light evaluated at AABB midpoint with raw normal on front side");
    f.view.position.z=-2;f.draw.frame=7;f.count=0;CHECK(bh_model_draw_lit(&f.lit,0,0) && f.color[0]==0x402010,"back side keeps SAME raw node normal for prepared point evaluation");
    f.nodes[0].minimum[0]=2;f.nodes[0].maximum[0]=4;point.position.x=3;f.draw.frame=8;f.count=0;CHECK(bh_light_prepare(&f.lights,&reference,2) && bh_model_draw_lit(&f.lit,0,0) && f.color[0]==0x402010,"point sampling follows changed binary32 AABB midpoint rather than eye or first vertex");
    f.nodes[0].front_leaf=f.nodes[0].back_leaf=-1;f.draw.frame=9;f.count=0;f.draw.color=0xdeadbeef;
    CHECK(bh_model_draw_lit(&f.lit,0,0) && !f.count && f.draw.color==0x402010,"node shade updates global color even for absent leaf");
    f.nodes[0].front_leaf=f.nodes[0].back_leaf=0;f.directional.enabled=1;f.lit.point_enabled=0;f.directional.ambient[0]=1e30f;f.draw.frame=10;
    CHECK(!bh_model_draw_lit(&f.lit,0,0) && f.lit.error==BH_MODEL_DRAW_LIT_NUMERICAL,"explicit out-of-range signed conversion guard is not backend success");
    f.directional.ambient[0]=0;f.lit.scale=1;f.directional.direction.x=1;f.directional.direction.y=2;f.directional.direction.z=3;identity(&f.inverse);f.inverse.m[0][0]=0;f.inverse.m[0][1]=-1;f.inverse.m[1][0]=1;f.inverse.m[1][1]=0;
    CHECK(bh_model_draw_lit_context_init(&f.lit,&f.draw,&f.lights,&f.directional,&f.inverse,0,1,0) && f.lit.local_direction.x==-2 && f.lit.local_direction.y==1 && f.lit.local_direction.z==3,"real direction owner binds model rotation without normalizing nonunit input");
    f.directional.enabled=0;f.lit.point_enabled=0;f.nodes[0].minimum[0]=f.nodes[0].maximum[0]=(float)ldexp(1.0,60);f.nodes[0].minimum[1]=f.nodes[0].maximum[1]=1;f.nodes[0].minimum[2]=f.nodes[0].maximum[2]=(float)ldexp(1.0,60);
    memset(f.view.planes,0,sizeof(f.view.planes));f.view.planes[0].normal.x=-1;f.view.planes[0].normal.y=1;f.view.planes[0].normal.z=1;f.view.planes[0].offset=.5f;f.draw.frame=11;f.count=0;
    CHECK(bh_model_draw_lit(&f.lit,0,1) && !f.count,"lit defaultY+(X+Z) retains unit residual and rejects cancellation fixture");
    CHECK(bh_model_draw_unlit(&f.draw,0,1) && f.count==1,"real frozen unlit positive control groupsZ+(X+Y) and accepts same cancellation fixture");
    bh_model_draw_cache_dispose(&f.cache);
}
static void menu_sign(void){
    LitFixture f;unsigned long i;static const float eye[3]={-2,2,0};
    static const unsigned long expected[3]={0xffe6ffe0UL,0xff663340UL,0xff663340UL};
    CHECK(fixture(&f),"menu lighting sign fixture uses real face/clip owners");
    f.nodes[0].front_node=f.nodes[0].back_node=-1;
    f.nodes[0].normal[0]=f.nodes[0].normal[1]=0;f.nodes[0].normal[2]=1;
    f.lit.point_enabled=0;f.lit.local_direction.x=f.lit.local_direction.y=0;f.lit.local_direction.z=-1;
    bh_light_configure(&f.directional,1,0,0,1,1,.4f,0x80ffa0,0xff80a0);
    /* Literal channel results from original negative-side FCHS and clamp;
       independent of the traversal oracle above, with equality control. */
    for(i=0;i<3;++i){bh_model_draw_cache_reset(&f.cache);f.draw.frame=i+1;f.view.position.z=eye[i];f.count=0;
        CHECK(bh_model_draw_lit(&f.lit,0,0)&&f.count==1&&f.color[0]==expected[i],"original menu light colors respect negative/nonnegative/zero side");}
    bh_model_draw_cache_dispose(&f.cache);
}
int bh_model_draw_lit_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    varied();controls();menu_sign();CHECK(bh_math_environment_supported(),"lit model draw preserves caller PC53 profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"model-draw-lit\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_MODEL_DRAW_LIT_STANDALONE
int main(void){unsigned long count;return bh_model_draw_lit_checkpoint(&count);}
#endif
