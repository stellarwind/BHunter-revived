#include "../src/bh_sprite_quad.h"
#include "../src/bh_fp.h"
#include "../src/bh_menu_present.h"
#include "sprite_quad_cases.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) check((x)!=0,s,__LINE__)
static void check(int ok,const char *s,int line){++checks;if(!ok){++failures;if(failures<20)printf("FAIL sprite quad %d: %s\n",line,s);}}
static int close_to(double a,double b){return fabs(a-b)<4e-5*(1+fabs(b));}
typedef struct Fixture {
    BHSpriteQuadContext draw;BHMainCamera camera;BHClipContext clip;
    BHLightConfig directional;BHLightContext lights;BHLight light;BHTextureImage image;
    BHClipVertex observed[16];unsigned long count,calls,color;int reject;
} Fixture;
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user) {
    Fixture *f=(Fixture *)user;BHClipVertex *p=ring;unsigned long n=0;
    CHECK(image==&f->image,"image identity retained");
    do{CHECK(p->next->previous==p && p->previous->next==p,"reciprocal clipped winding");if(n<16)f->observed[n]=*p;++n;p=p->next;}while(p!=ring && n<16);
    CHECK(p==ring && n>=3 && n<=16,"bounded convex result");f->count=n;f->color=color;++f->calls;return !f->reject;
}
static void setup(Fixture *f){memset(f,0,sizeof(*f));f->camera.projected.m[0][0]=f->camera.projected.m[1][1]=f->camera.projected.m[2][2]=1;
    bh_clip_init(&f->clip,1,100,-1,1,-1,1);bh_clip_set_camera(&f->clip,&f->camera.projected);
    f->draw.camera=&f->camera;f->draw.clip=&f->clip;f->draw.directional=&f->directional;f->draw.world_lights=&f->lights;
    f->draw.emit=capture;f->draw.emit_user=f;f->draw.base_color=0xff000000UL;
}
static int draw(Fixture *f,const BHVec3 *center,const BHVec3 *normal,const BHVec3 *up,float w,float h,float angle,unsigned char flags,float modulation){
    return bh_sprite_quad_draw(&f->draw,&f->image,center,normal,up,w,h,angle,flags,modulation,.125f,.5f,.25f,.375f);
}
static void geometry(void){Fixture f;BHVec3 p={0,0,4},n={0,0,-1},up={0,1,0};unsigned long flags,i;
    const double x[4]={1,1,-1,-1},y[4]={1,-1,-1,1};
    for(flags=0;flags<16;++flags){setup(&f);CHECK(draw(&f,&p,&n,&up,2,2,0,(unsigned char)flags,1),"all UV/front flags draw");
        CHECK(f.count==4 && f.calls==1 && f.color==0xffffffffUL && f.draw.emitted==1,"one white quad");
        for(i=0;i<4;++i){double u=i>=2?.625:.125,v=i==1 || i==2?.625:.25,a;
            if(flags&1)u=1-u;if(flags&2)v=1-v;if(flags&4){a=u;u=v;v=a;}
            CHECK(close_to(f.observed[i].local.x,x[i]) && close_to(f.observed[i].local.y,y[i]) && f.observed[i].local.z==4,"independent world-plane corners");
            CHECK(close_to(f.observed[i].sx,x[i]/4) && close_to(f.observed[i].sy,y[i]/4) && f.observed[i].point.z==4,"real clipper projection");
            CHECK(close_to(f.observed[i].u,u) && close_to(f.observed[i].v,v),"independent UV flip/transposition table");
        }
    }
    setup(&f);n.z=1;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && !f.calls && !f.draw.emitted,"strict backface rejection");
    CHECK(draw(&f,&p,&n,&up,2,2,0,8,.5f) && f.calls==1 && f.color==0xff7f7f7fUL,"backface flag admits and gray conversion truncates");
    CHECK(f.observed[0].local.x==-1 && f.observed[1].local.x==1 && f.observed[1].local.y==1,"backface reverses ring starting at same corner");
    n.z=-1;CHECK(draw(&f,&p,&n,&up,2,4,1.5707963267948966f,0,1),"quarter-turn basis rotation");
    CHECK(close_to(f.observed[0].local.x,-2) && close_to(f.observed[0].local.y,1) && close_to(f.observed[1].local.x,2) && close_to(f.observed[1].local.y,1),"analytical rotated rectangle");
}
static BHTextureImage *__cdecl menu_image(unsigned long handle,void *user){return handle==17?&((Fixture*)user)->image:NULL;}
static void menu_label(void){Fixture f;BHDrawQueue q;BHMenuPresent menu;BHMenuPresentRow row;BHMenuRow frame;
    BHVec3 eye={0,0,0},angles={0,0,0};BHQuadQueueContext consumer;unsigned long next;
    setup(&f);memset(&q,0,sizeof(q));memset(&menu,0,sizeof(menu));memset(&row,0,sizeof(row));memset(&frame,0,sizeof(frame));
    CHECK(bh_draw_queue_init(&q,32,NULL),"menu label real queue");
    row.row=&frame;row.image_handle=17;menu.queue=&q;menu.eye=&eye;menu.angles=&angles;menu.rows=&row;menu.row_count=1;menu.distance=4;
    CHECK(bh_menu_present(&menu)&&q.commands[10].kind==BH_DRAW_QUAD,"real menu emits rectangular text quad");
    memset(&consumer,0,sizeof(consumer));consumer.draw=&f.draw;
    /* Capture the actual queue payload through its real quad consumer below. */
    consumer.lookup=menu_image;consumer.lookup_user=&f;
    CHECK(bh_quad_queue_execute(&consumer,q.commands+10,&next)&&f.count==4,"menu text goes through real consumer and clipper");
    CHECK(close_to(f.observed[0].local.x,-.5)&&close_to(f.observed[0].local.y,-.2)&&close_to(f.observed[2].local.x,.5)&&close_to(f.observed[2].local.y,.2),"unchanged 1 by .4 menu text footprint");
    CHECK(f.observed[0].u==0&&f.observed[0].v==0&&f.observed[1].u==0&&f.observed[1].v==.5f&&f.observed[2].u==1&&f.observed[2].v==.5f&&f.observed[3].u==1&&f.observed[3].v==0,"original menu maps full page width and top half height to matching axes");
    CHECK(bh_draw_queue_dispose(&q),"menu label queue release");
}
static void clipping(void){Fixture f;BHVec3 p={3.5f,0,4},n={0,0,-1},up={0,1,0};unsigned long before;
    setup(&f);CHECK(draw(&f,&p,&n,&up,4,2,0,0,1) && f.clip.used>0 && f.calls==1,"quad passes real side clipping");
    CHECK(f.observed[0].sx<=1,"side intersection bounded");before=f.calls;p.x=100;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && f.calls==before && !f.draw.emitted,"side cull emits nothing");
    p.x=0;p.z=.5f;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && f.calls==before,"near cull emits nothing");
    p.z=101;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && f.calls==before,"far cull emits nothing");
    p.z=4;f.reject=1;CHECK(!draw(&f,&p,&n,&up,2,2,0,0,1) && f.draw.error==BH_SPRITE_QUAD_EMIT && !f.draw.emitted,"real downstream error propagated");
    f.reject=0;up.z=1;up.y=1;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && close_to(f.observed[0].local.y,1),"Gram-Schmidt removes normal component");
    up.y=0;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1),"parallel up retains original degenerate quad");
}
static void lighting(void){Fixture f;BHVec3 p={0,0,4},n={0,0,-1},up={0,1,0};
    setup(&f);f.directional.enabled=1;f.directional.direction.z=-1;
    f.directional.directional[0]=100.75f;f.directional.directional[1]=300;f.directional.directional[2]=-10;
    f.directional.ambient[0]=1.5f;f.directional.ambient[1]=4.5f;f.directional.ambient[2]=1.25f;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,.5f),"directional and ambient path");
    CHECK(f.color==0xff339800UL,"two integer conversions precede channel clamp (102*.5,304*.5,-8*.5)");
    f.directional.enabled=0;f.draw.point_enabled=1;f.lights.lights=&f.light;
    f.light.type=1;f.light.color=0x206080UL;f.light.intensity=1;f.light.effective_intensity=1;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,1),"world light owner called");
    CHECK(f.color==0xff206080UL,"actual ambient world-light contribution");
    f.directional.enabled=1;f.directional.directional[0]=f.directional.directional[1]=f.directional.directional[2]=0;
    f.directional.ambient[0]=f.directional.ambient[1]=f.directional.ambient[2]=0;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,.5f),"dynamic plus directional modulation path");
    CHECK(f.color==0xff081820UL,"dynamic scale occurs in real light owner and again in directional channel conversion");
    f.directional.enabled=0;f.light.type=2;f.light.direction.z=1;f.light.intensity=1;f.light.effective_intensity=1;f.light.color=0x204060;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && f.color==0xff204060UL,"front face reverses normal for world directional light");
    n.z=1;CHECK(draw(&f,&p,&n,&up,2,2,0,8,1) && f.color==0xff204060UL,"admitted back face uses unflipped negated normal");
}
static void golden(void){Fixture f;BHVec3 p,n,up;float size[2],uv[4];unsigned long a,i,j,word;
    memcpy(&n,bh_quad_case_normal,12);memcpy(&up,bh_quad_case_up,12);memcpy(size,bh_quad_case_size,8);
    for(a=0;a<sizeof(bh_quad_geometry_cases)/sizeof(bh_quad_geometry_cases[0]);++a){
        setup(&f);memcpy(&p,bh_quad_geometry_cases[a].center,12);f.camera.position=p;
        memset(&f.camera.projected,0,sizeof(f.camera.projected));f.camera.projected.m[0][0]=f.camera.projected.m[1][1]=1.0f/1073741824.0f;f.camera.projected.m[2][3]=4;
        CHECK(bh_clip_set_camera(&f.clip,&f.camera.projected),"independent golden projection binding");
        CHECK(draw(&f,&p,&n,&up,size[0],size[1],0,0,1) && f.count==4,"cancellation-sensitive quad emitted without clipping");
        for(i=0;i<4;++i)for(j=0;j<3;++j){memcpy(&word,((float *)&f.observed[i].local)+j,4);CHECK(word==bh_quad_geometry_cases[a].expected[i][j],"independent exact PC53 asymmetric-spill golden");}
    }
    p.x=p.y=0;p.z=4;n.x=n.y=0;n.z=-1;up.x=up.z=0;up.y=1;memcpy(uv,bh_quad_case_uv,sizeof(uv));
    for(a=0;a<sizeof(bh_quad_uv_cases)/sizeof(bh_quad_uv_cases[0]);++a){setup(&f);
        CHECK(bh_sprite_quad_draw(&f.draw,&f.image,&p,&n,&up,2,2,0,(unsigned char)bh_quad_uv_cases[a].flags,1,uv[0],uv[1],uv[2],uv[3]) && f.count==4,"retained UV sum golden emitted");
        for(i=0;i<4;++i){memcpy(&word,&f.observed[i].u,4);CHECK(word==bh_quad_uv_cases[a].expected[i][0],"exact retained U flip golden");memcpy(&word,&f.observed[i].v,4);CHECK(word==bh_quad_uv_cases[a].expected[i][1],"exact retained V flip golden");}
    }
}
int bh_sprite_quad_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    geometry();menu_label();clipping();lighting();golden();CHECK(bh_math_environment_supported(),"PC53 remains established");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"sprite-quad\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SPRITE_QUAD_STANDALONE
int main(void){unsigned long n;return bh_sprite_quad_checkpoint(&n);}
#endif
