#include "bh_portals.h"
#include <stddef.h>
#include <string.h>

typedef char portal_size_check[sizeof(BHPortal)==60 ? 1 : -1];
typedef char portal_neighbor_check[offsetof(BHPortal,neighbor)==56 ? 1 : -1];

static int finite_word(float value) {
    unsigned long bits;
    memcpy(&bits,&value,4);
    return (bits & 0x7f800000UL)!=0x7f800000UL;
}

static int valid_ring(BHPortalNode *node,unsigned long count) {
    BHPortalNode *current=node;
    unsigned long i,j,k;
    if (!node || !count || count>65535UL) return 0;
    for (i=0;i<count;++i) {
        if (!current || (i && current==node) || current->count>6) return 0;
        for (j=0;j<current->count;++j) {
            BHPortal *p=&current->portals[j];
            if (p->direction>6 || p->direction==3) return 0;
            for (k=0;k<4;++k)
                if (!finite_word(p->aperture[k]) || !finite_word(p->overlap[k]) ||
                    !finite_word(p->scratch[k])) return 0;
            if (p->aperture[0]>p->aperture[1] || p->aperture[2]>p->aperture[3] ||
                p->overlap[0]>p->overlap[1] || p->overlap[2]>p->overlap[3]) return 0;
        }
        current=current->next;
    }
    return current==node;
}

int __cdecl bh_portals_match(BHPortalNode *node,unsigned long ring_count) {
    unsigned long slot,other_slot,k;
    BHPortalNode *other;
    BHPortal *p,*q;
    if (!valid_ring(node,ring_count)) return 0;
    for (slot=0;slot<node->count;++slot) {
        p=&node->portals[slot];
        for (other=node->next;other!=node && !p->neighbor;other=other->next) {
            for (other_slot=0;other_slot<other->count && !p->neighbor;++other_slot) {
                q=&other->portals[other_slot];
                /* Original FSUB operands are double constants +0.1/-0.1.
                   Do not round the expanded/shrunken boundary to float. */
                if (q->direction!=(p->direction^4UL) || q->plane!=p->plane ||
                    !((double)q->aperture[0] < (double)p->aperture[1]-0.1) ||
                    !((double)p->aperture[0]+0.1 < (double)q->aperture[1]) ||
                    !((double)q->aperture[2] < (double)p->aperture[3]-0.1) ||
                    !((double)p->aperture[2]+0.1 < (double)q->aperture[3])) continue;
                p->neighbor=other;
                if (p->overlap[0]<q->aperture[0]) p->overlap[0]=q->aperture[0];
                if (q->aperture[1]<p->overlap[1]) p->overlap[1]=q->aperture[1];
                if (p->overlap[2]<q->aperture[2]) p->overlap[2]=q->aperture[2];
                if (q->aperture[3]<p->overlap[3]) p->overlap[3]=q->aperture[3];
                for (k=0;k<4;++k) p->scratch[k]=p->overlap[k];
            }
        }
    }
    return 1;
}
