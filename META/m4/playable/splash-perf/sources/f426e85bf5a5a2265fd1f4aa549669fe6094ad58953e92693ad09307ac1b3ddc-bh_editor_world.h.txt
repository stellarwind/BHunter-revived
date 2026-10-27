#ifndef BH_EDITOR_WORLD_H
#define BH_EDITOR_WORLD_H
#include "bh_world_reset.h"
#include "bh_editor_input.h"
#include "bh_editor_numeric.h"
typedef struct BHEditorWorld BHEditorWorld;
typedef struct BHEditorNode {
 BHWorldEditPlacement placement; /* first: real shared geometry/edit component */
 BHEntity *entity;BHEditorWorld *context;
 unsigned long model_index,heap_owned;
} BHEditorNode;
typedef struct BHEditorClipboard {
 BHEditorNode node;BHEntity entity;
} BHEditorClipboard;
struct BHEditorWorld {
 BHEntityPool *pool;BHScriptBlocks *library;BHDrawQueue *queue;BHSoundQueue *sound;
 BHEditorNode **nodes;unsigned long count,capacity;
 BHEditorNode *selection;BHScriptBrowser *browser;BHPlayerInputState *input;
 BHEditorNumericContext *numeric;
 long mode,catalog_open,axes_enabled,clipboard_count;
 BHEditorClipboard *clipboard;unsigned long clipboard_capacity;
 BHEditorNode *clipboard_origin;
 BHVec3 camera_offset;
 unsigned long line_image,selection_image,horizon_image;
 int error;
};
/* Count-zero explicit source ownership binding. Every semantic scene member
   must start with this node's real geometry/edit component; entity/model/ring
   storage remains borrowed. The actual refresh bridge invokes the full draw
   below. Private payload+560 is not an original executable global or vtable. */
int __cdecl bh_editor_world_attach(BHEditorWorld *,BHEditorNode *);
BHEditorNode *__cdecl bh_editor_world_node(BHEntity *);
BHEntityCallback __cdecl bh_editor_world_draw_callback(void);
/* Whole0040d140. Actual queue producers for model/camera/horizon/axes, linked
   owner line, text, portal borders and overlap/scratch rectangles. Mode/catalog
   and distance gates remain original. No renderer flush, screen or input use. */
int __cdecl bh_editor_world_draw(BHEditorWorld *,BHEditorNode *);
/* Whole0040e070. Insert after, copy prototype fields/8 variables and orbit,
   add modulo32 deltas, grid-truncate, install actual refresh and NULL RET step.
   Prototype may be live or a registered copied clipboard record; copied entity
   callback/link bytes remain data and are never installed or executed.
   Geometry rebuild remains the caller's separate real original owner. */
BHEditorNode *__cdecl bh_editor_world_clone(BHEditorWorld *,long,long,long,BHEditorNode *after,const BHEditorNode *prototype);
/* Count-zero owner cleanup. Detach callback owners before freeing metadata;
   borrowed nodes are not freed. Context must be quiescent. */
int __cdecl bh_editor_world_dispose(BHEditorWorld *);
#endif
