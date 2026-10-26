#include "../src/bh_pulse_visual.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL pulse-visual %lu %s\n",checks,s);}}while(0)
static float field(const BHDrawCommand *d,unsigned long i){float f;memcpy(&f,d->words+i,4);return f;}
static void put(BHEntity *e,unsigned long a,float f){memcpy((unsigned char*)e+a,&f,4);}
int __cdecl bh_pulse_visual_checkpoint(unsigned long *count){BHEntity e;BHMathEnvironment fp;BHDrawQueue queue;BHPulseVisualContext c;BHDrawCommand *d;float strength;memset(&e,0,sizeof(e));memset(&c,0,sizeof(c));CHECK(bh_math_environment_enter(&fp),"PC53");CHECK(bh_draw_queue_init(&queue,40,NULL),"actual queue owner");c.queue=&queue;c.model=17;c.image=29;e.position.x=3;e.position.y=6;e.position.z=9;put(&e,0xe8,4);put(&e,0xec,0);
 CHECK(bh_pulse_visual_draw(&c,&e)==1&&c.attempted==3&&c.queued==3,"whole early visual has two resources and billboard");d=queue.commands+9;CHECK(d->kind==BH_DRAW_MODEL_RESOURCE&&d->resource==17&&field(d,0)==3&&field(d,1)==6&&field(d,2)==9,"first resource actual position");CHECK(field(d,3)==0&&field(d,4)==0&&field(d,5)==0&&field(d,6)==1,"first resource angles modulation");d=queue.commands+10;CHECK(fabs(field(d,3)-3.141592653589793)<.000001&&field(d,4)==0,"opposed second resource orientation");d=queue.commands+11;CHECK(d->kind==BH_DRAW_BILLBOARD&&d->resource==29&&fabs(field(d,3)-.8)<.000001&&field(d,5)==-4,"early billboard radius and phase");
 bh_draw_queue_begin(&queue);put(&e,0xe8,1);CHECK(bh_pulse_visual_draw(&c,&e)==1&&queue.count==13,"late visual adds actual flare producer");strength=(float)(.1/1.001);d=queue.commands+9;CHECK(fabs(field(d,2)-(9+strength*.5))<.000001&&field(d,5)==-3,"first model split offset and lifetime roll");d=queue.commands+10;CHECK(fabs(field(d,2)-(9-strength*.5))<.000001,"second model opposite split offset");d=queue.commands+11;CHECK(fabs(field(d,3)-(.8+strength*2))<.000001,"late billboard expands with cubic envelope");d=queue.commands+12;CHECK(d->kind==BH_DRAW_FLARE&&field(d,3)==20&&fabs(field(d,4)-strength*25)<.000001&&field(d,5)==2,"actual flare cubic scale and doubled age");
 bh_draw_queue_begin(&queue);put(&e,0xe8,2);CHECK(bh_pulse_visual_draw(&c,&e)==1&&queue.count==12,"inclusive two threshold omits flare");queue.capacity=9;bh_draw_queue_begin(&queue);put(&e,0xe8,1);strength=(float)c.attempted;CHECK(bh_pulse_visual_draw(&c,&e)==0&&c.attempted==(unsigned long)strength+4&&queue.count==9,"full queue preserves all producer attempts");queue.capacity=40;CHECK(bh_draw_queue_dispose(&queue),"actual disposal");bh_math_environment_leave(&fp);if(count)*count+=checks;return !failures;}
#ifdef BH_PULSE_VISUAL_STANDALONE
int main(void){unsigned long n=0;int ok=bh_pulse_visual_checkpoint(&n);printf("pulse-visual: %lu checks, %lu failures\n",n,failures);return !ok;}
#endif
