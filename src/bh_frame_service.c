#include "bh_frame_service.h"
#include "bh_input_devices.h"
#include "bh_cd_close.h"
#include "bh_network_shutdown.h"
#include "bh_force_feedback.h"
#include "bh_display_shutdown.h"

static void failure(BHFrameService *s,BHFrameServiceError error){
    ++s->failures;if(s->error==BH_FRAME_SERVICE_OK)s->error=error;
}
static int cleanup(BHFrameService *s,LONG stop){
    unsigned long bits;
    if(!*s->worker_done||stop>1)return 1;
    if(!s->devices||!s->cd||!s->sound||!s->network||!s->force_feedback||!s->display){
        failure(s,BH_FRAME_SERVICE_ARGUMENT);return 0;
    }
    bits=(unsigned long)stop+1UL;*s->stop=(LONG)bits;
    ++s->shutdown_calls;s->cleanup_count=0;
    s->cleanup_order[s->cleanup_count++]=1;if(!bh_input_devices_stop(s->devices))failure(s,BH_FRAME_SERVICE_SHUTDOWN);
    s->cleanup_order[s->cleanup_count++]=2;if(!bh_cd_stop_close(s->cd))failure(s,BH_FRAME_SERVICE_SHUTDOWN);
    s->cleanup_order[s->cleanup_count++]=3;if(!bh_sound_queue_dispose(s->sound))failure(s,BH_FRAME_SERVICE_SHUTDOWN);
    s->cleanup_order[s->cleanup_count++]=4;bh_network_shutdown(s->network);
    s->cleanup_order[s->cleanup_count++]=5;bh_force_feedback_stop(s->force_feedback);
    s->cleanup_order[s->cleanup_count++]=6;if(!bh_display_shutdown(s->display))failure(s,BH_FRAME_SERVICE_SHUTDOWN);
    return s->error==BH_FRAME_SERVICE_OK;
}
int __cdecl bh_frame_service_tick(BHFrameService *s){
    void *buffer;long active;unsigned long i;LONG stop;
    if(!s)return 0;
    ++s->calls;s->error=BH_FRAME_SERVICE_OK;
    if(!s->stop||!s->worker_done){failure(s,BH_FRAME_SERVICE_ARGUMENT);return 0;}
    stop=*s->stop;if(stop)return cleanup(s,stop);
    if(!s->input||!s->entities||!s->sound||!s->frames||!s->draw||!s->paused||
       !s->frame_counter||!s->player_input||!s->packet||s->packet_bytes<2||
       s->frames->bytes<3500UL*sizeof(BHDrawCommand)||s->draw->allocator.allocate||s->draw->allocator.release){
        failure(s,BH_FRAME_SERVICE_ARGUMENT);return 0;
    }
    for(i=0;i<3;++i)if(!s->slot_draw[i]||s->slot_draw[i]==s->draw||
       (i&&s->slot_draw[i]==s->slot_draw[0])||(i==2&&s->slot_draw[2]==s->slot_draw[1])){
        failure(s,BH_FRAME_SERVICE_ARGUMENT);return 0;
    }
    ++s->input_calls;if(!bh_input_collect(s->input,&s->changed))failure(s,BH_FRAME_SERVICE_INPUT);
    if(!*s->paused)*s->frame_counter=*s->frame_counter+1UL;
    buffer=bh_frame_buffers_acquire(s->frames);
    if(!buffer){failure(s,BH_FRAME_SERVICE_BUFFER);return 0;}
    active=s->frames->active;s->draw->commands=(BHDrawCommand*)buffer;s->draw->capacity=3500;
    if(!bh_draw_queue_begin(s->draw))failure(s,BH_FRAME_SERVICE_BUFFER);
    ++s->step_calls;if(!bh_entities_step(s->entities))failure(s,BH_FRAME_SERVICE_ENTITY);
    ++s->refresh_calls;if(!bh_entities_refresh(s->entities))failure(s,BH_FRAME_SERVICE_ENTITY);
    s->last_dt=0.033333335071802139f;
    ++s->sound_calls;if(!bh_sound_tick(s->sound,s->last_dt,s->master_db,&s->sound_stats))failure(s,BH_FRAME_SERVICE_SOUND);
    ++s->packet_calls;if(!bh_player_input_apply_packet_checked(s->player_input,s->packet,s->packet_bytes))failure(s,BH_FRAME_SERVICE_PACKET);
    /* The worker sees metadata for THIS allocation; the canonical descriptor
       can move to another slot as soon as this one has been published. */
    *s->slot_draw[active]=*s->draw;
    if(!bh_frame_buffers_publish(s->frames))failure(s,BH_FRAME_SERVICE_BUFFER);
    else ++s->produced;
    return s->error==BH_FRAME_SERVICE_OK;
}
