#include "bh_hud_scores.h"
#include "bh_atlas_text.h"
#include "bh_round.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <float.h>

typedef char scores_payload_offset[offsetof(BHEntity,payload)==0xe4?1:-1];
typedef char scores_word_size[sizeof(long)==4 && sizeof(int)==4 && sizeof(short)==2?1:-1];
static long score(const BHEntity *entity) {
    short value;memcpy(&value,entity->payload+56,2);return value;
}
static long row_y(long index) {
    unsigned long value=(unsigned long)index*16UL+2UL;long result;
    memcpy(&result,&value,4);return result;
}
static void text_result(BHHudScoreText *c,int result) {
    ++c->attempted;if(result>0)++c->queued;else if(!result)++c->dropped;else c->error=BH_HUD_SCORES_QUEUE;
}
static void row(BHHudScoreText *c,const char *name,long amount,long y) {
    char buffer[128];long width;
    text_result(c,bh_text_queue_text(c->queue,c->page,name,1,y));
    sprintf(buffer,"%d",(int)amount);
    width=bh_atlas_page_text_width(c->page,(const unsigned char *)buffer);
    text_result(c,bh_text_queue_text(c->queue,c->page,buffer,126-width,y));
}
int __cdecl bh_hud_scores_text(BHHudScoreText *c,const BHHudScores *s) {
    unsigned long i;int cleared=0;
    if(!c || !s || !c->queue || !s->remote || !s->local_name){if(c)c->error=BH_HUD_SCORES_ARGUMENT;return 0;}
    c->error=BH_HUD_SCORES_OK;c->attempted=c->queued=c->dropped=0;
    for(i=0;i<8;++i)if(s->remote[i]) {
        if(!cleared)text_result(c,bh_text_queue_clear(c->queue,c->page,0));
        cleared=1;
        row(c,(const char *)s->remote[i]->payload+58,score(s->remote[i]),row_y((long)i));
    }
    if(!cleared)text_result(c,bh_text_queue_clear(c->queue,c->page,0));
    row(c,s->local_name,s->local_score,row_y(s->local_index));
    return c->error==BH_HUD_SCORES_OK;
}
static void draw_row(BHHudScoreDraw *c,long index,long ordinal,int local,float unit) {
    float x,y,width,v,depth;int result;
    bh_round_float(&x,(double)(c->display_width/2));
    bh_round_float(&y,((double)ordinal-(-.5))*(double)unit);
    bh_round_float(&width,(double)unit*8.0);
    bh_round_float(&v,(double)index*.125);
    bh_round_float(&depth,(double)c->near_z-(-.01));
    result=bh_pickup_rect_queue(c->queue,3,c->page_image,x,y,width,unit,0,0,
        local?1.0f:.5f,0,1,v,.125f,depth);
    ++c->attempted;if(result>0)++c->queued;else if(!result)++c->dropped;else c->error=BH_HUD_SCORES_QUEUE;
}
int __cdecl bh_hud_scores_draw(BHHudScoreDraw *c,const BHHudScores *s) {
    long upper=100000,best,index,ordinal=0;unsigned long i;float unit;
    if(!c || !s || !c->queue || !s->remote || !_finite(c->near_z)){
        if(c)c->error=BH_HUD_SCORES_ARGUMENT;return 0;
    }
    c->error=BH_HUD_SCORES_OK;c->attempted=c->queued=c->dropped=0;
    bh_round_float(&unit,(double)(c->display_width/40));
    for(;;) {
        best=-100000;index=-1;
        for(i=0;i<8;++i)if((long)i!=s->local_index && s->remote[i]) {
            long value=score(s->remote[i]);
            if(best<value && value<upper){best=value;index=(long)i;}
        }
        if(best<s->local_score && s->local_score<upper){index=s->local_index;best=s->local_score;}
        if(index<0)return c->error==BH_HUD_SCORES_OK;
        for(i=0;i<8;++i)if((long)i!=s->local_index && s->remote[i] && score(s->remote[i])==best)
            draw_row(c,(long)i,ordinal++,0,unit);
        if(s->local_score==best)draw_row(c,s->local_index,ordinal++,1,unit);
        upper=best;
    }
}
