#include "../src/bh_model_instance.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) check((x)!=0,s,__LINE__)
static void check(int ok,const char *s,int line){++checks;if(!ok){++failures;if(failures<20)printf("FAIL model instance %d: %s\n",line,s);}}
typedef struct Fixture {
 BHModel model;BHBspVertex vertices[3];BHBspEdge edges[3];BHBspFace face;BHBspNode node;BHBspLeaf leaf;
 BHTextureImage image,*images[1];BHModelDrawCache cache;BHMainCamera camera;BHViewConfig config;BHClipContext clip;
 BHLightConfig directional;BHLightContext lights;BHLight ambient;BHMat34 identity;BHVec3 reference;
 BHModelInstanceContext instance;unsigned long emitted,color;int reject;
} Fixture;
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user){
 Fixture *f=(Fixture *)user;BHClipVertex *p=ring;unsigned long count=0;
 CHECK(image==&f->image,"real model face image reaches emitter");
 do{CHECK(p->next->previous==p && p->previous->next==p,"real clipper supplies reciprocal output");CHECK(p->point.z>0,"positive projected depth");p=p->next;++count;}while(p!=ring && count<16);
 CHECK(count==3 && p==ring,"whole triangle reaches source emitter");++f->emitted;f->color=color;return !f->reject;
}
static int fixture(Fixture *f){unsigned long i;memset(f,0,sizeof(*f));identity(&f->identity);
 f->model.radius=2;f->model.vertices=f->vertices;f->model.vertex_count=3;f->model.edges=f->edges;f->model.edge_count=3;
 f->model.faces=&f->face;f->model.face_count=1;f->model.nodes=&f->node;f->model.node_count=1;f->model.leaves=&f->leaf;f->model.leaf_count=1;
 f->model.images=f->images;f->model.material_count=1;f->images[0]=&f->image;f->image.flags=1;
 f->vertices[0].x=-1;f->vertices[0].y=-1;f->vertices[1].x=1;f->vertices[1].y=-1;f->vertices[2].y=1;
 for(i=0;i<3;++i){f->edges[i].vertex=i;f->edges[i].next=i==2?-1:(long)i+1;f->node.minimum[i]=-1;f->node.maximum[i]=1;}
 f->node.normal[2]=1;f->node.front_node=f->node.back_node=-1;f->node.front_leaf=f->node.back_leaf=0;f->leaf.next=-1;
 if(!bh_view_config_init(&f->config,640,480) || !bh_main_camera_settings(&f->config,60,1,100) ||
 !bh_main_camera_pose(&f->config,&f->camera,0,0,0,0,0,0) || !bh_clip_init(&f->clip,1,100,f->config.left,f->config.right,f->config.top,f->config.bottom) ||
 !bh_model_draw_cache_init(&f->cache,&f->model,NULL))return 0;
 f->instance.model=&f->model;f->instance.cache=&f->cache;f->instance.camera=&f->camera;f->instance.config=&f->config;f->instance.clip=&f->clip;
 f->instance.light_config=&f->directional;f->instance.lights=&f->lights;f->instance.light_reference=&f->reference;
 f->instance.emit=capture;f->instance.emit_user=f;f->instance.texture_mask=1;f->instance.base_color=0xff000000UL;return 1;
}
static void culling(void){Fixture f;BHMat34 before,saved_camera;BHLightContext saved_lights;unsigned long frame;int side;double delta=ldexp(1.0,-30);
 CHECK(fixture(&f),"instance links real camera and model cache");memset(&f.model.transform,0x35,sizeof(f.model.transform));before=f.model.transform;frame=f.instance.frame;
 f.instance.color=0x12345678;f.instance.opacity=.25f;memset(&f.instance.last_camera_view,0x27,sizeof(BHMat34));saved_camera=f.instance.last_camera_view;saved_lights=f.lights;
 CHECK(bh_model_instance_draw(&f.instance,0,0,-1-delta,0,0,0,1) && !f.instance.drawn,"near rejection keeps original double precision");
 CHECK(!memcmp(&before,&f.model.transform,sizeof(before)) && f.instance.frame==frame,"culled model transform and frame retained");
 CHECK(f.instance.color==0x12345678 && f.instance.opacity==.25f && !memcmp(&saved_camera,&f.instance.last_camera_view,sizeof(saved_camera)) && !memcmp(&saved_lights,&f.lights,sizeof(saved_lights)),"culling retains color/scalar/camera/light state");
 CHECK(bh_model_instance_draw(&f.instance,0,0,-1,0,0,0,1) && f.instance.drawn,"near sphere equality accepted");
 CHECK(bh_model_instance_draw(&f.instance,0,0,102,0,0,0,1) && f.instance.drawn,"far sphere equality accepted");
 frame=f.instance.frame;CHECK(bh_model_instance_draw(&f.instance,0,0,102+delta,0,0,0,1) && !f.instance.drawn && frame==f.instance.frame,"far double precision rejection");
 /* Independent axis planes isolate all four side comparisons. */
 memset(f.camera.planes,0,sizeof(f.camera.planes));f.camera.planes[0].normal.z=1;
 for(side=2;side<6;++side){f.camera.planes[side].normal.x=1;
 CHECK(bh_model_instance_draw(&f.instance,2,0,10,0,0,0,1) && f.instance.drawn,"side sphere tangent accepted");
 frame=f.instance.frame;CHECK(bh_model_instance_draw(&f.instance,2+delta,0,10,0,0,0,1) && !f.instance.drawn && frame==f.instance.frame,"side rejection uses double position");
 f.camera.planes[side].normal.x=0;}
 bh_model_draw_cache_dispose(&f.cache);
}
static void drawing(void){Fixture f;unsigned long i,expected;float alpha[]={0,1,.5f,-.5f,1.25f};double a=.3,b=-.4,c=.2,cy=cos(a),sy=sin(a),cx=cos(b),sx=sin(b),cz=cos(c),sz=sin(c);double oracle[3][3];
 CHECK(fixture(&f),"emission fixture");
 for(i=0;i<5;++i){expected=f.instance.base_color-(unsigned long)(long)((double)alpha[i]*-255.0)*0x10101UL;
 CHECK(bh_model_instance_draw(&f.instance,0,0,10,0,0,0,alpha[i]),"unlit instance traverses real BSP and clipper");
 CHECK(f.instance.drawn && f.instance.frame==i+1 && f.emitted==i+1,"per visible instance frame epoch");
 CHECK(f.color==expected && f.instance.opacity==alpha[i],"signed grayscale truncation and raw scalar retention");
 CHECK(!memcmp(&f.camera.view,&f.instance.last_camera_view,sizeof(BHMat34)),"world camera copied after traversal");}
 CHECK(bh_model_instance_draw(&f.instance,1,2,10,a,b,c,1),"three-axis instance transform");
 oracle[0][0]=cy*cz+sy*sx*sz;oracle[0][1]=-cy*sz+sy*sx*cz;oracle[0][2]=sy*cx;
 oracle[1][0]=cx*sz;oracle[1][1]=cx*cz;oracle[1][2]=-sx;
 oracle[2][0]=-sy*cz+cy*sx*sz;oracle[2][1]=sy*sz+cy*sx*cz;oracle[2][2]=cy*cx;
 for(i=0;i<9;++i)CHECK(fabs(f.model.transform.m[i/3][i%3]-oracle[i/3][i%3])<2e-6,"independent Ry Rx Rz oracle");
 CHECK(f.model.transform.m[0][3]==1 && f.model.transform.m[1][3]==2 && f.model.transform.m[2][3]==10,"local rotation preserves translated origin");
 f.directional.enabled=1;f.directional.ambient[0]=32;f.directional.ambient[1]=64;f.directional.ambient[2]=96;
 CHECK(bh_model_instance_draw(&f.instance,0,0,10,0,0,0,.5f) && f.color==0xff204060UL,"directional flag selects complete lit owner/base color");
 f.directional.enabled=0;f.instance.point_enabled=1;f.ambient.type=f.ambient.prepared_type=1;f.ambient.color=0x804020;f.ambient.intensity=1;f.ambient.effective_intensity=1;f.lights.lights=&f.ambient;
 CHECK(bh_model_instance_draw(&f.instance,0,0,10,0,0,0,.5f) && f.color==0xff402010UL,"point flag uses real prepare/evaluate with final scalar");
 f.instance.point_enabled=0;f.reject=1;
 CHECK(!bh_model_instance_draw(&f.instance,0,0,10,0,0,0,1) && f.instance.error==BH_MODEL_INSTANCE_DRAW,"emitter failure propagates without claiming draw success");
 bh_model_draw_cache_dispose(&f.cache);
}
int bh_model_instance_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
 culling();drawing();CHECK(bh_math_environment_supported(),"instance preserves PC53 profile");bh_math_environment_leave(&fp);*count=checks;
 printf("{\"scenario\":\"model-instance\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;}
