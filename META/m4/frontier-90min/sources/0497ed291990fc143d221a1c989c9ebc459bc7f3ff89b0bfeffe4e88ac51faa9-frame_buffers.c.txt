#include "../src/bh_frame_buffers.h"
#include "../src/bh_draw_queue.h"
#include "../src/bh_input_messages.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<14)printf("FAIL frame buffers %d: %s\n",__LINE__,s);}}while(0)
typedef struct Bridge {BHFrameBuffers *frames;BHDrawQueue queue;BHInputSnapshot snapshot;void *lease;unsigned long produced,released;} Bridge;
static void __cdecl produce(void *user){Bridge *b=(Bridge *)user;BHVec3 point={1,2,3};CHECK(bh_draw_billboard(&b->queue,0,17,&point,2,4,0,0,1)==1,"real source frame callback submits actual draw command");++b->produced;}
static LRESULT __cdecl message(void *user,HWND window,UINT code,WPARAM key,LPARAM bits){Bridge *b=(Bridge *)user;return bh_input_message(&b->snapshot,window,code,key,bits);}
static void *__cdecl lease(unsigned long bytes,void *user){Bridge *b=(Bridge *)user;if(bytes>b->frames->bytes)return NULL;b->lease=bh_frame_buffers_acquire(b->frames);return b->lease;}
static void __cdecl publish(void *memory,void *user){Bridge *b=(Bridge *)user;CHECK(memory==b->lease && bh_frame_buffers_publish(b->frames),"real draw-owner disposal publishes borrowed frame lease");++b->released;b->lease=NULL;}
static void composition(void){BHFrameBuffers f;Bridge bridge;BHSceneAllocator allocator;unsigned long i;long index;void *p;memset(&bridge,0,sizeof(bridge));bridge.frames=&f;
    CHECK(bh_frame_buffers_bind(&f,NULL),"own real Win32 mutex");CHECK(bh_frame_buffers_start(&f,produce,&bridge,30,3500*84,message,&bridge),"original30Hz/3500command frame initialization");
    CHECK(fabs(f.delay_ms-(1000.0/30.0-1.5))<1e-12,"original stored scheduling delay");
    for(i=0;i<3;++i)CHECK(f.slots[i].data && !f.slots[i].busy && f.slots[i].age==3,"three actual initial allocations and age state");
    CHECK(!f.message(f.message_user,NULL,WM_KEYDOWN,'A',0) && bridge.snapshot.current['A']==1,"stored actual message callback reaches restored input owner");
    allocator.allocate=lease;allocator.release=publish;allocator.user=&bridge;
    for(i=0;i<15;++i){CHECK(bh_draw_queue_init(&bridge.queue,16,&allocator),"real draw queue uses acquired render buffer allocation");index=f.active;
        CHECK(index==(long)(i%3) && f.slots[index].busy==1 && f.slots[index].age==-1 && f.writing,"oldest age selection, initial ties and active writer marking");
        f.produce(f.produce_user);CHECK(bridge.queue.count==10 && bridge.queue.commands[9].kind==BH_DRAW_BILLBOARD,"draw command is stored in selected real frame memory");
        CHECK(bh_draw_queue_dispose(&bridge.queue) && f.latest==index && f.active==index && !f.writing && !f.slots[index].busy && f.slots[index].age==0,"publication clears busy, increments age and retains active pointer");
    }
    CHECK(bridge.produced==15 && bridge.released==15,"callbacks/leases compose through actual owners for repeated frames");
    p=bh_frame_buffers_acquire(&f);CHECK(p!=NULL && !bh_frame_buffers_dispose(&f) && f.error==BH_FRAME_BUFFER_BUSY,"outstanding writer blocks lifetime disposal");
    CHECK(!bh_frame_buffers_acquire(&f) && f.error==BH_FRAME_BUFFER_BUSY,"nested producer rejected explicitly");CHECK(bh_frame_buffers_publish(&f),"finish live writer");
    f.slots[0].busy=f.slots[1].busy=f.slots[2].busy=1;CHECK(!bh_frame_buffers_acquire(&f) && f.error==BH_FRAME_BUFFER_EMPTY,"all-unavailable saved-ECX fallback guarded without foreign memory write");
    f.slots[0].busy=f.slots[1].busy=f.slots[2].busy=0;f.slots[0].age=-2;f.slots[1].age=0;f.slots[2].age=0;
    CHECK(bh_frame_buffers_acquire(&f)==f.slots[1].data && f.active==1,"signed ages and earliest equal priority");f.slots[0].age=0x7fffffffL;
    CHECK(bh_frame_buffers_publish(&f) && f.slots[0].age==(-2147483647L-1L),"all age increments preserve original32bit wrap");
    CHECK(bh_frame_buffers_dispose(&f) && !f.mutex && !f.slots[0].data && !f.slots[1].data && !f.slots[2].data,"all real buffers/mutex cleaned");
}
typedef struct Hold {HANDLE mutex,ready;} Hold;
static DWORD WINAPI hold(void *user){Hold *h=(Hold *)user;DWORD result=WaitForSingleObject(h->mutex,1000);if(result==WAIT_OBJECT_0){SetEvent(h->ready);Sleep(60);ReleaseMutex(h->mutex);}return result;}
static void native_wait(void){BHFrameBuffers f;Bridge b;Hold h;HANDLE thread;DWORD exit_code,ready;memset(&b,0,sizeof(b));b.frames=&f;
    CHECK(bh_frame_buffers_bind(&f,NULL) && bh_frame_buffers_start(&f,produce,&b,500,16*84,message,&b),"real mutex wait fixture initialization");CHECK(f.delay_ms==1,"high frame rate lower-clamps interval to1ms");
    h.mutex=f.mutex;h.ready=CreateEventA(NULL,TRUE,FALSE,NULL);thread=CreateThread(NULL,0,hold,&h,0,NULL);
    CHECK(h.ready && thread,"real isolated contention thread and event");
    if(h.ready && thread){ready=WaitForSingleObject(h.ready,1000);CHECK(ready==WAIT_OBJECT_0,"contention holder really owns mutex before acquire");
        if(ready==WAIT_OBJECT_0){CHECK(bh_frame_buffers_acquire(&f)!=NULL && f.timeouts>0 && f.wait_calls>1,"real5ms timeout loop waits until native mutex holder releases");CHECK(bh_frame_buffers_publish(&f),"publish after real synchronization");}}
    if(thread){CHECK(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0 && GetExitCodeThread(thread,&exit_code) && exit_code==WAIT_OBJECT_0,"contention thread exits naturally");CloseHandle(thread);}if(h.ready)CloseHandle(h.ready);
    CHECK(bh_frame_buffers_dispose(&f),"contention context cleanup");
}
typedef struct Allocation {unsigned long calls,freed,live,deny;} Allocation;
static void *__cdecl allocate(unsigned long bytes,void *user){Allocation *a=(Allocation *)user;void *p;++a->calls;if(a->calls==a->deny)return NULL;p=malloc(bytes);if(p)++a->live;return p;}
static void __cdecl release(void *p,void *user){Allocation *a=(Allocation *)user;if(p){--a->live;++a->freed;free(p);}}
static void rollback(void){BHFrameBuffers f;Bridge b;Allocation a;BHSceneAllocator allocator;unsigned long n;memset(&b,0,sizeof(b));
    for(n=1;n<=3;++n){memset(&a,0,sizeof(a));a.deny=n;allocator.allocate=allocate;allocator.release=release;allocator.user=&a;
        CHECK(bh_frame_buffers_bind(&f,&allocator),"real mutex with explicit failing owning allocator");
        CHECK(!bh_frame_buffers_start(&f,produce,&b,30,84,message,&b) && f.error==BH_FRAME_BUFFER_MEMORY && !a.live && a.freed==n-1 && !f.bytes,"all partial allocation prefixes roll back without callback/buffer publication");
        CHECK(bh_frame_buffers_dispose(&f),"failed-start mutex cleanup");}
}
int bh_frame_buffers_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;composition();native_wait();rollback();
    CHECK(bh_math_environment_supported(),"frame handoff retainsPC53");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"frame-buffers\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
