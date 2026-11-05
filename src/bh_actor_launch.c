#include "bh_actor_launch.h"
#include <stdlib.h>
BHEntity *__cdecl bh_actor_launch(BHActorLaunchContext *ctx, BHWorldGeometry *world, unsigned long type, long token,
                                  float speed, long tag)
{
    unsigned long r;
    if (!ctx || !ctx->actors)
    {
        if (ctx)
            ctx->error = 1;
        return 0;
    }
    ctx->error = 0;
    r = (unsigned long)rand();
    r += (unsigned long)rand() * 65536UL;
    return bh_actor_construct_room(ctx->actors, world, type, token, speed, (unsigned long)tag, r);
}
