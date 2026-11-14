#include "bh_world_edit.h"
#include "../Math/bh_round.h"
#include <stddef.h>
static_assert(offsetof(BHWorldEditPlacement, geometry) == 0, "BHWorldEditPlacement.geometry offset");
static long add(long a, long b)
{
    return (long)((unsigned long)a + (unsigned long)b);
}
static long sub(long a, long b)
{
    return (long)((unsigned long)a - (unsigned long)b);
}
int bh_world_edit_rebuild(BHWorldGeometry *world, unsigned long count)
{
    if (!bh_world_geometry_build(world, count))
        return 0;
    return bh_portals_match(&world->navigation, count);
}
static int snap(BHWorldEditPlacement *world_edit_placement, unsigned long count)
{
    unsigned long i;
    long grid;
    if (!world_edit_placement->geometry.model || (grid = world_edit_placement->geometry.model->grid) <= 0)
        return 0;
    for (i = 0; i < 3; i++)
        world_edit_placement->coordinates[i] = (world_edit_placement->coordinates[i] / grid) * grid;
    for (i = 0; i < 3; i++)
        bh_round_float((&world_edit_placement->geometry.position.x) + i, (double)((world_edit_placement->coordinates[i] / grid) * grid));
    return bh_world_edit_rebuild(&world_edit_placement->geometry, count);
}
int bh_world_edit_move(BHWorldEditPlacement *world_edit_placement, unsigned long count, long x, long y, long z, int single)
{
    unsigned long steps = 0;
    if (!world_edit_placement || !count || count > 65535)
        return 0;
    do
    {
        if (!world_edit_placement->geometry.model || world_edit_placement->geometry.model->grid <= 0 || ++steps > count)
            return 0;
        world_edit_placement->coordinates[0] = add(world_edit_placement->coordinates[0], x);
        world_edit_placement->coordinates[1] = add(world_edit_placement->coordinates[1], y);
        world_edit_placement->coordinates[2] = add(world_edit_placement->coordinates[2], z);
        if (!snap(world_edit_placement, count))
            return 0;
        if (single)
            return 1;
        world_edit_placement = (BHWorldEditPlacement *)world_edit_placement->geometry.navigation.next;
        if (!world_edit_placement)
            return 0;
    } while (world_edit_placement->geometry.owner);
    return 1;
}
int bh_world_edit_rotate(BHWorldEditPlacement *world_edit_placement, unsigned long count, long turn, long pivot_x, long pivot_z)
{
    long x, z;
    unsigned long steps = 0;
    if (!world_edit_placement || !count || count > 65535)
        return 0;
    while (world_edit_placement->geometry.owner)
    {
        if (!world_edit_placement->geometry.model || world_edit_placement->geometry.model->grid <= 0 || ++steps > count)
            return 0;
        world_edit_placement->geometry.flags = ((unsigned long)turn + world_edit_placement->geometry.flags) & 3;
        world_edit_placement->angles.y = world_edit_placement->angles.z = 0;
        bh_round_float(&world_edit_placement->angles.x, (double)world_edit_placement->geometry.flags * 1.5707963267948966);
        x = sub(world_edit_placement->coordinates[0], pivot_x);
        z = sub(world_edit_placement->coordinates[2], pivot_z);
        if (turn < 0)
            z = sub(0, z);
        else
            x = sub(0, x);
        world_edit_placement->coordinates[0] = add(z, pivot_x);
        world_edit_placement->coordinates[2] = add(x, pivot_z);
        if (!snap(world_edit_placement, count))
            return 0;
        world_edit_placement = (BHWorldEditPlacement *)world_edit_placement->geometry.navigation.next;
        if (!world_edit_placement)
            return 0;
    }
    return 1;
}
