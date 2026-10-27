#ifndef BH_SCRIPT_BLOCKS_H
#define BH_SCRIPT_BLOCKS_H
#include "bh_script_parse.h"
#include "bh_render_resources.h"
#include "bh_collision_model.h"
#include "bh_world_geometry.h"
typedef struct BHScriptBlock {
 BHScriptDefinition definition;unsigned long resource;
 BHCollisionModel collision;BHGeometryModel model;
 float preview_yaw,preview_phase;
} BHScriptBlock;
typedef struct BHScriptBlocks {
 BHScriptBlock *blocks;unsigned long count,capacity;
 BHScriptParse *parser;BHRenderResources *resources;
 const char *collision_prefix;BHBspSearch collision_search;
 long columns,rows;BHTextureImage *water;int error;
} BHScriptBlocks;
/* Count-zero owner adapters. Release scripts through their real pool owners
   separately; render descriptors/textures belong to the supplied registry. */
int __cdecl bh_script_blocks_init(BHScriptBlocks *,BHScriptParse *,BHRenderResources *,const char *,BHBspSearch);
void __cdecl bh_script_blocks_dispose(BHScriptBlocks *);
/* Whole00411f00: original empty-registry gate, real VFILE line list, sidecar
   templates, loaded rendering model + independent collision model/material
   bounds/portal flags, count-on-render-success, square-root page dimensions,
   water image flags. Resources and partial scripts retain earlier effects if
   a later model fails. No scenes, gameplay or original code are loaded. */
int __cdecl bh_script_blocks_load(BHScriptBlocks *,const char *list);
typedef struct BHScriptBrowser {
 BHScriptBlocks *library;BHDrawQueue *queue;BHSoundQueue *sound;
 long height,selected,active;
 float cursor_x,cursor_y,offset_x,offset_y;
 unsigned long grid_image,selection_image,select_sound,cells;int error;
} BHScriptBrowser;
/* Whole0040c660 /0040c9b0, source editor catalogue preview producers. Queue
   camera/labels/resources/quads and deferred sound only; no screen/input/game
   execution. Animation uses original wrapping private arithmetic, independent
   of shared gameplay rand. Source counters are diagnostics, count zero.
   Library count<=600, columns>=5, rows>=0, finite cursor/angles/bounds and
   height1..16384; original small-row scroll behavior and signed truncation
   retained. Caller drains real queue before changing/discarding resources. */
int __cdecl bh_script_block_preview(BHScriptBrowser *,long index,long x,long y);
int __cdecl bh_script_block_browser(BHScriptBrowser *);
#endif
