#include "bh_message_panel.h"
#include "bh_round.h"
#include "bh_pickup_draw.h"
#include "bh_atlas_text.h"
#include <stddef.h>
#include <string.h>

typedef char message_panel_fields_size[sizeof(BHMessagePanelFields)==20?1:-1];
typedef char message_panel_cursor_offset[offsetof(BHMessagePanelFields,cursor)==12?1:-1];
static BHMessagePanel *active_panel;

BHMessagePanelFields *__cdecl bh_message_panel_fields(BHEntity *entity){
    return entity?(BHMessagePanelFields*)(entity->payload+4):NULL;
}
static int live(BHMessagePanel *panel){
    BHEntity *cursor;unsigned long count=0;
    if(!panel || !panel->pool || !panel->entity)return 0;
    cursor=panel->pool->active;
    while(cursor && count++<panel->pool->used){
        if(cursor==panel->entity)return cursor->generation==panel->generation;
        cursor=cursor->next;
    }
    return 0;
}
int __cdecl bh_message_text_plain(BHTextQueue *queue,BHAtlasPage *page,const char *text){
    BHTextCommand *command;unsigned long old,index;
    if(!queue || !page || !text)return -1;
    old=queue->write;index=old&255UL;
    queue->write=(old&0xffffff00UL)|((old+1UL)&255UL);
    if((queue->write&255UL)==(queue->read&255UL)){
        queue->write=old;queue->error=BH_TEXT_QUEUE_FULL;return 0;
    }
    command=queue->commands+index;
    command->kind=BH_TEXT_COMMAND_PLAIN;
    strncpy((char*)command->text,text,59);command->text[59]=0;
    command->page=page;queue->error=BH_TEXT_QUEUE_OK;return 1;
}
unsigned long __cdecl bh_message_text_execute_plain(const BHTextCommand *command){
    bh_atlas_page_text(command->page,command->text);
    return command->next;
}
int __cdecl bh_message_text_drain(BHTextQueue *queue,BHAtlasContext *atlas,
                                  unsigned long *processed){
    BHTextCommand *command;unsigned long count=0;
    if(processed)*processed=0;
    if(!queue)return -1;
    while((queue->read&255UL)!=(queue->write&255UL)){
        command=queue->commands+(queue->read&255UL);
        switch(command->kind){
        case BH_TEXT_COMMAND_TEXT:bh_text_queue_execute_text(command);break;
        case BH_TEXT_COMMAND_CLEAR:bh_text_queue_execute_clear(atlas,command);break;
        case BH_TEXT_COMMAND_PLAIN:bh_message_text_execute_plain(command);break;
        default:queue->error=BH_TEXT_QUEUE_KIND;
            if(processed)*processed=count;return -1;
        }
        queue->read=(queue->read&0xffffff00UL)|((queue->read+1UL)&255UL);
        ++count;
    }
    queue->error=BH_TEXT_QUEUE_OK;
    if(processed)*processed=count;
    return 1;
}
static int request(BHMessagePanel *panel,unsigned long resource){
    unsigned long handle;
    handle=bh_sound_request(panel->sound,resource,1.0f,1.0f,NULL,NULL);
    ++panel->sound_requests;
    if(handle==BH_SOUND_INVALID){panel->error=BH_PANEL_SOUND;return 0;}
    return 1;
}
static void stored_sub(float *out,float value,double amount){
    bh_round_float(out,(double)value-amount);
}
static float stored(double value){float result;bh_round_float(&result,value);return result;}
static int draw_quad(BHMessagePanel *panel,unsigned long image,float x,float y,
                     float width,float height){
    int result=bh_screen_quad_queue(panel->draw,7,image,x,y,width,height,0,0,1.0f);
    if(result>0)++panel->draw_commands;else panel->error=BH_PANEL_DRAW;
    return result>0;
}
static int draw_rect(BHMessagePanel *panel,unsigned long image,float x,float y,
                     float width,float height,float alpha,float u,float v){
    int result=bh_pickup_rect_queue(panel->draw,7,image,x,y,width,height,0,0,alpha,
                                    u,v,u,v,stored((double)panel->near_depth+.01));
    if(result>0)++panel->draw_commands;else panel->error=BH_PANEL_DRAW;
    return result>0;
}
static int icon(BHMessagePanel *panel,unsigned char initial,unsigned long *image){
    int slot=0;
    if(initial>'0' && initial<='9')slot=initial-'1';
    else if(initial>'@' && initial<='C')slot=9+initial-'A';
    else if(initial>'9' && initial<='@')slot=0;
    else if(initial>'C'){
        panel->error=BH_PANEL_ICON;return 0;
    }
    *image=panel->icon_images[slot];return 1;
}
static size_t first_message_length(const BHMessageState *messages){
    const char *end;
    if(!messages || !messages->arena_used || messages->arena_used>8192)return 0;
    end=(const char*)memchr(messages->arena,0,messages->arena_used);
    return end?(size_t)(end-messages->arena)+1:0;
}
static int startup(BHMessagePanel *panel,BHMessagePanelFields *fields){
    unsigned long image;int sound_ok,clear_ok;
    stored_sub(&fields->timer,fields->timer,1.0/30.0);
    if(fields->timer>0)return 1;
    if(!panel->messages->arena_used){fields->timer=.5f;return 1;}
    if(!icon(panel,(unsigned char)panel->messages->arena[0],&image))return 0;
    fields->icon_image=image;
    fields->cursor=panel->messages->arena+1;
    fields->timer=.3f;fields->scale=0;
    sound_ok=request(panel,panel->tv_on_sound);
    panel->mode_identity=0x00442b60UL;
    clear_ok=bh_text_queue_clear(panel->text,panel->page,0)>0;
    ++panel->clear_commands;
    if(!clear_ok)panel->error=BH_PANEL_TEXT_QUEUE;
    return sound_ok && clear_ok;
}
static int entry(BHMessagePanel *panel,BHMessagePanelFields *fields){
    size_t first;
    stored_sub(&fields->timer,fields->timer,1.0/30.0);
    stored_sub(&fields->scale,fields->scale,-1.0/9.0);
    if(fields->timer>0)return 1;
    first=first_message_length(panel->messages);
    if(!first){panel->error=BH_PANEL_MESSAGE;return 0;}
    fields->timer=first==panel->messages->arena_used?7.0f:1.5f;
    fields->flash=1.8f;fields->scale=1.0f;
    panel->mode_identity=0x00442950UL;return 1;
}
static int typing(BHMessagePanel *panel,BHMessagePanelFields *fields){
    unsigned char ch;char letter[2];int text_ok=1,sound_ok=1,off_ok=1;
    if(!fields->cursor || fields->cursor<panel->messages->arena ||
       fields->cursor>=panel->messages->arena+panel->messages->arena_used){
        panel->error=BH_PANEL_MESSAGE;return 0;
    }
    ch=(unsigned char)*fields->cursor;
    if(!ch)stored_sub(&fields->timer,fields->timer,1.0/30.0);
    else{
        ++fields->cursor;letter[0]=(char)ch;letter[1]=0;
        text_ok=bh_message_text_plain(panel->text,panel->page,letter)>0;
        ++panel->text_commands;
        if(ch!=' ' && ch!='\n')sound_ok=request(panel,panel->tv_char_sound);
    }
    stored_sub(&fields->flash,fields->flash,.02);
    if(!(fields->flash>=0))fields->flash=0;
    if(!(fields->timer>0)){
        fields->timer=.3f;panel->mode_identity=0x004428b0UL;
        off_ok=request(panel,panel->tv_off_sound);
    }
    if(!text_ok)panel->error=BH_PANEL_TEXT_QUEUE;
    return text_ok && sound_ok && off_ok;
}
static int exit_message(BHMessagePanel *panel,BHMessagePanelFields *fields){
    size_t first,remaining;
    stored_sub(&fields->timer,fields->timer,1.0/30.0);
    stored_sub(&fields->scale,fields->scale,1.0/9.0);
    if(fields->timer>0)return 1;
    first=first_message_length(panel->messages);
    if(!first){panel->error=BH_PANEL_MESSAGE;return 0;}
    if(first>panel->messages->arena_used){panel->error=BH_PANEL_MESSAGE;return 0;}
    remaining=panel->messages->arena_used-first;
    memmove(panel->messages->arena,panel->messages->arena+first,remaining);
    panel->messages->arena_used=(unsigned long)remaining;
    fields->timer=.5f;fields->scale=0;
    panel->mode_identity=0x00442bf0UL;return 1;
}
/* 00442440: draw the expanding television and ring, then age the notice.
   The source image handles and screen width replace original scattered globals. */
int __cdecl bh_message_panel_render(BHMessagePanel *panel){
    BHMessagePanelFields *fields;BHMessageState *messages;
    long unit,j,slot,index,top;int ok=1;float f_unit,x,y,size;
    size_t length;
    if(!panel || active_panel!=panel || !live(panel) || !panel->draw ||
       !panel->messages || panel->screen_width<=0){
        if(panel)panel->error=BH_PANEL_STALE;return 0;
    }
    messages=panel->messages;fields=bh_message_panel_fields(panel->entity);
    if(messages->ring_count<0 || messages->ring_count>8 ||
       messages->ring_start<0 || messages->ring_start>7 ||
       messages->arena_used>8192){panel->error=BH_PANEL_MESSAGE;return 0;}
    panel->error=BH_PANEL_OK;
    unit=panel->screen_width/10;f_unit=(float)unit;
    if(fields->timer==fields->timer && fields->timer!=0.0f &&
       fields->scale==fields->scale && fields->scale!=0.0f){
        top=(long)((double)unit*(messages->ring_count?1.7:1.1));
        x=stored((double)f_unit*1.05);y=(float)top;
        size=(float)(unit*2);
        if(!draw_quad(panel,fields->icon_image,x,y,size,
                      stored((double)size*fields->scale)))ok=0;
        if(!draw_rect(panel,panel->flash_image,stored((double)f_unit*3.6),
                      stored((double)f_unit*fields->flash+(double)top),
                      (float)(unit*3),stored((double)size*fields->scale),
                      1.0f,0.00390625f,0.9921875f))ok=0;
    }
    slot=messages->ring_start;
    for(j=0;j<messages->ring_count;j++){
        unsigned char initial=(unsigned char)messages->ring[slot][0];
        index=(long)initial-'1';if(initial>='A')index=(long)initial-'8';
        x=stored((.3-(double)j*(-.6))*(double)f_unit);
        y=stored((double)f_unit*.35);
        size=stored((double)f_unit*.5);
        if(slot==messages->last_index && !(messages->display_ticks&4)){
            if(!draw_rect(panel,panel->alert_image,x,y,size,size,1.0f,
                          0.5049999952316284f,0.49000000953674316f))ok=0;
        }else if(index>=0 && index<12){
            if(!draw_quad(panel,panel->icon_images[index],x,y,size,size))ok=0;
        }else if(index>=0){panel->error=BH_PANEL_ICON;ok=0;}
        slot=(slot+1)&7;
    }
    if(!(messages->display_ticks&4) && messages->last_index<0 &&
       messages->ring_count>0){
        j=messages->ring_count;
        x=stored((.3-(double)j*(-.6))*(double)f_unit);
        y=stored((double)f_unit*.35);
        size=stored((double)f_unit*.5);
        if(!draw_rect(panel,panel->alert_image,x,y,size,size,1.0f,
                      0.5049999952316284f,0.49000000953674316f))ok=0;
    }
    if(messages->display_ticks){
        --messages->display_ticks;
        if(!messages->display_ticks && messages->display_phase){
            messages->display_phase=0;
            if(messages->last_index>=0 && messages->last_index<8){
                const char *end=(const char*)memchr(
                    messages->ring[messages->last_index],0,512);
                length=end?(size_t)(end-messages->ring[messages->last_index])+1:0;
                if(!length){panel->error=BH_PANEL_MESSAGE;ok=0;}
                else if(length<=8192-messages->arena_used){
                    memcpy(messages->arena+messages->arena_used,
                           messages->ring[messages->last_index],length);
                    messages->arena_used+=(unsigned long)length;
                    if(panel->mode_identity==0x00442950UL && fields->timer>.5f)
                        fields->timer=.5f;
                }
            }
        }
        if(!messages->display_ticks)messages->last_index=-1;
    }
    return ok;
}
void __cdecl bh_message_panel_refresh(BHEntity *entity,void *packet){
    BHMessagePanel *panel=active_panel;(void)packet;
    if(panel && entity==panel->entity)bh_message_panel_render(panel);
}
int __cdecl bh_message_panel_advance(BHMessagePanel *panel){
    BHMessagePanelFields *fields;int result;
    if(!panel || active_panel!=panel || !live(panel) || !panel->messages ||
       !panel->text || !panel->sound || !panel->page){
        if(panel)panel->error=BH_PANEL_STALE;return 0;
    }
    fields=bh_message_panel_fields(panel->entity);
    panel->error=BH_PANEL_OK;
    switch(panel->mode_identity){
    case 0x00442bf0UL:result=startup(panel,fields);break;
    case 0x00442b60UL:result=entry(panel,fields);break;
    case 0x00442950UL:result=typing(panel,fields);break;
    case 0x004428b0UL:result=exit_message(panel,fields);break;
    default:panel->error=BH_PANEL_ARGUMENT;return 0;
    }
    return result;
}
void __cdecl bh_message_panel_step(BHEntity *entity){
    BHMessagePanel *panel=active_panel;
    if(panel && entity==panel->entity)bh_message_panel_advance(panel);
}
void __cdecl bh_message_panel_query(BHEntity *entity,void *packet){
    BHMessagePanel *panel=active_panel;
    if(!panel || !entity || !packet || entity!=panel->entity || !live(panel))return;
    if(*(long*)packet==4){
        if(!bh_entity_release(panel->pool,entity))panel->error=BH_PANEL_POOL;
        else if(panel->messages && panel->messages->panel_entity==entity){
            panel->messages->panel_entity=NULL;
            panel->messages->panel_step_identity=NULL;
        }
    }
}
BHEntity *__cdecl bh_message_panel_create(BHMessagePanel *panel){
    BHEntity *entity;BHMessagePanelFields *fields;
    if(!panel || !panel->pool || !panel->messages || !panel->text || !panel->sound ||
       !panel->draw || panel->screen_width<=0 || !panel->page || active_panel){
        if(panel)panel->error=BH_PANEL_ARGUMENT;return NULL;
    }
    if(panel->messages->panel_entity || panel->messages->panel_step_identity){
        panel->error=BH_PANEL_ARGUMENT;return NULL;
    }
    entity=bh_entity_acquire(panel->pool);
    if(!entity){panel->error=BH_PANEL_POOL;return NULL;}
    panel->entity=entity;panel->generation=entity->generation;
    fields=bh_message_panel_fields(entity);fields->timer=10.0f;fields->scale=0;
    panel->mode_identity=0x00442bf0UL;
    entity->step=bh_message_panel_step;
    entity->refresh=bh_message_panel_refresh;
    active_panel=panel;
    if(!bh_entity_set_query(panel->pool,entity,bh_message_panel_query)){
        active_panel=NULL;bh_entity_release(panel->pool,entity);
        panel->entity=NULL;panel->error=BH_PANEL_POOL;return NULL;
    }
    panel->messages->panel_entity=entity;
    panel->messages->panel_step_identity=&panel->mode_identity;
    panel->error=BH_PANEL_OK;return entity;
}
int __cdecl bh_message_panel_close(BHMessagePanel *panel){
    if(!panel || active_panel!=panel)return 0;
    if(live(panel) && !bh_entity_release(panel->pool,panel->entity)){
        panel->error=BH_PANEL_POOL;return 0;
    }
    if(panel->messages && panel->messages->panel_entity==panel->entity){
        panel->messages->panel_entity=NULL;panel->messages->panel_step_identity=NULL;
    }
    active_panel=NULL;panel->entity=NULL;panel->error=BH_PANEL_OK;return 1;
}
