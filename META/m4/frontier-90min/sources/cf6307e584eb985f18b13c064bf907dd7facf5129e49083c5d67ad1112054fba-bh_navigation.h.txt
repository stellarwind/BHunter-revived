#ifndef BH_NAVIGATION_H
#define BH_NAVIGATION_H
#include "bh_world_geometry.h"

typedef struct BHNavigationNode {
    BHWorldGeometry *placement;
    long junction_first_port,junction_second_port; /* original +10c/+110 */
    unsigned long endpoints_valid; /* source metadata: bits 1/2 indicate writes */
} BHNavigationNode;
typedef enum BHNavigationError {
    BH_NAV_OK,BH_NAV_ARGUMENT,BH_NAV_LIMIT,BH_NAV_MEMORY,BH_NAV_TOPOLOGY,
    BH_NAV_CYCLE,BH_NAV_NUMERICAL,BH_NAV_CACHE
} BHNavigationError;
typedef struct BHNavigation {
    BHNavigationNode *nodes; /* junctions first; corridors follow in scene order */
    unsigned long node_count,junction_count;
    BHSceneRoutes routes; /* destination-major local exit slots, not node IDs */
    BHSceneAllocator allocator;
    BHNavigationError error;
    int cache_used;
} BHNavigation;

/* Explicit source lifecycle; initialize fresh, dispose before reinitialization.
   Topology is borrowed; retain placements and portal targets until graph disposal.
   Nodes/routing cells/allocator ownership must not be changed by the caller. */
int __cdecl bh_navigation_init(BHNavigation *navigation,const BHSceneAllocator *allocator);
void __cdecl bh_navigation_dispose(BHNavigation *navigation);
/* Complete 00407f30 behavior over a bounded live scene ring, with 00407d90
   corridor behavior. No gameplay executes. At most 65535 scene nodes/2048
   junctions. Degree counts all six slots and excludes placements with an owner.
   Existing graph survives failure. Foreign/unindexed neighbors, malformed ring,
   nonfinite bounds and corridor cycles reject explicitly. Pure degree-two rings
   have nodes but no junction matrix/endpoints, as in the original.
   Optional borrowed cache: matching count uses the bounded existing PTH reader;
   a mismatching count consumes only its header and recomputes. Truncation or
   malformed matching cache is an error, not fake success; failure attempts cursor
   rollback. The cache is never closed. The original relaxation order and double
   comparison before float stores are preserved; no symmetry/diagonal repair. */
int __cdecl bh_navigation_build(BHNavigation *navigation,BHWorldGeometry *first,
                                unsigned long scene_count,BHVFile *cache);
long __cdecl bh_navigation_find(const BHNavigation *navigation,const BHWorldGeometry *placement);
#endif
