#include "../src/bh_sprite_billboard.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<18)printf("FAIL sprite billboard %d: %s\n",line,label);}}
static int close_value(double a,double b){return fabs(a-b)<3e-5*(1+fabs(b));}
typedef struct Fixture {
    BHSpriteBillboardContext draw;
    BHMainCamera camera;
    BHViewConfig config;
    BHScreenClipContext clip;
    BHTextureImage image;
    BHClipVertex observed[12];
    unsigned long count,calls,color;
    int reject;
} Fixture;
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user) {
    Fixture *f=(Fixture *)user;BHClipVertex *p=ring;unsigned long count=0;
    CHECK(image==&f->image,"actual image forwarded to primitive emitter");
    do {
        CHECK(p->next->previous==p && p->previous->next==p,"reciprocal sprite ring");
        if(count<12)f->observed[count]=*p;
        ++count;p=p->next;
    }while(p!=ring && count<12);
    CHECK(p==ring && count>=3 && count<=12,"bounded clipped ring");
    f->count=count;f->color=color;++f->calls;return !f->reject;
}
static void setup(Fixture *f){
    memset(f,0,sizeof(*f));
    f->camera.projected.m[0][0]=f->camera.projected.m[1][1]=f->camera.projected.m[2][2]=1;
    f->camera.focal=100;
    f->config.near_z=1;f->config.far_z=100;
    f->config.left=0;f->config.right=200;f->config.top=0;f->config.bottom=200;
    bh_screen_clip_init(&f->clip,1,100,0,200,0,200);
    f->draw.camera=&f->camera;f->draw.config=&f->config;f->draw.clip=&f->clip;
    f->draw.emit=capture;f->draw.emit_user=f;f->draw.base_color=0xff000000UL;
}
static void basic(void){Fixture f;unsigned long k;setup(&f);
    CHECK(bh_sprite_billboard_draw(&f.draw,&f.image,1000,1000,10,4,2,0,0,1),"unrotated billboard draw");
    CHECK(f.calls==1 && f.count==4 && f.draw.emitted==1,"one quad emitted");
    CHECK(f.color==0xffffffffUL && f.draw.color==f.color,"original -255 grayscale conversion");
    CHECK(close_value(f.observed[0].sx,80) && close_value(f.observed[0].sy,90) && close_value(f.observed[1].sx,80) && close_value(f.observed[1].sy,110) &&
        close_value(f.observed[2].sx,120) && close_value(f.observed[2].sy,110) && close_value(f.observed[3].sx,120) && close_value(f.observed[3].sy,90),"independent rectangle coordinates");
    for(k=0;k<4;++k)CHECK(close_value(f.observed[k].point.z,10),"projected depth copied to full ring");
    CHECK(close_value(f.observed[0].u,0) && close_value(f.observed[0].v,0) && close_value(f.observed[1].u,0) && close_value(f.observed[1].v,1) &&
        close_value(f.observed[2].u,1) && close_value(f.observed[2].v,1) && close_value(f.observed[3].u,1) && close_value(f.observed[3].v,0),"default UV winding");
    CHECK(bh_sprite_billboard_draw(&f.draw,&f.image,1000,1000,10,4,2,0,7,.5f),"three UV flags");
    CHECK(close_value(f.observed[0].u,1) && close_value(f.observed[0].v,1) && close_value(f.observed[1].u,0) && close_value(f.observed[1].v,1) &&
        close_value(f.observed[2].u,0) && close_value(f.observed[2].v,0) && close_value(f.observed[3].u,1) && close_value(f.observed[3].v,0),"flips and quarter-turn UV mapping");
    CHECK(f.color==0xff7f7f7fUL,"half modulation truncates toward zero");
}
static void rotation_and_cull(void){Fixture f;unsigned long before;setup(&f);
    CHECK(bh_sprite_billboard_draw(&f.draw,&f.image,1000,1000,10,4,2,1.5707963267948966f,0,1),"rotated billboard draw");
    CHECK(f.count==4 && close_value(f.observed[0].sx,110) && close_value(f.observed[0].sy,80) &&
        close_value(f.observed[1].sx,90) && close_value(f.observed[1].sy,80) && close_value(f.observed[2].sx,90) && close_value(f.observed[2].sy,120) &&
        close_value(f.observed[3].sx,110) && close_value(f.observed[3].sy,120),"analytical quarter-turn rectangle");
    before=f.calls;CHECK(bh_sprite_billboard_draw(&f.draw,&f.image,1000,1000,.5f,4,2,0,0,1),"near-depth rejection succeeds");
    CHECK(f.calls==before && f.draw.emitted==0,"rejected depth never emits");
    CHECK(bh_sprite_billboard_draw(&f.draw,&f.image,100,100,10,4,2,0,0,1),"partially clipped billboard");
    CHECK(f.calls==before+1 && f.clip.used>0,"real screen clipper generated boundary vertices");
    f.reject=1;CHECK(!bh_sprite_billboard_draw(&f.draw,&f.image,1000,1000,10,4,2,0,0,1) &&
        f.draw.error==BH_SPRITE_BILLBOARD_EMIT && f.draw.emitted==0,"primitive failure propagates");
}
int bh_sprite_billboard_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=0;failures=0;
    if(!bh_math_environment_enter(&fp))return 2;
    basic();rotation_and_cull();CHECK(bh_math_environment_supported(),"caller PC53 profile retained");
    bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"sprite-billboard\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_SPRITE_BILLBOARD_STANDALONE
int main(void){unsigned long count;return bh_sprite_billboard_checkpoint(&count);}
#endif
