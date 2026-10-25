#include "../src/bh_fade.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fade_cases.h"
static unsigned long checks,failures;
#define CHECK(v,n) do{++checks;if(!(v)){++failures;printf("fade: %s\n",n);}}while(0)
static void queue_state(void) {
 BHDrawQueue q;BHFadeContext c;unsigned long next,bits=0x7fc12345;float f;
 memset(&q,0,sizeof(q));memset(&c,0,sizeof(c));memcpy(&f,&bits,4);
 CHECK(bh_draw_queue_init(&q,10,0),"queue init");
 CHECK(bh_fade_queue(&q,f,0x87654321)==1,"queue raw state");
 CHECK(q.commands[9].resource==bits&&q.commands[9].words[0]==0x87654321,"verbatim payload");
 CHECK(bh_fade_execute(&c,q.commands+9,&next),"state consumer");memcpy(&bits,&c.amount,4);
 CHECK(bits==0x7fc12345&&c.color==0x87654321&&next==q.commands[9].next,"raw state and next");
 CHECK(bh_fade_queue(&q,0,0)==0,"full suppresses command");bh_draw_queue_dispose(&q);
}
static void indexed(void) {
 BHFadeContext c;BHRenderState r;BHTextureContext t;unsigned char p[12]={0,42,84,99,126,168,210,99,255,1,2,99};
 memset(&c,0,sizeof(c));memset(&r,0,sizeof(r));memset(&t,0,sizeof(t));r.textures=&t;t.software=1;
 c.render=&r;c.pixels=p;c.width=3;c.height=3;c.pitch=4;c.pixel_bytes=1;c.cached_amount=.125f;c.cached_color=0x123456;
 bh_fade_set(&c,1,0x00ffffff);CHECK(bh_fade_apply(&c),"indexed full white");
 CHECK(c.rebuilds==1&&c.pixels_changed==9,"indexed rebuild rows");
 CHECK(p[0]==251&&p[10]==251&&p[3]==99&&p[7]==99&&p[11]==99,"palette white and padding");
 CHECK(c.amount==.125f&&c.color==0x123456,"original cache-to-active copy");
 CHECK(bh_fade_apply(&c)&&c.rebuilds==1,"matching cached state reuses old table");
 bh_fade_dispose(&c);memset(p,0,sizeof(p));c.table_ready=0;
 CHECK(!bh_fade_apply(&c),"undefined equal-cache first-use rejected");bh_fade_dispose(&c);
 c.amount=0;c.cached_amount=1;c.color=0;
 CHECK(bh_fade_apply(&c),"indexed zero fade builds");
 CHECK(((unsigned char*)c.table)[0]==0&&((unsigned char*)c.table)[42]==42&&((unsigned char*)c.table)[255]==255,"palette basis and last entry");
 bh_fade_dispose(&c);
}
static void packed(void) {
 BHFadeContext c;BHRenderState r;BHTextureContext t;unsigned short p[8]={0,0xffff,0xf800,0xeeee,0x07e0,0x001f,0x1234,0xeeee};unsigned long i;
 memset(&c,0,sizeof(c));memset(&r,0,sizeof(r));memset(&t,0,sizeof(t));r.textures=&t;t.software=1;
 c.render=&r;c.pixels=(unsigned char*)p;c.width=3;c.height=2;c.pitch=8;c.pixel_bytes=2;
 c.masks[0]=0xf800;c.masks[1]=0x7e0;c.masks[2]=31;c.extract[0]=11;c.extract[1]=5;c.expand[0]=c.expand[2]=3;c.expand[1]=2;
 c.cached_amount=1;bh_fade_set(&c,0,0);
 CHECK(bh_fade_apply(&c),"565 zero fade");CHECK(p[0]==0&&p[1]==0xffff&&p[2]==0xf800&&p[4]==0x07e0&&p[5]==31&&p[6]==0x1234,"565 identity full range channels");
 CHECK(p[3]==0xeeee&&p[7]==0xeeee,"565 pitch padding");
 for(i=0;i<65536;++i)if(((unsigned short*)c.table)[i]!=(unsigned short)i)break;
 CHECK(i==65536,"entire 565 identity table");
 bh_fade_set(&c,1,0x00ff8000);CHECK(bh_fade_apply(&c),"565 tint full strength");
 CHECK(p[0]==0xfc00&&p[1]==0xfc00&&p[6]==0xfc00,"565 exact output tint");
 c.color=0xff000001;c.amount=1;c.cached_amount=0;
 CHECK(bh_fade_apply(&c),"signed packed color");
 CHECK(((unsigned short*)c.table)[0]==0,"signed color truncation channels");
 bh_fade_dispose(&c);
}
static void mixed_cases(void) {
 BHFadeContext c;BHRenderState r;BHTextureContext t;unsigned long i,got;const FadeCase *p;
 memset(&c,0,sizeof(c));memset(&r,0,sizeof(r));memset(&t,0,sizeof(t));r.textures=&t;t.software=1;c.render=&r;
 c.masks[0]=0xf800;c.masks[1]=0x7e0;c.masks[2]=31;c.extract[0]=11;c.extract[1]=5;c.expand[0]=c.expand[2]=3;c.expand[1]=2;
 c.cached_amount=-9;c.cached_color=0x123456;
 for(i=0;i<sizeof(fade_cases)/sizeof(fade_cases[0]);++i) {
  p=fade_cases+i;c.pixel_bytes=p->depth;memcpy(&c.amount,&p->amount,4);c.color=p->color;
  if(c.table_bytes && c.table_bytes<(c.pixel_bytes==1?256UL:131072UL))bh_fade_dispose(&c);
  CHECK(bh_fade_apply(&c),"mixed independent static table fixture builds");
  got=c.pixel_bytes==1?((unsigned char*)c.table)[p->index]:((unsigned short*)c.table)[p->index];
  CHECK(got==p->expected,"mixed signed RGB and quantization exact fixture");
 }
 bh_fade_dispose(&c);
}
int __cdecl bh_fade_checkpoint(unsigned long *count) {
 BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
 queue_state();indexed();packed();mixed_cases();CHECK(bh_math_environment_supported(),"PC53 retained");bh_math_environment_leave(&fp);
 if(count)*count=checks;printf("{\"scenario\":\"fade\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_FADE_STANDALONE
int main(void){unsigned long n;return bh_fade_checkpoint(&n);}
#endif
