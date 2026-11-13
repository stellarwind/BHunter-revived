#include "bh_impact_ring.h"
#include "bh_round.h"
#include <stddef.h>
#include <math.h>
#include <float.h>

static_assert(offsetof(BHEntity, payload) + 4 + offsetof(BHImpactState, age) == 0x100, "ring age layout");
static_assert(offsetof(BHEntity, payload) + 4 + offsetof(BHImpactState, normal) == 0x114, "ring normal layout");
int bh_impact_ring_draw(BHDrawQueue *queue, unsigned long texture, const BHEntity *entity, const float *camera_yaw)
{
    const BHImpactState *state;
    BHVec3 up = {0, 0, 1};
    float size, alpha;
    if (!queue || !entity)
        return -1;
    state = (const BHImpactState *)(entity->payload + 4);
    if (!_finite(state->age) || !_finite(state->duration) || state->duration == 0 || !_finite(state->size) ||
        (camera_yaw && !_finite(*camera_yaw)))
        return -1;
    if (camera_yaw)
    {
        bh_round_float(&up.x, sin((double)*camera_yaw));
        bh_round_float(&up.z, cos((double)*camera_yaw));
    }
    bh_round_float(&size, (double)state->size * state->age);
    bh_round_float(&alpha, (1.0 - (double)state->age / state->duration) * .7);
    return bh_draw_oriented(queue, 3, texture, &entity->position, &up, &state->normal, size, size, 0, 0, alpha);
}
