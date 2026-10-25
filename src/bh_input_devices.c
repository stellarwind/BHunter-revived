#include "bh_input_devices.h"
#include "bh_input_key_names.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
typedef char joystick_caps_size[sizeof(JOYCAPSA)==0x194?1:-1];
typedef char joystick_caps_flags[offsetof(JOYCAPSA,wCaps)==0x60?1:-1];
static void *__cdecl allocate(unsigned long bytes,void *user){return malloc(bytes);}
static void __cdecl release(void *memory,void *user){free(memory);}
int __cdecl bh_input_devices_bind(BHInputDevices *d,BHInputCollection *c,const BHSceneAllocator *a){
    unsigned long i;if(!d || !c || !c->snapshot || !c->lists || (a && (!a->allocate || !a->release)))return 0;
    memset(d,0,sizeof(*d));d->collection=c;
    for(i=0;i<255;++i)d->names[i]=bh_input_key_defaults[i];
    d->api.map_key=MapVirtualKeyA;d->api.capture=joySetCapture;d->api.capabilities=joyGetDevCapsA;d->api.release=joyReleaseCapture;
    if(a)d->allocator=*a;else{d->allocator.allocate=allocate;d->allocator.release=release;}
    return 1;
}
int __cdecl bh_input_devices_start(BHInputDevices *d,HWND window){
    BHInputCollection *c;BHInputSnapshot *s;unsigned long i,character,bytes;char *name;int replace;
    if(!d || !(c=d->collection) || !(s=c->snapshot) || !c->lists || c->lists->first || !d->api.map_key || !d->api.capture ||
       !d->api.capabilities || !c->api.recenter || !d->allocator.allocate || !d->allocator.release)return 0;
    d->allocation_failures=0;
    for(i=0;i<255;++i){character=d->api.map_key((UINT)i,2)&255;
        replace=character!=i && i>=65 && i<=90 && character>=65 && character<=90;
        if(replace || !d->names[i]){
            if(d->allocated[i])d->allocator.release((void *)d->names[i],d->allocator.user);
            d->allocated[i]=0;bytes=replace || character>=32?8:10;
            name=(char *)d->allocator.allocate(bytes,d->allocator.user);d->names[i]=name;
            if(!name){++d->allocation_failures;continue;}
            d->allocated[i]=1;
            if(replace || character>=32)sprintf(name,"Key '%c'",(int)character);else sprintf(name,"Key #%d",(int)i);
        }
    }
    c->enabled[0]=c->enabled[1]=0;s->history_head=0;memset(s->history,0,32);memset(s->current,0,255);memset(s->previous,0,255);
    for(i=0;i<2;++i){d->capture_result[i]=d->api.capture(window,(UINT)i,100,FALSE);
        if(d->capture_result[i]==JOYERR_NOERROR){d->caps_result[i]=d->api.capabilities((UINT)i,d->caps+i,0x194);c->capabilities[i]=d->caps[i].wCaps;c->enabled[i]=1;}}
    memset(s->axes,0,sizeof(s->axes));c->last_recenter=c->api.recenter(160,100);
    c->delta[0]=c->delta[1]=0;s->pair4[0]=s->pair4[1]=0;s->pair3[0]=s->pair3[1]=512;s->mouse_axes[0]=s->mouse_axes[1]=0;
    c->deadzone_mask=0xffffffffUL;c->calibrated[0]=c->calibrated[1]=0;
    for(i=0;i<8;++i){c->minimum[i]=1;c->maximum[i]=-1;}d->started=1;return !d->allocation_failures;
}
int __cdecl bh_input_devices_stop(BHInputDevices *d){unsigned long i;int ok=1;BHInputCollection *c;
    if(!d || !(c=d->collection) || !d->api.release)return 0;
    for(i=0;i<2;++i)if(c->enabled[i]){d->release_result[i]=d->api.release((UINT)i);if(d->release_result[i]!=JOYERR_NOERROR)ok=0;}
    c->enabled[0]=c->enabled[1]=0;d->started=0;return ok;
}
void __cdecl bh_input_devices_dispose(BHInputDevices *d){unsigned long i;if(!d)return;
    bh_input_devices_stop(d);
    if(d->allocator.release)for(i=0;i<255;++i)if(d->allocated[i])d->allocator.release((void *)d->names[i],d->allocator.user);
    for(i=0;i<255;++i){d->allocated[i]=0;d->names[i]=bh_input_key_defaults[i];}
}
