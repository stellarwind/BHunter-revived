#include "bh_flash_flare.h"
#include "bh_queue_reserve.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <string.h>
typedef struct FlareRow{float position,size,alpha;unsigned long texture;}FlareRow;
static const FlareRow rows[14]={
 {1,-1,1,0},{-1,300,.1f,1},{1.7f,150,.1f,2},{-1.1f,80,.2f,3},
 {.3f,70,.5f,3},{-.7f,70,.5f,2},{.5f,60,.4f,2},{-.5f,60,.4f,3},
 {.7f,50,.3f,3},{-.3f,50,.3f,2},{.4f,40,.2f,2},{-.4f,40,.2f,3},
 {.8f,40,.1f,2},{-.9f,40,.1f,3}};
static float number(const unsigned long*p){float x;memcpy(&x,p,4);return x;}
static void store(unsigned long*p,float x){memcpy(p,&x,4);}
static int greater(double a,double b){return !_isnan(a)&&!_isnan(b)&&a>b;}
static int greater_equal(double a,double b){return !_isnan(a)&&!_isnan(b)&&a>=b;}
static BHFlashFlareContext *context(BHEntity*e){BHFlashFlareContext*c;memcpy(&c,e->payload+464,4);return c;}
int __cdecl bh_flash_flare_bind(BHFlashFlareContext*c,BHEntity*e,float lifetime,float strength,float size){
 if(!c||!c->pool||!c->queue||!c->queue->commands||!bh_script_flash_bind(c->pool,e,lifetime))return 0;
 memcpy(e->payload+8,&strength,4);memcpy(e->payload+12,&size,4);memcpy(e->payload+464,&c,4);e->refresh=bh_flash_flare_refresh;return 1;
}
void __cdecl bh_flash_flare_refresh(BHEntity*e,void*packet){BHFlashFlareContext*c=context(e);float life,strength,size,value;
 (void)packet;memcpy(&life,e->payload+4,4);memcpy(&strength,e->payload+8,4);memcpy(&size,e->payload+12,4);
 bh_round_float(&value,(1.0-(double)life*10.0)*strength);
 bh_flare_queue(c->queue,&e->position,size,value,c->textures[0],c->textures[1],c->textures[2],c->textures[3],0);
}
int __cdecl bh_flare_queue(BHDrawQueue*q,const BHVec3*p,float size,float strength,
 unsigned long first,unsigned long second,unsigned long third,unsigned long fourth,float angle){BHDrawCommand*c;
 if(!p)return -1;c=bh_queue_reserve(q,7);if(!c)return q&&q->error==BH_DRAW_FULL?0:-1;
 c->kind=BH_DRAW_FLARE;c->resource=0;memcpy(c->words,p,12);store(c->words+3,size);store(c->words+4,strength);store(c->words+5,angle);
 c->words[6]=first;c->words[7]=second;c->words[8]=third;c->words[9]=fourth?fourth:third;return 1;
}
int __cdecl bh_flare_execute(BHFlareSubmit*c,BHDrawCommand*command,unsigned long*next){
 float x,y,scale,norm,dx,dy,gain,remaining,size,alpha,sx,sy,angle;
 double normalized,threshold,deltax,deltay,radius,weight,product,n;long halfx,halfy;unsigned long i,id;BHTextureImage*image;int ok=1;
 if(!c||!c->camera||!c->view||!c->screen||!c->screen->draw||!c->screen->lookup||!command||!next||command->kind!=BH_DRAW_FLARE||
    c->width<2||c->height<=0||!(c->camera->focal>0)||!(c->view->near_z>0)||!bh_math_environment_supported())return 0;
 c->attempted=c->emitted=c->culled=c->failures=0;
 x=number(command->words);y=number(command->words+1);scale=number(command->words+2);
 if(!bh_project_point_scale(c->camera,c->view,&x,&y,&scale)){++c->culled;*next=command->next;return 1;}
 halfx=c->width/2;halfy=c->height/2;threshold=1.0/(double)scale;
 normalized=(double)number(command->words+4)/halfx;bh_round_float(&norm,normalized);store(command->words+4,norm);
 if(greater(threshold,normalized)){++c->culled;*next=command->next;return 1;}
 deltax=(double)x-halfx;deltay=(double)y-halfy;bh_round_float(&dx,deltax);bh_round_float(&dy,deltay);
 radius=sqrt(deltax*dx+deltay*dy);
 if(greater_equal(radius,(double)c->height)){++c->culled;*next=command->next;return 1;}
 weight=((double)norm-threshold)/(double)norm;bh_round_float(&gain,weight*(1.0-radius/(double)c->height));
 product=(double)gain*14.0;bh_round_float(&remaining,product);
 if(greater(product,2.0)&&!command->words[8]&&!command->words[9])remaining=2;
 angle=number(command->words+5);
 /* Normal positive-camera input yields at most14 entries. An invalid source
    magnitude never reads beyond the complete original immutable table. */
 if(greater(remaining,14)){++c->failures;*next=command->next;return 0;}
 i=0;
 while(greater(remaining,0)){
  id=command->words[6+rows[i].texture];
  if(id){
   n=greater(1.0,remaining)?(double)remaining*gain:gain;bh_round_float(&alpha,n*rows[i].alpha);
   n=rows[i].size;if(n<0)n=-((n*(double)number(command->words+3))*scale);
   bh_round_float(&size,(n*(double)c->width)*.0015625);
   bh_round_float(&sx,(double)dx*rows[i].position+halfx);bh_round_float(&sy,(double)dy*rows[i].position+halfy);
   ++c->attempted;image=c->screen->lookup(id,c->screen->lookup_user);
   if(!image||!bh_screen_quad_draw(c->screen->draw,image,sx,sy,size,size,angle,0,alpha)){++c->failures;ok=0;}
   else if(c->screen->draw->emitted)++c->emitted;else ++c->culled;
  }
  ++i;n=(double)remaining-1.0;bh_round_float(&remaining,n);if(!greater(n,0))break;
 }
 *next=command->next;return ok;
}
