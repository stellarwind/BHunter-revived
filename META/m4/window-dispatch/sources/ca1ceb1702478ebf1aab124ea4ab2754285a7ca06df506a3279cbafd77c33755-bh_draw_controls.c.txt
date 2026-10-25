#include "bh_draw_controls.h"
#include <string.h>
static BHDrawCommand *append(BHDrawQueue *q,unsigned long bucket) {
    static const unsigned long mapping[9]={1,2,3,4,5,0,6,7,8};
    unsigned long header,tail,index;BHDrawCommand *command;
    if(!q || !q->commands || q->count<9 || q->count>q->capacity){if(q)q->error=BH_DRAW_ARGUMENT;return NULL;}
    if(q->count==q->capacity){q->error=BH_DRAW_FULL;return NULL;}
    header=bucket<9?mapping[bucket]:0;tail=q->commands[header].resource;
    if(tail>=q->count || (q->commands[tail].next!=BH_DRAW_END && q->commands[tail].next>=q->count)){q->error=BH_DRAW_TOPOLOGY;return NULL;}
    index=q->count++;command=q->commands+index;command->next=q->commands[tail].next;
    q->commands[tail].next=index;q->commands[header].resource=index;q->error=BH_DRAW_OK;return command;
}
int __cdecl bh_draw_controls_state(BHDrawQueue *q,unsigned long bucket,long value) {
    BHDrawCommand *c=append(q,bucket);if(!c)return q && q->error==BH_DRAW_FULL?0:-1;
    c->kind=BH_DRAW_TEXTURE_MASK;c->resource=(unsigned long)value;return 1;
}
int __cdecl bh_draw_controls_clip_rect(BHDrawQueue *q,unsigned long bucket,float x,float y,float width,float height) {
    BHDrawCommand *c=append(q,bucket);float values[4];if(!c)return q && q->error==BH_DRAW_FULL?0:-1;
    values[0]=x;values[1]=y;values[2]=width;values[3]=height;
    c->kind=BH_DRAW_CLIP_RECT;memcpy(c->words,values,sizeof(values));return 1;
}
int __cdecl bh_draw_controls_texture_mask(BHDrawControlContext *c,unsigned long mask) {
    unsigned long i;if(!c)return 0;c->texture_mask=mask;
    /* Explicit binding adapter: original 00491778 was shared by every model. */
    if(c->resources){c->resources->prototype.texture_mask=mask;
        for(i=1;i<=c->resources->count;++i){const BHDrawResource *r=bh_render_resources_lookup(i,c->resources);
            if(!r || r->tag!=BH_RESOURCE_TAG_MODEL || !r->data.model.instance)return 0;
            r->data.model.instance->texture_mask=mask;}}
    return 1;
}
int __cdecl bh_draw_controls_execute(BHDrawControlContext *c,const BHDrawQueue *q,
    const BHDrawCommand *command,unsigned long *next) {
    float v[4];int ok;unsigned long following;
    if(!c || !q || !q->commands || !command || !next)return 0;
    following=command->next;if(following!=BH_DRAW_END && following>=q->count)return 0;
    if(command->kind==BH_DRAW_TEXTURE_MASK){ok=bh_draw_controls_texture_mask(c,command->resource);*next=command->next;return ok;}
    if(command->kind!=BH_DRAW_CLIP_RECT)return 0;
    if(following!=BH_DRAW_END && q->commands[following].kind==BH_DRAW_CLIP_RECT){*next=following;return 1;}
    if(!c->view)return 0;memcpy(v,command->words,sizeof(v));
    ok=bh_view_clip_rect(c->view,v[0],v[1],v[2],v[3]);
    if(ok){
        if(c->model_clip){c->model_clip->left=c->view->left;c->model_clip->right=c->view->right;c->model_clip->top=c->view->top;c->model_clip->bottom=c->view->bottom;}
        if(c->screen_clip){c->screen_clip->left=c->view->left;c->screen_clip->right=c->view->right;c->screen_clip->top=c->view->top;c->screen_clip->bottom=c->view->bottom;}
        if(c->attribute_clip){c->attribute_clip->left=c->view->left;c->attribute_clip->right=c->view->right;c->attribute_clip->top=c->view->top;c->attribute_clip->bottom=c->view->bottom;}
    }
    *next=command->next;return ok;
}
