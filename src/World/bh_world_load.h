#ifndef BH_WORLD_LOAD_H
#define BH_WORLD_LOAD_H
#include "../Game/Editor/bh_editor_control.h"
#include "../Game/Scripts/bh_script_vm.h"
#include "bh_rail_paths.h"
typedef struct BHWorldLoad
{
    BHEditorControl *editor;
    BHScriptVMContext *vm;
    BHRailPaths *rails;
    BHScriptTimerActor *states;
    unsigned long actor_capacity;
    unsigned long skipped, overrides, timers, immediate;
    int error;
    BHBindingIndex actor_index;
    BHSceneAllocator index_allocator;
} BHWorldLoad;
/* Whole004100d0. Fresh empty editor registry and explicit stable actor tables;
   vm->actors and states have actor_capacity records; both VM and timer counts
   initially zero. Existing resource/VM damage contexts in each record retained.
   Nodes/real entities remain owned by editor_world_dispose; caller keeps typed
   actor arrays, libraries, VM and rail contexts alive until all timers retire.
   VFILE records retain original IDs including skipped unknown model names.
   Saved overrides precede startup scripts, geometry build then portal match.
   Partial effects retained on resource failure; source closes VFILE on every
   exit, bounds malformed records and rejects original all-unknown NULL crash.
   No original code loading or gameplay execution. PC53 finite model state. */
int bh_world_load(BHWorldLoad *, const char *name);
/* Release the optional actor identity workspace before retiring its context. */
void bh_world_load_dispose_index(BHWorldLoad *);
#endif
