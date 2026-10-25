#include "bh_sound_queue.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <string.h>

typedef char distance_offset[offsetof(BHSoundVoice,distance_squared)==0x2c ? 1:-1];
typedef char listener_offset[offsetof(BHSoundVoice,listener_index)==0x40 ? 1:-1];
typedef char range_offset[offsetof(BHSoundResource,audible_range)==0x94 ? 1:-1];
static int native_result(BHSoundQueue *q,BHSoundTickStats *s,HRESULT hr) {
    q->last_hresult=hr;
    if(hr!=DS_OK) {s->last_error=hr;q->error=BH_SOUND_BACKEND;return 0;}
    return 1;
}
static long truncate_low(double x) {
    __int64 converted=(__int64)x;unsigned long low=(unsigned long)converted;long result;
    memcpy(&result,&low,4);return result;
}
static long clamp(long value,long low,long high) {return value<low ? low:value>high ? high:value;}
static void select_listener(BHSoundQueue *q,BHSoundVoice *v,BHVec3 *scratch) {
    double x,y,z,distance=0;
    v->distance_squared=9e37f;
    if(v->positioned) {
        x=(double)v->position.x-q->listener.x;y=(double)v->position.y-q->listener.y;z=(double)v->position.z-q->listener.z;
        bh_round_float(&scratch->x,x);bh_round_float(&scratch->y,y);bh_round_float(&scratch->z,z);
        distance=(x*scratch->x+y*scratch->y)+z*scratch->z;
    }
    if(distance<v->distance_squared) {
        bh_round_float(&v->distance_squared,distance);v->cached=*scratch;v->listener_index=0;
    }
}
static void spatial(BHSoundQueue *q,BHSoundResource *r,BHSoundVoice *v,
    float *attenuation,double *pan,float *factor) {
    double length,dot,n;float distance;
    *attenuation=0;*pan=0;*factor=1;
    if(!v->positioned) return;
    length=sqrt((double)v->distance_squared);bh_round_float(&distance,length);
    if(length>r->audible_range)
        bh_round_float(attenuation,-(((double)distance-r->audible_range)/r->audible_range)*1000.0);
    if(distance==0) return;
    if(r->streaming) dot=((double)v->cached.y*q->listener_right.y+(double)v->cached.z*q->listener_right.z)+(double)v->cached.x*q->listener_right.x;
    else dot=((double)v->cached.x*q->listener_right.x+(double)v->cached.y*q->listener_right.y)+(double)v->cached.z*q->listener_right.z;
    n=dot/distance;*pan=(n*n)*(n>0 ? 10000.0:-10000.0);
    if(v->moving) {
        if(r->streaming) dot=(((double)v->velocity.y-q->listener_velocity.y)*v->cached.y+
            ((double)v->velocity.z-q->listener_velocity.z)*v->cached.z)+((double)v->velocity.x-q->listener_velocity.x)*v->cached.x;
        else dot=(((double)v->velocity.x-q->listener_velocity.x)*v->cached.x+
            ((double)v->velocity.y-q->listener_velocity.y)*v->cached.y)+((double)v->velocity.z-q->listener_velocity.z)*v->cached.z;
        bh_round_float(factor,1.0-(-(dot/distance))*(-1.0/330.0));
    }
}
static void stop(BHSoundQueue *q,BHSoundResource *r,unsigned long slot,BHSoundTickStats *s) {
    ++s->stop_calls;native_result(q,s,IDirectSoundBuffer_Stop(r->buffers[slot]));r->playing[slot]=0;
}
static void parameters(BHSoundQueue *q,BHSoundResource *r,BHSoundVoice *v,unsigned long slot,
    float master,BHSoundTickStats *s) {
    float attenuation,factor;double pan_value;long volume,pan,frequency;
    spatial(q,r,v,&attenuation,&pan_value,&factor);
    volume=clamp(truncate_low(((double)v->volume+attenuation)+master),-10000,0);
    if(r->streaming && volume< -3500) {if(r->playing[slot])stop(q,r,slot,s);return;}
    pan=clamp(truncate_low(pan_value),-10000,10000);
    frequency=truncate_low((double)v->frequency*factor);
    if(volume< -3500) {
        if(r->playing[slot])stop(q,r,slot,s);
        v->pending=0;return;
    }
    if(r->cached_volume[slot]!=volume) {
        ++s->volume_calls;native_result(q,s,IDirectSoundBuffer_SetVolume(r->buffers[slot],volume));r->cached_volume[slot]=volume;
    }
    if(r->cached_pan[slot]!=pan) {
        ++s->pan_calls;native_result(q,s,IDirectSoundBuffer_SetPan(r->buffers[slot],pan));r->cached_pan[slot]=pan;
    }
    if(r->cached_frequency[slot]!=frequency) {
        ++s->frequency_calls;native_result(q,s,IDirectSoundBuffer_SetFrequency(r->buffers[slot],(DWORD)frequency));r->cached_frequency[slot]=frequency;
    }
    if(r->streaming ? !r->playing[slot]:v->pending!=0) {
        if(!r->streaming)v->pending=0;
        ++s->position_calls;native_result(q,s,IDirectSoundBuffer_SetCurrentPosition(r->buffers[slot],0));
        ++s->play_calls;native_result(q,s,IDirectSoundBuffer_Play(r->buffers[slot],0,0,r->streaming ? DSBPLAY_LOOPING:0));r->playing[slot]=1;
    }
}
int __cdecl bh_sound_tick(BHSoundQueue *q,float dt,float master,BHSoundTickStats *out) {
    BHSoundTickStats stats;BHVec3 scratch={0,0,0};
    BHSoundResource *r;BHSoundVoice *v,*best[4],*swap;
    unsigned long i,j,k,n,walk;DWORD status;double phase;
    memset(&stats,0,sizeof(stats));
    if(!q || !_finite(dt) || !_finite(master) || !bh_math_environment_supported())return 0;
    /* Explicit source contexts reject malformed ownership instead of following
       unchecked original globals. Loaded metadata is supplied by the caller;
       the original name/options loader is a separate unfinished owner. */
    if((q->resource_count && !q->resources) || (q->voice_count && !q->voices)) {
        q->error=BH_SOUND_TOPOLOGY;return 0;
    }
    q->error=BH_SOUND_OK;++stats.clock_calls;GetTickCount();
    if(!q->listener_active) {if(out)*out=stats;return 1;}
    for(i=0;i<q->resource_count;++i) {
        r=q->resources[i];
        if(!r || r->buffer_count<1 || r->buffer_count>3 || r->voice_limit<1 || r->voice_limit>3) {q->error=BH_SOUND_TOPOLOGY;break;}
        for(j=0;j<r->buffer_count;++j)if(!r->buffers[j])break;
        if(j!=r->buffer_count) {q->error=BH_SOUND_TOPOLOGY;break;}
        if(!_finite(r->phase) || !_finite(r->time_scale) || !_finite(r->audible_range)) {
            q->error=BH_SOUND_NUMERICAL;break;
        }
        phase=(double)r->phase-dt;bh_round_float(&r->phase,phase);
        if(!(phase<0))continue;
        phase+=r->time_scale;bh_round_float(&r->phase,phase);if(phase<0)r->phase=0;
        if(r->streaming) {
            n=0;for(v=r->stream_head;v && n<r->voice_limit;v=v->next)++n;
            while(!r->no_more && r->buffer_count<n) {
                ++stats.duplicate_calls;
                if(!q->device) {q->error=BH_SOUND_TOPOLOGY;break;}
                q->last_hresult=IDirectSound_DuplicateSoundBuffer(q->device,r->buffers[0],r->buffers+r->buffer_count);
                if(q->last_hresult==DS_OK) {
                    ++r->buffer_count;if(r->buffer_count==r->voice_limit)r->no_more=1;
                } else {r->buffers[r->buffer_count]=NULL;r->no_more=1;}
            }
            if(n>r->buffer_count)n=r->buffer_count;
            memset(best,0,sizeof(best));walk=0;
            for(v=r->stream_head;v;v=v->next) {
                if(++walk>q->voice_count) {q->error=BH_SOUND_TOPOLOGY;break;}
                select_listener(q,v,&scratch);best[n]=v;
                for(k=n;k && (!best[k-1] || best[k]->distance_squared<best[k-1]->distance_squared);--k) {
                    swap=best[k-1];best[k-1]=best[k];best[k]=swap;
                }
            }
            if(walk>q->voice_count)break;
            for(j=0;j<n;++j)parameters(q,r,best[j],j,master,&stats);
            for(j=n;j<r->buffer_count;++j)if(r->playing[j])stop(q,r,j,&stats);
        } else {
            for(j=0;j<r->buffer_count;++j) {
                v=r->voices[j];
                if(r->playing[j] && v && !v->pending) {
                    status=0;++stats.status_calls;native_result(q,&stats,IDirectSoundBuffer_GetStatus(r->buffers[j],&status));
                    if(!(status&DSBSTATUS_PLAYING)) {
                        r->playing[j]=0;++v->generation;v->next=q->free_voices;q->free_voices=v;r->voices[j]=NULL;++stats.recycled;
                    }
                }
            }
            for(j=0;j<r->buffer_count;++j) {
                v=r->voices[j];if(!v)continue;
                if(v->pending && r->playing[j])stop(q,r,j,&stats);
                select_listener(q,v,&scratch);parameters(q,r,v,j,master,&stats);
            }
        }
    }
    ++stats.clock_calls;GetTickCount();if(out)*out=stats;return q->error==BH_SOUND_OK;
}
