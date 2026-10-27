#ifndef BH_FRAME_BUFFERS_H
#define BH_FRAME_BUFFERS_H
#include <windows.h>
#include "bh_scene.h"
typedef void(__cdecl *BHFrameProduce)(void *);
typedef LRESULT(__cdecl *BHFrameMessage)(void *, HWND, UINT, WPARAM, LPARAM);
typedef struct BHFrameSlot
{
    void *data;
    long busy, age;
} BHFrameSlot;
typedef enum BHFrameBufferError
{
    BH_FRAME_BUFFER_OK,
    BH_FRAME_BUFFER_ARGUMENT,
    BH_FRAME_BUFFER_MEMORY,
    BH_FRAME_BUFFER_WAIT,
    BH_FRAME_BUFFER_BUSY,
    BH_FRAME_BUFFER_EMPTY
} BHFrameBufferError;
typedef struct BHFrameBuffers
{
    BHFrameSlot slots[3];
    HANDLE mutex;
    BHSceneAllocator allocator;
    unsigned long bytes, wait_calls, timeouts;
    DWORD last_wait;
    BOOL last_release;
    long active, latest;
    int writing;
    double delay_ms;
    BHFrameProduce produce;
    void *produce_user;
    BHFrameMessage message;
    void *message_user;
    BHFrameBufferError error;
} BHFrameBuffers;
/* Explicit lifetime adapter owns a real Win32 mutex and optional allocator.
   Fresh owner; dispose before rebinding. No worker thread/window is created. */
int __cdecl bh_frame_buffers_bind(BHFrameBuffers *, const BHSceneAllocator *);
/* Complete00471c00: install borrowed callbacks, store1000/fps-1.5 delay with
   lower clamp1, allocate THREE buffers with busy0/age3. Original startup uses
   30Hz and3500*84bytes. Source rollback on allocation failure is an ownership
   guard; original failure paths could publish NULL buffers. No worker or
   callback body is supplied/recovered by this initializer. */
int __cdecl bh_frame_buffers_start(BHFrameBuffers *, BHFrameProduce, void *, double fps, unsigned long bytes,
                                   BHFrameMessage, void *);
/* Complete00471cc0: real mutex wait5ms, repeat on timeout; select oldest signed
   age strictly>-1 among busy0, earliest ties, then busy1/age-1 and active slot.
   Returned buffer is borrowed until publish. Source guards reject recursive
   acquire/no eligible slot/wait failure instead of saved-ECX invalid fallback. */
void *__cdecl bh_frame_buffers_acquire(BHFrameBuffers *);
/* Complete00471d70: increment ALL ages with32bit wrap, clear active busy, set
   latest=active, release mutex. Active pointer/index remains as original.
   Source writing flag guards an unmatched/duplicate publication. */
int __cdecl bh_frame_buffers_publish(BHFrameBuffers *);
/* Source lifetime adapter; rejects an outstanding writer, releases only owned
   allocations/mutex. Caller must stop external consumers before disposing. */
int __cdecl bh_frame_buffers_dispose(BHFrameBuffers *);
#endif
