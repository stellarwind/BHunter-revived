#include "../src/bh_remote_render.h"
#include "../src/bh_round.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
static float word(unsigned long value){float out;memcpy(&out,&value,4);return out;}
int bh_remote_render_checkpoint(unsigned long *count){
    BHEntity e,saved;BHVec3 p,a,reference,anchors[2],v[2],acc[2];
    BHRemoteRender c;BHDrawQueue q;BHSoundQueue sound;BHDrawCommand *d;
    unsigned short stamp,tick;short model=2;unsigned long handle,resource=BH_SOUND_INVALID,frames;
    unsigned long checks=0,failures=0,i,j,k;int next,expected;
#define CHECK(x) do{++checks;if(!(x)){++failures;printf("FAIL remote render %d\n",__LINE__);}}while(0)
    memset(&e,0,sizeof(e));memset(&c,0,sizeof(c));
    for(j=0;j<2;j++)for(i=0;i<3;i++){
        (&anchors[j].x)[i]=(float)(j*12+i+1);
        (&v[j].x)[i]=(float)(i+1)*.125f;
        (&acc[j].x)[i]=(float)(j+1)*.03125f;
    }
    memcpy((char*)&e+0x180,anchors,sizeof(anchors));memcpy((char*)&e+0x48,v,sizeof(v));memcpy((char*)&e+0x60,acc,sizeof(acc));
    stamp=65530;memcpy((char*)&e+0x17c,&stamp,2);saved=e;
    for(k=0;k<128;k++){
        tick=(unsigned short)(stamp+k);CHECK(bh_remote_predict_pose(&e,tick,&p,&a));
        CHECK(bh_remote_predict_position(&e,tick,&reference)&&!memcmp(&p,&reference,12));
        for(i=0;i<3;i++){
            float first,want;bh_round_float(&first,(double)(&v[1].x)[i]*k+(&anchors[1].x)[i]);
            bh_round_float(&want,(double)(&acc[1].x)[i]*(k*k*.5)+first);CHECK((&a.x)[i]==want);
        }
    }
    CHECK(bh_remote_predict_pose(&e,(unsigned short)(stamp-1),&p,&a)&&!memcmp(&p,anchors,12)&&!memcmp(&a,anchors+1,12));
    CHECK(bh_remote_predict_pose(&e,(unsigned short)(stamp+32768),&p,&a)&&!memcmp(&p,anchors,12));
    CHECK(!memcmp(&e,&saved,sizeof(e)));
    CHECK(bh_draw_queue_init(&q,34,NULL));CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL));
    c.queue=&q;c.sound=&sound;c.models[2]=52;c.spark_texture=73;c.tick=stamp;
    handle=BH_SOUND_INVALID;memcpy((char*)&e+0x104,&handle,4);memcpy((char*)&e+0x10c,&resource,4);memcpy((char*)&e+0x114,&model,2);
    srand(42);expected=rand();srand(42);CHECK(bh_remote_render(&c,&e)&&c.attempted==1&&!c.dropped&&q.count==10);next=rand();CHECK(next==expected);
    memcpy(&frames,(char*)&e+0x108,4);CHECK(frames==60);
    d=q.commands+9;CHECK(d->kind==BH_DRAW_MODEL_RESOURCE&&d->resource==52&&word(d->words[0])==1&&word(d->words[3])==13);
    CHECK(bh_draw_queue_begin(&q));*((unsigned char*)&e+0x11a)=1;
    srand(123);for(i=0;i<96;i++)rand();expected=rand();srand(123);
    CHECK(bh_remote_render(&c,&e)&&c.attempted==25&&!c.dropped&&q.count==34);CHECK(rand()==expected);
    for(i=10;i<34;i++){
        double length=0;d=q.commands+i;CHECK(d->kind==BH_DRAW_ORIENTED&&d->resource==73&&d->words[13]==8);
        for(j=0;j<3;j++){double n=word(d->words[6+j]);length+=n*n;CHECK(fabs(word(d->words[j])-word(d->words[6+j])-(&anchors[0].x)[j])<.000002);}
        CHECK(fabs(length-9)<.00001&&word(d->words[9])==3&&word(d->words[10])==3&&word(d->words[12])==.3f);
    }
    CHECK(bh_draw_queue_begin(&q));q.capacity=9;
    srand(123);CHECK(bh_remote_render(&c,&e)&&c.attempted==25&&c.dropped==25);CHECK(rand()==expected);
    CHECK(bh_sound_queue_dispose(&sound));CHECK(bh_draw_queue_dispose(&q));
    if(count)*count=checks;printf("{\"scenario\":\"remote-render\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
#undef CHECK
}
