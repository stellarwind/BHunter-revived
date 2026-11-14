#include "../Game/Entities/bh_field_access.h"
#include "bh_world_reset.h"
#include "../Math/bh_rotation.h"
#include "../Math/bh_round.h"
#include "../Math/bh_fp.h"
#include <math.h>
#include <string.h>
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static void setword(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static int live(BHEntityPool *p, BHEntity *e)
{
    return bh_entity_is_active(p, e);
}
static void identity(BHMat34 *matrix)
{
    memset(matrix, 0, sizeof(*matrix));
    matrix->m[0][0] = matrix->m[1][1] = matrix->m[2][2] = 1;
}
int bh_world_reset(BHWorldReset *world_reset)
{
    BHWorldResetNode *world_reset_node;
    BHEntityPool *pool;
    BHEntity *entity, *timer;
    BHWorldGeometry *world;
    BHScriptMotionActor motion;
    BHScriptLifetimeState life;
    BHScriptVMState vm;
    BHVec3 old, position, zero = {0, 0, 0};
    unsigned long i, j, next;
    float angle, dx, dy;
    double dz;
    if (!world_reset || !world_reset->vm || !world_reset->vm->lifetime || !(pool = world_reset->vm->lifetime->timers.pool) || !world_reset->nodes || !world_reset->count ||
        world_reset->count > 65535 || !world_reset->loot_rows || !bh_math_environment_supported())
    {
        if (world_reset)
            world_reset->error = 1;
        return 0;
    }
    world_reset->error = 0;
    world_reset->timers = world_reset->immediate = 0;
    for (i = 0; i < world_reset->count; ++i)
        if (!world_reset->nodes[i].entity || !world_reset->nodes[i].placement || !world_reset->nodes[i].definition || !live(pool, world_reset->nodes[i].entity))
        {
            world_reset->error = 1;
            return 0;
        }
    for (i = 0; i < world_reset->count; ++i)
    {
        world_reset_node = world_reset->nodes + i;
        next = (i + 1) % world_reset->count;
        if (world_reset_node->placement->geometry.navigation.next != &world_reset->nodes[next].placement->geometry.navigation ||
            world_reset_node->placement->geometry.previous != &world_reset->nodes[(i + world_reset->count - 1) % world_reset->count].placement->geometry)
        {
            world_reset->error = 1;
            return 0;
        }
    }
    memset(world_reset->loot_rows, 0, 4 * 32 * 4);
    for (i = 0; i < world_reset->count; ++i)
    {
        world_reset_node = world_reset->nodes + i;
        entity = world_reset_node->entity;
        world = &world_reset_node->placement->geometry;
        old = entity->position;
        bh_script_motion_actor_read(entity, &motion);
        angle = stored((world->flags & 3UL) * 1.5707963267948966);
        motion.angles.x = angle;
        motion.angles.y = motion.angles.z = 0;
        for (j = 0; j < 3; ++j)
        {
            ((float *)&position)[j] = stored((double)world_reset_node->placement->coordinates[j]);
            if (!_finite(((float *)&old)[j]) || !_finite(((float *)&motion.minimum)[j]) ||
                !_finite(((float *)&motion.maximum)[j]))
            {
                world_reset->error = 3;
                return 0;
            }
        }
        entity->position = position;
        identity(&motion.local_to_world);
        bh_mat34_translate_local(position.x, position.y, position.z, &motion.local_to_world);
        bh_mat34_rotate_y(angle, &motion.local_to_world);
        identity(&motion.world_to_local);
        bh_mat34_rotate_y(-angle, &motion.world_to_local);
        bh_mat34_translate_local(-position.x, -position.y, -position.z, &motion.world_to_local);
        bh_field_store_u32(&entity->placement.damage, 0);
        bh_field_store_u32(&entity->placement.damage_source, 0);
        dx = stored((double)old.x - position.x);
        dy = stored((double)old.y - position.y);
        dz = (double)old.z - position.z;
        motion.minimum.x = stored((double)motion.minimum.x - dx);
        motion.maximum.x = stored((double)motion.maximum.x - dx);
        motion.minimum.y = stored((double)motion.minimum.y - dy);
        motion.maximum.y = stored((double)motion.maximum.y - dy);
        motion.minimum.z = stored((double)motion.minimum.z - dz);
        motion.maximum.z = stored((double)motion.maximum.z - dz);
        entity->origin = zero;
        bh_field_store_u32(&entity->placement.query_flags, world_reset_node->definition->flags);
        bh_script_motion_actor_write(entity, &motion);
        world->position = position;
        world->query_flags = world_reset_node->definition->flags;
        world->transform = motion.local_to_world;
        world->inverse = motion.world_to_local;
        world->minimum = motion.minimum;
        world->maximum = motion.maximum;
        world_reset_node->placement->angles = motion.angles;
        if (world_reset_node->draw_angles)
            *world_reset_node->draw_angles = motion.angles;
        if (world_reset_node->definition->script)
        {
            timer = bh_entity_acquire(pool);
            if (timer)
            {
                bh_script_lifetime_read(timer, &life);
                life.actor = entity;
                life.script_loaded = world_reset->vm->script_loading;
                life.sound = 0;
                if (!bh_script_lifetime_bind(world_reset->vm->lifetime, timer, &life, bh_script_unloaded_query))
                {
                    world_reset->error = 2;
                    return 0;
                }
                bh_script_timer_fields(timer)->remaining = 1;
                bh_script_timer_fields(timer)->high = 0;
                bh_script_timer_fields(timer)->countdown = 0;
                bh_script_timer_fields(timer)->fired = world_reset_node->definition->script;
                timer->position = pool->origin;
                timer->step = bh_script_timer_unconditional_step();
                ++world_reset->timers;
                if (world_reset_node->definition->script->query == bh_script_vm_query &&
                    bh_script_vm_read(world_reset_node->definition->script, &vm) && vm.opcode == 20)
                {
                    ++world_reset->immediate;
                    timer->step(timer);
                    if (world_reset->vm->error)
                    {
                        world_reset->error = 2;
                        return 0;
                    }
                }
            }
        }
        bh_field_store_u32(&world_reset->nodes[(i + 1) % world_reset->count].entity->mass, 0x47c35000UL);
    }
    return 1;
}
