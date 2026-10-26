#include "../src/bh_effect_puff.h"
#include "../src/bh_delayed_expiry.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL puff %s\n",n);}}while(0)
static void value(BHEntity*e,unsigned long a,float f){memcpy((char*)e+a,&f,4);}
static float number(const BHDrawCommand*c,int a){float f;memcpy(&f,c->words+a,4);return f;}
int bh_effect_puff_checkpoint(unsigned long*count){BHEntityPool pool;BHDrawQueue q;BHEffectPuffContext c;BHEntity *e;BHMathEnvironment fp;unsigned long seed=0x816700ff,after,expected,i;unsigned char before[440];BHDrawCommand first;checks=failures=0;
 CHECK(bh_math_environment_enter(&fp),"PC53");CHECK(bh_entities_init(&pool,2,NULL)&&bh_draw_queue_init(&q,160,NULL),"real pool/queue");memset(&c,0,sizeof(c));c.queue=&q;e=bh_entity_acquire(&pool);CHECK(e!=NULL,"real allocation");e->position.x=2;e->position.y=3;e->position.z=4;value(e,0xe8,.5f);value(e,0xec,1);value(e,0x194,100);value(e,0x198,.1f);memcpy((char*)e+0x1a8,&seed,4);CHECK(bh_effect_puff_bind(&c,e,0),"random owner binding");e->refresh(e,NULL);CHECK(!c.attempted,"delay suppresses all producers");value(e,0x198,0);memcpy(before,e,sizeof(before));srand(17);expected=rand();srand(17);e->refresh(e,NULL);after=rand();CHECK(c.attempted==48&&c.queued==48&&q.count==57,"48 actual oriented commands");CHECK(after==expected&&!memcmp(before,e,sizeof(before)),"local generator leaves global RNG/seed/all entity fields unchanged");first=q.commands[9];CHECK(first.kind==BH_DRAW_ORIENTED&&first.words[13]==8,"random draw owner control");CHECK(fabs(number(&first,0)-2)<=50&&fabs(number(&first,1)-3)<=50&&fabs(number(&first,2)-4)<=50,"normalized radial positions");CHECK(bh_draw_queue_begin(&q),"queue reset");e->refresh(e,NULL);CHECK(!memcmp(&first,&q.commands[9],sizeof(first)),"unchanged seed repeats exact command");
 CHECK(bh_draw_queue_begin(&q)&&bh_effect_puff_bind(&c,e,1),"ring actual owner binding");c.attempted=c.queued=0;e->refresh(e,NULL);CHECK(c.attempted==64&&c.queued==64&&q.count==73,"64 actual ring commands");first=q.commands[9];CHECK(first.words[13]==12&&fabs(number(&first,0)-52)<.00001&&number(&first,1)==3&&number(&first,2)==4,"first ring point/control");
 for(i=9;i<73;++i)CHECK(fabs(number(&q.commands[i],1)-3)<.00001,"ring constant world height");
 CHECK(bh_draw_queue_dispose(&q)&&bh_draw_queue_init(&q,10,NULL),"one command available");c.attempted=c.queued=0;e->refresh(e,NULL);CHECK(c.attempted==64&&c.queued==1&&q.count==10,"full queue attempts every original producer");CHECK(bh_delayed_expiry_bind(&pool,e,.01f,0)&&bh_effect_puff_bind(&c,e,0),"real delayed expiry context independent");CHECK(bh_entities_step(&pool)&&!pool.used,"actual whole expiry executes");CHECK(bh_draw_queue_dispose(&q)&&bh_entities_dispose(&pool),"dispose");bh_math_environment_leave(&fp);if(count)*count+=checks;printf("{\"scenario\":\"effect-puff\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;}
#ifdef BH_EFFECT_PUFF_STANDALONE
int main(void){unsigned long n=0;return bh_effect_puff_checkpoint(&n);}
#endif
