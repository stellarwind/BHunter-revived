#ifndef BH_EDITOR_CONTROL_H
#define BH_EDITOR_CONTROL_H
#include "bh_editor_world.h"
typedef struct BHEditorControl {
 BHEditorWorld *world;
 unsigned long select_sound,step_limit,steps;
 int error;
} BHEditorControl;
/* Whole0040e420 and00410080. Existing decoded input counters are borrowed:
   packed signed16 axes are index8; each transition is consumed at its original
   branch position. Source fixtures supply data, never poll or inject OS input.
   Real world editing/geometry, clone, numeric entry, source queue/sound and
   actual callbacks implement the complete editor behaviors. Clipboard entries
   are caller-owned snapshots(max50), never live or executable entity objects.
   Context private payload+556; node binding remains+560. Coherent reciprocal
   ring/model/entity source components required. PC53 finite moderate state;
   loop budget bounds malformed/extreme movement after earlier edits.
   Deleting the last editable node retains its actual entity as the original
   empty-editor sentinel. Enter installs real callbacks without inventing a
   selection; drawing with no selection reports a bounded source error. */
int __cdecl bh_editor_control_bind(BHEditorControl *,BHEditorNode *);
void __cdecl bh_editor_tick(BHEntity *); /*0040e420*/
void __cdecl bh_editor_enter(BHEntity *); /*00410080*/
#endif
