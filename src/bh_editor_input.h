#ifndef BH_EDITOR_INPUT_H
#define BH_EDITOR_INPUT_H
#include "bh_player_input.h"
#include "bh_entities.h"
/* Explicit bindings for original editor globals004cda1c/00496168/004dfb5c.
   This is editor input, not player setup. No polling/cursor/device API calls.
   Zero list/dispatcher before setup. owner/pool/state and this context outlive
   the dispatcher. Release dispatcher before disposing the list owner/context.
   list may be rebound to another live packet list; tick reloads it each time. */
typedef struct BHEditorInput
{
    BHPlayerInputOwner *owner;
    BHEntityPool *pool;
    BHPlayerInputList *list;
    BHPlayerInputState *state;
    BHEntity *dispatcher;
    const char *name;
} BHEditorInput;
/* Whole00412190. Source returns1 once the list exists, even if entity allocation
   fails: original already-installed bindings remain. Invalid/repeated setup or
   list-allocation failure returns0. Original mode1,29 slots, kind4 at8 retained. */
int __cdecl bh_editor_input_setup(BHEditorInput *context);
/* Whole004100b0 with source-private context binding in the actual pooled entity.
   A valid encoded list->state packet and disjoint destination are required. */
void __cdecl bh_editor_input_tick(BHEntity *entity);
#endif
