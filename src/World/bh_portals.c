#include "bh_portals.h"
#include <stddef.h>
#include <string.h>

static_assert(sizeof(BHPortal) == 60, "BHPortal size");
static_assert(offsetof(BHPortal, neighbor) == 56, "BHPortal.neighbor offset");

static int finite_word(float value)
{
    unsigned long bits;
    memcpy(&bits, &value, 4);
    return (bits & 0x7f800000UL) != 0x7f800000UL;
}

static int valid_ring(BHPortalNode *node, unsigned long count)
{
    BHPortalNode *current = node;
    unsigned long i, j, k;
    if (!node || !count || count > 65535UL)
        return 0;
    for (i = 0; i < count; ++i)
    {
        if (!current || (i && current == node) || current->count > 6)
            return 0;
        for (j = 0; j < current->count; ++j)
        {
            BHPortal *portal = &current->portals[j];
            if (portal->direction > 6 || portal->direction == 3)
                return 0;
            for (k = 0; k < 4; ++k)
                if (!finite_word(portal->aperture[k]) || !finite_word(portal->overlap[k]) || !finite_word(portal->scratch[k]))
                    return 0;
            if (portal->aperture[0] > portal->aperture[1] || portal->aperture[2] > portal->aperture[3] || portal->overlap[0] > portal->overlap[1] ||
                portal->overlap[2] > portal->overlap[3])
                return 0;
        }
        current = current->next;
    }
    return current == node;
}

int bh_portals_match(BHPortalNode *node, unsigned long ring_count)
{
    unsigned long slot, other_slot, k;
    BHPortalNode *other;
    BHPortal *portal, *other_portal;
    if (!valid_ring(node, ring_count))
        return 0;
    for (slot = 0; slot < node->count; ++slot)
    {
        portal = &node->portals[slot];
        for (other = node->next; other != node && !portal->neighbor; other = other->next)
        {
            for (other_slot = 0; other_slot < other->count && !portal->neighbor; ++other_slot)
            {
                other_portal = &other->portals[other_slot];
                /* Original FSUB operands are double constants +0.1/-0.1.
                   Do not round the expanded/shrunken boundary to float. */
                if (other_portal->direction != (portal->direction ^ 4UL) || other_portal->plane != portal->plane ||
                    !((double)other_portal->aperture[0] < (double)portal->aperture[1] - 0.1) ||
                    !((double)portal->aperture[0] + 0.1 < (double)other_portal->aperture[1]) ||
                    !((double)other_portal->aperture[2] < (double)portal->aperture[3] - 0.1) ||
                    !((double)portal->aperture[2] + 0.1 < (double)other_portal->aperture[3]))
                    continue;
                portal->neighbor = other;
                if (portal->overlap[0] < other_portal->aperture[0])
                    portal->overlap[0] = other_portal->aperture[0];
                if (other_portal->aperture[1] < portal->overlap[1])
                    portal->overlap[1] = other_portal->aperture[1];
                if (portal->overlap[2] < other_portal->aperture[2])
                    portal->overlap[2] = other_portal->aperture[2];
                if (other_portal->aperture[3] < portal->overlap[3])
                    portal->overlap[3] = other_portal->aperture[3];
                for (k = 0; k < 4; ++k)
                    portal->scratch[k] = portal->overlap[k];
            }
        }
    }
    return 1;
}
