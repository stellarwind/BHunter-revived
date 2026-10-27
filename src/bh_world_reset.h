#ifndef BH_WORLD_RESET_H
#define BH_WORLD_RESET_H
#include "bh_world_edit.h"
#include "bh_script_blocks.h"
typedef struct BHWorldResetNode
{
    BHEntity *entity;
    BHWorldEditPlacement *placement;
    const BHScriptDefinition *definition;
    BHVec3 *draw_angles; /* Optional borrowed renderer copy of entity +3c. */
} BHWorldResetNode;
typedef struct BHWorldReset
{
    BHScriptVMContext *vm;
    BHWorldResetNode *nodes;
    unsigned long count;
    unsigned long (*loot_rows)[32];
    unsigned long timers, immediate;
    int error;
} BHWorldReset;
/* Whole00410590. Caller supplies actual live source entities and the same
   reciprocal semantic geometry ring in traversal order; bindings outlive all
   spawned real timers. Resets transforms, bounds by old-minus-new POSITION
   only (does not rotate/rebuild portals), damage attribution, origin, flags,
   priority and 4x32 loot words. All script trees/callbacks are actual source.
   Existing script variables, intrusive actor lists and portal extents retained.
   Only a leading VM location(opcode20) executes its timer immediately.
   Bounds X/Y displacement stores binary32; Z retains its PC53 difference.
   Source failure preserves preceding changes, just as the original sequence.
   No editor callback, world loader or game loop is installed here. */
int __cdecl bh_world_reset(BHWorldReset *);
#endif
