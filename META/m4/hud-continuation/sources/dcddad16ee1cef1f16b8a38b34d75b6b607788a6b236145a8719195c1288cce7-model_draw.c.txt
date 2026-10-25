#include "../src/bh_model_draw.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<18)printf("FAIL model draw %d: %s\n",line,label);}}
typedef struct Fixture {BHModel model;BHBspVertex vertices[3];BHBspEdge edges[12];BHBspFace faces[4];BHBspNode nodes[3];BHBspLeaf leaves[6];BHTextureImage image[4],*images[4];BHViewCamera view;BHClipContext clip;BHModelDrawCache cache;BHModelDrawContext draw;unsigned long count,order[32],vertices_seen;int reject;float last_u,last_z;} Fixture;
/* Isolated synchronous capture of the real producer/clipper output. Full
   native queue composition is a separate parent-owned integration target. */
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user) {
    Fixture *f=(Fixture *)user;BHClipVertex *p=ring;unsigned long n=0,id;
    for(id=0;id<4;++id)if(image==f->images[id])break;
    CHECK(id<4 && color==0x12345678UL,"explicit image/color dispatch boundary");
    do{CHECK(p->next->previous==p && p->previous->next==p,"emitted polygon reciprocal ring");CHECK(p->point.z>=f->clip.near_z && p->point.z<=f->clip.far_z,"real clipper depth output");++n;p=p->next;}while(p!=ring && n<32);
    CHECK(p==ring && n>=3 && n<32,"capture consumes complete bounded polygon synchronously");
    f->last_u=ring->u;f->last_z=ring->point.z;f->vertices_seen+=n;if(f->count<32)f->order[f->count++]=id;return !f->reject;
}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
static int fixture(Fixture *f) {
    unsigned long i,j;memset(f,0,sizeof(*f));f->model.vertex_count=3;f->model.vertices=f->vertices;f->model.edge_count=12;f->model.edges=f->edges;
    f->model.face_count=4;f->model.faces=f->faces;f->model.node_count=3;f->model.nodes=f->nodes;f->model.leaf_count=6;f->model.leaves=f->leaves;f->model.material_count=4;f->model.images=f->images;
    f->vertices[0].x=-1;f->vertices[0].y=-1;f->vertices[1].x=1;f->vertices[1].y=-1;f->vertices[2].x=0;f->vertices[2].y=1;
    for(i=0;i<3;++i){f->vertices[i].z=10;memset(f->vertices[i].opaque,0x7f,48);for(j=0;j<3;++j){f->nodes[i].minimum[j]=-2;f->nodes[i].maximum[j]=2;}f->nodes[i].front_node=f->nodes[i].back_node=-1;f->nodes[i].normal[0]=1;f->nodes[i].front_leaf=f->nodes[i].back_leaf=(long)i+1;}
    for(i=0;i<4;++i){f->images[i]=f->image+i;f->image[i].flags=1;f->faces[i].material=i;f->faces[i].first_edge=i*3;f->faces[i].runtime=0x55555555;
        for(j=0;j<3;++j){f->edges[i*3+j].vertex=j;f->edges[i*3+j].next=j==2?-1:(long)(i*3+j+1);f->edges[i*3+j].u=(float)(i*10+j);f->edges[i*3+j].v=(float)j*.25f;}}
    for(i=0;i<6;++i){f->leaves[i].face=i<4?i:0;f->leaves[i].next=-1;}
    f->leaves[0].next=4;f->leaves[4].face=1;f->leaves[4].next=5;f->leaves[5].face=0;
    f->nodes[0].front_node=1;f->nodes[0].back_node=2;f->nodes[0].front_leaf=0;f->nodes[0].back_leaf=1;
    identity(&f->view.projected);f->view.position.x=1;for(i=0;i<6;++i)f->view.planes[i].offset=100;
    if(!bh_clip_init(&f->clip,1,100,-10,10,-10,10) || !bh_model_draw_cache_init(&f->cache,&f->model,NULL))return 0;
    return bh_model_draw_context_init(&f->draw,&f->model,&f->cache,&f->view,&f->clip,1,1,0x12345678UL,capture,f);
}
static void order_cache(void) {
    Fixture f;BHBspVertex raw[3];BHBspFace faces[4];unsigned long i,epoch;
    CHECK(fixture(&f),"typed model cache owns valid working records");memcpy(raw,f.vertices,sizeof(raw));memcpy(faces,f.faces,sizeof(faces));
    CHECK(bh_model_draw_unlit(&f.draw,0,63) && f.count==4 && f.order[0]==3 && f.order[1]==0 && f.order[2]==1 && f.order[3]==2,"positive eye recurse back, front leaf chain, tail front; duplicate face suppressed");
    CHECK(f.draw.faces_marked==4 && f.draw.vertices_transformed==3 && f.draw.polygons_emitted==4,"shared vertices transformed once and all unique faces marked per frame");
    CHECK(f.last_u==22 && f.cache.vertices[2].next==f.cache.vertices && f.cache.vertices[0].previous==f.cache.vertices+2,"original edge UV overwrite and ring starts at last edge node");
    CHECK(!memcmp(raw,f.vertices,sizeof(raw)) && !memcmp(faces,f.faces,sizeof(faces)),"serialized original cache/pointer bytes remain immutable in typed source adapter");
    f.count=0;CHECK(bh_model_draw_unlit(&f.draw,0,0) && !f.count,"same-frame face cache suppresses entire second traversal emission");
    bh_model_draw_cache_reset(&f.cache);f.draw.frame=2;f.view.position.x=-1;f.count=0;
    CHECK(bh_model_draw_unlit(&f.draw,0,0) && f.count==3 && f.order[0]==2 && f.order[1]==1 && f.order[2]==3,"negative eye reverses child order and selects back leaf chain");
    bh_model_draw_cache_reset(&f.cache);f.draw.frame=3;f.count=0;f.image[0].flags=0;
    CHECK(bh_model_draw_face(&f.draw,0) && !f.count && f.cache.face_frames[0]==3,"face marked before texture filter rejection");f.image[0].flags=1;
    CHECK(bh_model_draw_face(&f.draw,0) && !f.count,"same-frame filter change does not undo earlier face mark");
    f.draw.frame=4;f.images[0]=NULL;CHECK(bh_model_draw_face(&f.draw,0) && !f.count && f.cache.face_frames[0]==4,"null image face marks frame without emission");f.images[0]=f.image;
    CHECK(bh_model_draw_face(&f.draw,0) && !f.count,"null-image mark also suppresses same-frame retry");
    bh_model_draw_cache_reset(&f.cache);f.draw.frame=5;f.reject=1;CHECK(!bh_model_draw_face(&f.draw,0) && f.draw.error==BH_MODEL_DRAW_EMIT,"isolated callback failure propagates explicit adapter error");
    f.reject=0;f.draw.frame=6;f.draw.error=BH_MODEL_DRAW_OK;f.count=0;f.view.projected.m[0][3]=1000;
    CHECK(bh_model_draw_face(&f.draw,0) && !f.count && f.clip.error==BH_CLIP_OK,"real source clipper completely rejects transformed out-of-viewport face");
    f.draw.frame=7;identity(&f.view.projected);f.image[0].unused54=1;{float terms[6]={2,0,1,0,3,-1};memcpy(f.image[0].padding,terms,sizeof(terms));}
    bh_clip_set_camera(&f.clip,&f.view.projected);f.count=0;CHECK(bh_model_draw_face(&f.draw,0) && f.count==1 && f.last_u==1,"face owner invokes real projected-UV image branch and general-inverse dependency");
    CHECK(bh_model_draw_leaf_chain(&f.draw,-1),"negative leaf chain is original empty list");
    f.image[0].unused54=0;
    for(i=0;i<3;++i){epoch=i==0?0x01000000UL:i==1?0x01000001UL:0xffffffffUL;f.draw.frame=epoch;f.count=0;
        CHECK(bh_model_draw_face(&f.draw,0) && f.count==1 && f.cache.face_frames[0]==epoch,"raw32bit face epoch survives float precision boundary and allones token");
        memcpy(&epoch,&f.cache.vertices[0].frame,4);CHECK(epoch==f.draw.frame,"vertex epoch stores original integer bit pattern in opaque float-declared slot");
        CHECK(bh_model_draw_face(&f.draw,0) && f.count==1,"same raw32bit epoch suppresses repeated face");
    }
    for(i=0;i<3;++i)CHECK(!memcmp(raw+i,f.vertices+i,sizeof(raw[i])),"all original vertex storage preserved after cache reuse");
    bh_model_draw_cache_dispose(&f.cache);CHECK(!f.cache.vertices && !f.cache.face_frames && !f.cache.tasks,"cache owner releases all working allocations");
}
/* Independent visibility oracle enumerates all eight box corners. */
static int classify(const BHBspNode *n,const BHViewCamera *view,unsigned long *mask) {
    unsigned long plane,bit,corner,k;double minimum,maximum,d,p[3];
    for(plane=0,bit=1;plane<6;++plane,bit*=2)if(*mask&bit){minimum=1e100;maximum=-1e100;
        for(corner=0;corner<8;++corner){for(k=0;k<3;++k)p[k]=(corner&(1<<k))?n->maximum[k]:n->minimum[k];
            d=p[0]*view->planes[plane].normal.x+p[1]*view->planes[plane].normal.y+p[2]*view->planes[plane].normal.z-view->planes[plane].offset;
            if(d<minimum)minimum=d;if(d>maximum)maximum=d;}
        if(minimum>=0)return 0;if(maximum<0)*mask&=(0xffffUL^bit);
    }return 1;
}
static void oracle(Fixture *f,long node,unsigned long mask,unsigned long *seen,unsigned long *order,unsigned long *count) {
    const BHBspNode *n;long leaf;double side;if(node<0)return;n=f->nodes+node;if(!classify(n,&f->view,&mask))return;
    side=f->view.position.x*n->normal[0]+f->view.position.y*n->normal[1]+f->view.position.z*n->normal[2]+n->plane;
    oracle(f,side<0?n->front_node:n->back_node,mask,seen,order,count);leaf=side<0?n->back_leaf:n->front_leaf;
    while(leaf>=0){if(!seen[f->leaves[leaf].face]){seen[f->leaves[leaf].face]=1;order[(*count)++]=f->leaves[leaf].face;}leaf=f->leaves[leaf].next;}
    oracle(f,side<0?n->back_node:n->front_node,mask,seen,order,count);
}
static void visibility(void) {
    Fixture f;unsigned long n,i,j,mask,seen[4],order[4],count;float nx,ny,nz;
    CHECK(fixture(&f),"visibility fixture cache");
    for(n=0;n<80;++n){f.view.position.x=(n&1)?-1:1;f.view.position.y=.5f;f.view.position.z=2;
        for(i=0;i<6;++i){nx=(n&(1UL<<(i%3)))?-1.0f:1.0f;ny=(n&(1UL<<((i+1)%4)))?-.5f:.5f;nz=(n&(1UL<<((i+2)%5)))?-2.0f:2.0f;
            f.view.planes[i].normal.x=nx;f.view.planes[i].normal.y=ny;f.view.planes[i].normal.z=nz;f.view.planes[i].offset=(float)((int)((n+i)%11)-5);}
        for(mask=0;mask<64;++mask){bh_model_draw_cache_reset(&f.cache);f.count=0;f.draw.frame=n*64+mask+1;memset(seen,0,sizeof(seen));count=0;
            oracle(&f,0,mask,seen,order,&count);CHECK(bh_model_draw_unlit(&f.draw,0,mask),"complete traversal mask fixture succeeds");
            CHECK(f.count==count,"independent corner visibility oracle face count");for(j=0;j<count && j<f.count;++j)CHECK(f.order[j]==order[j],"independent BSP visibility/order oracle");}
    }
    f.nodes[0].front_node=f.nodes[0].back_node=-1;f.nodes[0].front_leaf=f.nodes[0].back_leaf=1;
    memset(f.view.planes,0,sizeof(f.view.planes));f.view.planes[0].normal.x=1;f.view.planes[0].offset=-2;
    bh_model_draw_cache_reset(&f.cache);f.count=0;f.draw.frame=10000;
    CHECK(bh_model_draw_unlit(&f.draw,0,1) && !f.count,"AABB minimum-support tangent is rejected at exact equality");
    f.view.planes[0].offset=3;f.draw.frame=10001;CHECK(bh_model_draw_unlit(&f.draw,0,0x80000001UL) && f.cache.tasks[0].mask==0,"wholly inside AABB uses exact16bit AND and truncates high mask bits");
    f.nodes[0].minimum[0]=f.nodes[0].maximum[0]=(float)ldexp(1.0,60);f.nodes[0].minimum[1]=f.nodes[0].maximum[1]=(float)ldexp(1.0,60);f.nodes[0].minimum[2]=f.nodes[0].maximum[2]=1;
    f.view.planes[0].normal.x=-1;f.view.planes[0].normal.y=1;f.view.planes[0].normal.z=1;f.view.planes[0].offset=.5f;
    bh_model_draw_cache_reset(&f.cache);f.count=0;f.draw.frame=10002;
    CHECK(bh_model_draw_unlit(&f.draw,0,1) && !f.count,"sign-tree exceptional Z+(X+Y) grouping retains small residual and rejects box");
    f.view.planes[0].normal.x=1;f.view.planes[0].normal.y=-1;f.draw.frame=10003;
    CHECK(bh_model_draw_unlit(&f.draw,0,1) && f.count==1,"positive control ordinary X+(Y+Z) grouping loses small residual before comparison");
    bh_model_draw_cache_dispose(&f.cache);
}
int bh_model_draw_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    order_cache();visibility();CHECK(bh_math_environment_supported(),"model draw preserves caller PC53 profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"model-draw\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_MODEL_DRAW_STANDALONE
int main(void){unsigned long count;return bh_model_draw_checkpoint(&count);}
#endif
