#include "../src/bh_effect_draw.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL effect-draw %d: %s\n",__LINE__,s);}}while(0)
static float value(const unsigned long *p){float f;memcpy(&f,p,4);return f;}
static void cases(void){BHEntityPool pool;BHEntity *e,saved;BHDrawQueue q;BHEffectDrawContext c;BHVec3 velocity={1,0,1},rotation={.25f,.5f,.75f};float phase=2,roll=.125f;unsigned long model=99,seed;int a,b,next;
    CHECK(bh_entities_init(&pool,1,NULL),"actual entity owner");e=bh_entity_acquire(&pool);CHECK(e!=NULL,"real live entity");if(!e)return;
    e->position.x=10;e->position.y=20;e->position.z=30;memcpy((unsigned char*)e+0xf8,&velocity,12);memcpy((unsigned char*)e+0xe8,&phase,4);memcpy((unsigned char*)e+0xf0,&roll,4);memcpy((unsigned char*)e+0x148,&model,4);memcpy((unsigned char*)e+0x3c,&rotation,12);saved=*e;
    memset(&c,0,sizeof(c));c.queue=&q;c.directional_model=41;c.fixed_model=42;c.timed_texture=51;c.random_texture=52;c.fixed_texture=53;
    CHECK(bh_draw_queue_init(&q,16,NULL),"actual source bucket queue");CHECK(bh_effect_directional_draw(&c,e)==1&&q.count==11,"whole directional model and flare queued");
    CHECK(q.commands[9].kind==BH_DRAW_MODEL_RESOURCE&&q.commands[9].resource==41&&q.commands[1].next==9,"actual model producer bucket0");
    CHECK(!memcmp(q.commands[9].words,&e->position,12)&&fabs(value(q.commands[9].words+3)-.7853981633974483)<.000001&&value(q.commands[9].words+4)==0&&value(q.commands[9].words+5)==0&&value(q.commands[9].words+6)==1,"velocity yaw/pitch, zero roll and unit modulation");
    CHECK(q.commands[10].kind==BH_DRAW_FLARE&&q.commands[7].next==10&&value(q.commands[10].words+3)==4&&value(q.commands[10].words+4)==50&&value(q.commands[10].words+5)==3&&q.commands[10].words[6]==51,"exact timed flare and phase*1.5, bucket7");
    bh_draw_queue_dispose(&q);
    for(seed=0;seed<16;++seed){srand(seed);a=rand();b=rand();next=rand();srand(seed);CHECK(bh_draw_queue_init(&q,16,NULL),"variable real queue");
        CHECK(bh_effect_variable_draw(&c,e)==1&&q.count==11&&q.commands[9].resource==99&&value(q.commands[9].words+5)==roll,"optional model and retained roll");
        CHECK(value(q.commands[10].words+4)==(float)((double)a/128)&&value(q.commands[10].words+3)==(float)((double)b/16384)&&value(q.commands[10].words+5)==0&&q.commands[10].words[6]==52,"two random values map strength then size, angle zero");
        CHECK(rand()==next,"exact shared CRT random consumption");bh_draw_queue_dispose(&q);
    }
    model=0;memcpy((unsigned char*)e+0x148,&model,4);CHECK(bh_draw_queue_init(&q,16,NULL),"no-model queue");CHECK(bh_effect_variable_draw(&c,e)==1&&q.count==10&&q.commands[9].kind==BH_DRAW_FLARE,"NULL model still emits full random flare");bh_draw_queue_dispose(&q);
    model=99;memcpy((unsigned char*)e+0x148,&model,4);
    CHECK(bh_draw_queue_init(&q,10,NULL),"one-slot remaining queue");CHECK(bh_effect_fixed_draw(&c,e)==0&&q.count==10&&q.commands[9].resource==42&&!memcmp(q.commands[9].words+3,&rotation,12),"whole fixed owner keeps model on later flare exhaustion");bh_draw_queue_dispose(&q);
    CHECK(bh_draw_queue_init(&q,16,NULL),"fixed complete queue");CHECK(bh_effect_fixed_draw(&c,e)==1&&value(q.commands[10].words+3)==4&&value(q.commands[10].words+4)==250&&value(q.commands[10].words+5)==0&&q.commands[10].words[6]==53,"exact fixed flare constants and identity");bh_draw_queue_dispose(&q);
    srand(27);rand();rand();next=rand();srand(27);CHECK(bh_draw_queue_init(&q,9,NULL),"completely full queue");CHECK(bh_effect_variable_draw(&c,e)==0&&q.count==9&&rand()==next,"random calls retained despite both failed producers");bh_draw_queue_dispose(&q);
    CHECK(!memcmp(e,&saved,sizeof(saved)),"all three complete draw owners retain every entity byte");CHECK(bh_effect_fixed_draw(NULL,e)==-1,"source invalid context guard");
    CHECK(bh_entity_release(&pool,e)&&bh_entities_dispose(&pool),"actual entity release and disposal");
}
int bh_effect_draw_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;cases();bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"effect-draw\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures;}
