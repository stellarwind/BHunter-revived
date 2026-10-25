#ifndef BH_WORLD_GEOMETRY_H
#define BH_WORLD_GEOMETRY_H
#include "bh_geometry.h"
#include "bh_portals.h"

typedef struct BHGeometryModel {
    BHGeometry *geometry;
    unsigned long portal_count;
    long portal_faces[6];
    long grid;
    unsigned long flags;
    BHVec3 minimum,maximum;
    BHBspError error;
} BHGeometryModel;

typedef struct BHWorldGeometry {
    BHPortalNode navigation; /* first: source scene ring/portal ownership */
    struct BHWorldGeometry *previous;
    const BHGeometryModel *model;
    BHVec3 position;
    unsigned long flags;
    BHMat34 transform,inverse;
    BHVec3 minimum,maximum;
    struct BHWorldGeometry *owner;
    unsigned long query_flags; /* caller's model/script +114 flags, not orientation */
    unsigned long radar_visited; /* source field for original +fc traversal mark */
} BHWorldGeometry;

/* Bind a fresh source model to live immutable geometry; real material callbacks
   recover 004083d0 model bounds and 00410880 portal/roof/water/scrap flags.
   grid and initial_flags must come from the caller's model definition, including
   any applied script settings. This does not claim full block-script recovery.
   Portal list is in original face order. >6 portals or nonpositive grid rejects
   before touching geometry flags. Own/dispose the geometry separately. */
int __cdecl bh_geometry_model_init(BHGeometryModel *model,BHGeometry *geometry,
                                   long grid,unsigned long initial_flags);
/* 004084a0 and its 004082d0 bound callback. Inputs are a zero-initialized or
   previously built component with a live model, position/flags, next and previous
   scene links. Construction preserves those inputs and replaces output only on
   success. The scene ring must contain exactly ring_count accessible members,
   at most 65535, with reciprocal previous/next links. Models/geometry remain
   borrowed and immutable except their real owned face flags. No simulation or
   script/entity callbacks are installed. After constructing every member, run
   bh_portals_match on each navigation component to connect the scene. */
int __cdecl bh_world_geometry_build(BHWorldGeometry *placement,unsigned long ring_count);
#endif
