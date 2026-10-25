#ifndef BH_PORTALS_H
#define BH_PORTALS_H

typedef struct BHPortalNode BHPortalNode;
/* Original 60-byte portal record. Plane is an integer, not a float. */
typedef struct BHPortal {
    unsigned long direction;
    long plane;
    float aperture[4]; /* u_min, u_max, v_min, v_max */
    float overlap[4];
    float scratch[4];
    BHPortalNode *neighbor;
} BHPortal;

/* Source-owned navigation component. The caller supplies the scene ring in
   original +124 traversal order and the model's resolved portal count. This
   is not a declaration of the original full entity or model registry ABI. */
struct BHPortalNode {
    BHPortalNode *next;
    unsigned long count;
    BHPortal portals[6];
};

/* Complete finite-input behavior of 00408940. Match only this node's vacant
   portals; do not force reciprocal links. First matching node/slot wins.
   Existing links/overlap remain untouched. On entry, unlinked overlap must
   already contain the owning construction pass's aperture extents.
   Ring members and their borrowed neighbor pointers must stay alive; no
   allocation or ownership transfer occurs. No concurrent mutation is allowed.
   The bounded source interface validates exactly ring_count members and all
   finite ordered extents before writing. Rejection leaves the ring unchanged.
   Limits: 65535 nodes, six portals each. Requires the project's PC53 profile. */
int __cdecl bh_portals_match(BHPortalNode *node,unsigned long ring_count);
#endif
