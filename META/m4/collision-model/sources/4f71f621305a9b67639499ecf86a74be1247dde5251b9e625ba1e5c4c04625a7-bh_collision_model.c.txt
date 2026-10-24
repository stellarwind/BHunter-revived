#include "bh_collision_model.h"
#include <string.h>

int __cdecl bh_collision_model_init(BHCollisionModel *model, const BHSceneAllocator *allocator) {
    BHCollisionModel next;
    if (!model)
        return 0;
    memset(&next, 0, sizeof(next));
    if (!bh_bsp_init(&next.bsp, allocator))
        return 0;
    *model = next;
    return 1;
}
int __cdecl bh_collision_model_dispose(BHCollisionModel *model) {
    if (!model || model->geometry.enumerating)
        return 0;
    if (model->geometry.bsp)
        bh_geometry_dispose(&model->geometry);
    bh_bsp_dispose(&model->bsp);
    return 1;
}
static int prepare(BHCollisionModel *model) {
    BHBspFace *faces = (BHBspFace *)model->bsp.faces;
    unsigned long i;
    if (!bh_geometry_init(&model->geometry, &model->bsp, &model->bsp.allocator)) {
        model->bsp.error = BH_BSP_MEMORY;
        return 0;
    }
    /* These bytes belong to the newly decoded owner, never to caller input. */
    for (i = 0; i < model->bsp.counts[3]; ++i)
        faces[i].runtime = 0;
    return 1;
}
static int commit(BHCollisionModel *model, BHCollisionModel *next, int ok) {
    BHBspError error;
    if (ok)
        ok = prepare(next);
    if (!ok) {
        error = next->bsp.error;
        bh_collision_model_dispose(next);
        model->bsp.error = error;
        return 0;
    }
    bh_collision_model_dispose(model);
    *model = *next;
    model->geometry.bsp = &model->bsp;
    return 1;
}
int __cdecl bh_collision_model_decode(BHCollisionModel *model, const void *bytes,
                                      unsigned long size) {
    BHCollisionModel next;
    int ok;
    if (!model || model->geometry.enumerating ||
        !bh_collision_model_init(&next, &model->bsp.allocator))
        return 0;
    ok = bh_bsp_decode(&next.bsp, bytes, size);
    return commit(model, &next, ok);
}
int __cdecl bh_collision_model_load(BHCollisionModel *model, const char *name, const char *prefix,
                                    BHBspSearch search) {
    BHCollisionModel next;
    int ok;
    if (!model || model->geometry.enumerating ||
        !bh_collision_model_init(&next, &model->bsp.allocator))
        return 0;
    ok = bh_bsp_load(&next.bsp, name, prefix, search);
    return commit(model, &next, ok);
}
