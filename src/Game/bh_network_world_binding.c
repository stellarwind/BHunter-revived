#include "bh_network_world_binding.h"
#include "../Network/bh_network_game.h"
#include "Scripts/bh_script_wait.h"
#include <stdlib.h>

struct BHNetworkPickupBinding
{
    BHEntity *entity;
    unsigned long entity_generation, retry_frame;
    unsigned int spawn_generation, actor_index;
};

static int script_pickup(void *user, unsigned long placement)
{
    BHNetworkWorldBinding *map = user;
    BHNetworkGame *game = map->game;
    BHNetworkPickup *item = bh_network_world_pickup(&game->world, placement);
    if (!game->world_sync || !item || (!game->hosting && !item->synced))
        return 0;
    if (!item->activated)
        bh_network_pickup_activate(game, item);
    return 1;
}

static BHVec3 pickup_position(const BHNetworkWorldBinding *map, const BHNetworkPickup *item)
{
    const BHNetworkPickupBinding *view = map->pickups + (item - map->game->world.pickups);
    const BHWorldGeometry *world = map->vm->actors[view->actor_index].world;
    return (BHVec3){(world->minimum.x + world->maximum.x) * .5f,
                    (world->minimum.y + world->maximum.y) * .5f,
                    (world->minimum.z + world->maximum.z) * .5f};
}

static void apply_pickup_grant(void *user, const BHNetworkPickup *item)
{
    BHNetworkWorldBinding *map = user;
    const BHNetworkPickupBinding *view = map->pickups + (item - map->game->world.pickups);
    BHPickupContext *pickups = map->vm->pickups;
    unsigned int offer = item->offer < item->total ? item->offer : item->total;
    BHPlayerQueryPacket packet = {.words = {13, item->type, 0, offer, item->price}};
    if (view->actor_index >= map->vm->actor_count)
        return;
    if (item->type >= 10 && item->type < 20)
    {
        packet.words[1] -= 10;
        packet.words[2] = 1;
    }
    bh_player_query_dispatch(map->player_query, &packet);
    if (packet.words[6])
    {
        BHVec3 position = pickup_position(map, item);
        bh_sound_request(pickups->sound, pickups->take_sound, 1, 1, &position, &pickups->sound_velocity);
    }
}

static void step_pickup(BHNetworkWorldBinding *map, BHNetworkPickup *item, BHNetworkPickupBinding *view,
                        unsigned long frame, int paused)
{
    BHPickupContext *pickups = map->vm->pickups;
    const BHEntity *player = map->player_query->player;
    BHEntity *entity = view->entity;
    BHVec3 position;
    double x, y, z;
    if (entity && (!bh_entity_is_active(pickups->pool, entity) || entity->generation != view->entity_generation))
        entity = view->entity = NULL;
    if (entity && (!item->available || item->generation != view->spawn_generation))
    {
        bh_pickup_cleanup(entity);
        entity = view->entity = NULL;
    }
    if (!item->available || view->actor_index >= map->vm->actor_count ||
        (!map->game->hosting && !item->synced))
        return;
    position = pickup_position(map, item);
    if (!entity)
    {
        if (item->type < 20)
            entity = bh_pickup_spawn(pickups, &position, item->type % 10, item->type / 10, item->total,
                                     item->offer, item->price, 0);
        else
            entity = bh_pickup_basic_spawn(pickups, &position, item->type, item->total, item->offer, item->price, 0);
        if (!entity)
            return;
        /* The server owns depletion and expiry. Keep the original renderer and reset query. */
        entity->step = NULL;
        view->entity = entity;
        view->entity_generation = entity->generation;
        view->spawn_generation = item->generation;
        view->retry_frame = 0;
    }
    bh_pickup_state(entity)->rotation += 1.0f / 30;
    if (!player || !player->world || paused || frame < view->retry_frame)
        return;
    x = (double)position.x - player->position.x;
    y = (double)position.y - player->position.y;
    z = (double)position.z - player->position.z;
    if (x * x + y * y + z * z >= 36)
        return;
    view->retry_frame = frame + 8;
    bh_network_pickup_take(map->game, item);
}

/* Read the flag changes from the model script without firing its effects. */
static unsigned long destroyed_flags(BHNetworkWorldBinding *map, BHEntity *node, int in_wait, unsigned int depth)
{
    unsigned long flags = 0;
    if (depth == 32)
        return 0;
    while (node)
    {
        BHScriptWaitState *wait = bh_script_wait_state(&map->vm->lifetime->timers, node);
        BHScriptTimerState *timer;
        BHScriptVMState command;
        if (wait)
        {
            flags |= destroyed_flags(map, wait->fired, 1, depth + 1);
            node = wait->next;
        }
        else if ((timer = bh_script_timer_state(&map->vm->lifetime->timers, node)))
        {
            flags |= destroyed_flags(map, timer->fired, in_wait, depth + 1);
            node = timer->next;
        }
        else if (node->query == bh_script_vm_query && bh_script_vm_read(node, &command))
        {
            if (in_wait && command.opcode == 0 && !command.args[0])
                flags |= (unsigned long)command.args[1] & 6;
            node = command.next;
        }
        else
            break;
    }
    return flags;
}

static void apply_destroyed_props(BHNetworkWorldBinding *map)
{
    const BHNetworkWorld *world = &map->game->world;
    while (map->props_applied < world->destroyed_count)
    {
        unsigned int placement = world->destroyed_props[map->props_applied++];
        unsigned long mask;
        BHScriptVMActor *actor;
        if (placement >= map->vm->actor_count)
            continue;
        actor = map->vm->actors + placement;
        if (actor->destroyed)
            continue; /* The live event already ran its destruction script. */
        mask = destroyed_flags(map, map->placements[placement].definition->script, 0, 0);
        actor->destroyed = 1;
        actor->state->entity->placement.query_flags &= ~mask;
        actor->world->query_flags &= ~mask;
        if (map->log)
            fprintf(map->log, "world_prop placement=%u hidden_flags=%lu\n", placement, mask);
    }
}

int bh_network_world_binding_bind(BHNetworkWorldBinding *map, BHNetworkGame *game, BHScriptVMContext *vm,
                                  const BHWorldResetNode *placements, BHPlayerQueryContext *player_query,
                                  FILE *log, int custom_level)
{
    BHNetworkWorld *world = &game->world;
    BHLevelRecords records = {0};
    int loaded;
    if (!game->host)
        return 1;
    bh_network_world_binding_dispose(map);
    if (!bh_level_records_load(&records, custom_level))
        return 0;
    loaded = bh_network_world_load(world, &records);
    bh_level_records_dispose(&records);
    if (!loaded)
        return 0;
    if (world->pickup_count)
    {
        map->pickups = calloc(world->pickup_count, sizeof(*map->pickups));
        if (!map->pickups)
            return 0;
    }
    map->game = game;
    map->vm = vm;
    map->placements = placements;
    map->player_query = player_query;
    map->log = log;
    for (unsigned int i = 0, actor = 0; i < world->pickup_count; ++i)
    {
        unsigned int placement = world->pickups[i].placement;
        while (actor < vm->actor_count && vm->actors[actor].state->entity->placement.placement_index < placement)
            ++actor;
        map->pickups[i].actor_index =
            actor < vm->actor_count && vm->actors[actor].state->entity->placement.placement_index == placement
                ? actor : (unsigned int)vm->actor_count;
    }
    if (game->hosting)
    {
        game->world_sync = game->world_ready = 1;
        bh_network_update_world(game);
    }
    vm->map_pickup = script_pickup;
    vm->map_pickup_user = map;
    game->pickup_grant = apply_pickup_grant;
    game->pickup_user = map;
    return 1;
}

void bh_network_world_binding_step(BHNetworkWorldBinding *map, unsigned long frame, int paused)
{
    BHNetworkGame *game = map->game;
    if (!game || !game->world_sync)
        return;
    if (game->hosting)
        bh_network_update_world(game);
    else
        apply_destroyed_props(map);
    for (unsigned int i = 0; i < game->world.pickup_count; ++i)
        step_pickup(map, game->world.pickups + i, map->pickups + i, frame, paused);
}

void bh_network_world_binding_dispose(BHNetworkWorldBinding *map)
{
    if (map->vm && map->vm->map_pickup_user == map)
    {
        map->vm->map_pickup = NULL;
        map->vm->map_pickup_user = NULL;
    }
    if (map->game && map->game->pickup_user == map)
    {
        map->game->pickup_grant = NULL;
        map->game->pickup_user = NULL;
    }
    free(map->pickups);
    *map = (BHNetworkWorldBinding){0};
}
