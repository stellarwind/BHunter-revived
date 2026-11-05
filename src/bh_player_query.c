#include "bh_player_query.h"
#include "bh_round.h"
#include "bh_physics_step.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
static BHPlayerQueryContext *active;
typedef char query_abi[sizeof(void *) == 4 && sizeof(BHPlayerQueryPacket) == 40 ? 1 : -1];
static long word(BHEntity *entity, unsigned int o)
{
    long v;
    memcpy(&v, (unsigned char *)entity + o, 4);
    return v;
}
static float scalar(BHEntity *entity, unsigned int o)
{
    float v;
    memcpy(&v, (unsigned char *)entity + o, 4);
    return v;
}
static void put(BHEntity *entity, unsigned int o, long v)
{
    memcpy((unsigned char *)entity + o, &v, 4);
}
static void set(BHEntity *entity, unsigned int o, double d)
{
    float v;
    bh_round_float(&v, d);
    memcpy((unsigned char *)entity + o, &v, 4);
}
static float stored(double d)
{
    float v;
    bh_round_float(&v, d);
    return v;
}
static void put_byte(BHEntity *entity, unsigned int o, unsigned char v)
{
    ((unsigned char *)entity)[o] = v;
}
static unsigned char octet(BHEntity *entity, unsigned int o)
{
    return ((unsigned char *)entity)[o];
}
static int protected_player(BHEntity *entity)
{
    unsigned short v;
    memcpy(&v, (unsigned char *)entity + 0x2b4, 2);
    return v != 0;
}
static long add(long a, long b)
{
    return (long)((unsigned long)a + (unsigned long)b);
}
static long sub(long a, long b)
{
    return (long)((unsigned long)a - (unsigned long)b);
}
static long mul(long a, long b)
{
    return (long)((unsigned long)a * (unsigned long)b);
}
static void message(BHPlayerQueryContext *ctx, const char *s)
{
    if (!bh_message_enqueue(ctx->messages, s))
        ctx->error = 2;
}
static void broadcast(BHPlayerQueryContext *ctx)
{
    BHEntity *entity = *ctx->actors;
    unsigned long n = 0;
    BHPlayerQueryPacket packet;
    while (entity)
    {
        if (++n > ctx->weapons->pool->capacity)
        {
            ctx->error = 3;
            return;
        }
        if (word(entity, 0xfc) == 1)
        {
            packet.words[0] = 15;
            if (!entity->query)
            {
                ctx->error = 3;
                return;
            }
            ++ctx->broadcasts;
            entity->query(entity, &packet);
        }
        memcpy(&entity, (unsigned char *)entity + 0xf0, 4);
    }
}
static const char warning30[] =
    "1Listen, Hunter!\n\nYour unpaid fines\nexceed $ 30.000.\n\nBetter come down and\npay your debts...\n\nNOW !!!\n";
static const char warning50[] =
    "1That's it !\n\nYour fines exceed\n$ 50.000.\n\nWe put a bounty on\nyour head until you\npay.\n\nBHunter scum!\n";
static const char restored[] =
    "1Congratulations,\nHunter.\n\nYou have paid back\nyour debts and\nregained your\ncivil rights.\n\nNow keep your\npath clean!\n";
static int registry(BHPlayerQueryContext *ctx, unsigned int i, BHEntity **entity, BHWeaponObjectState **weapon)
{
    BHEntity *p;
    if (!bh_weapon_registry_resolve(ctx->weapons, i, entity, weapon))
    {
        ctx->error = 1;
        return 0;
    }
    memcpy(&p, (unsigned char *)ctx->player + 0x258 + i * 4, 4);
    if (p != *entity)
    {
        ctx->error = 1;
        return 0;
    }
    return 1;
}
static void cue(BHPlayerQueryContext *ctx, int secondary)
{
    BHEntity *entity = bh_entity_acquire(ctx->weapons->pool);
    BHScriptLifetimeContext *life = ctx->lifetime;
    if (!entity)
        return;
    /* Same private binding consumed by the real bh_mission_sound_tick owner.
    No original field besides104/180 is initialized by this allocation. */
    memcpy(entity->payload + 400, &life, 4);
    put(entity, 0x104, (long)(secondary ? ctx->secondary_cue : ctx->primary_cue));
    set(entity, 0x180, 2);
    entity->step = bh_mission_sound_tick;
}
static void select_weapon(BHPlayerQueryContext *ctx, unsigned int i)
{
    BHEntity *old, *now;
    BHWeaponObjectState *os, *ns;
    unsigned int slot = i < 5 ? 0x2b6 : 0x2b7, previous = octet(ctx->player, slot);
    if (previous == i)
        return;
    if (!registry(ctx, previous, &old, &os) || !registry(ctx, i, &now, &ns))
        return;
    switch (os->method_120)
    {
    case 0x47d680UL:
        break;
    case 0x431130UL:
        if (!bh_weapon_target_cancel(ctx->target, old))
            ctx->error = 4;
        break;
    case 0x432510UL:
        if (!bh_weapon_arc_deactivate(ctx->arc, old))
            ctx->error = 4;
        break;
    default:
        ctx->error = 4;
        return;
    }
    put_byte(ctx->player, slot, (unsigned char)i);
    switch (ns->method_11c)
    {
    case 0x431050UL:
        if (!bh_weapon_ready_reset(ctx->ready, now, ctx->player))
            ctx->error = 4;
        break;
    case 0x4310c0UL:
        if (!bh_weapon_target_ready(ctx->target, now, ctx->player))
            ctx->error = 4;
        break;
    case 0x4324a0UL:
        if (!bh_weapon_arc_activate(ctx->arc, now, ctx->player))
            ctx->error = 4;
        break;
    default:
        ctx->error = 4;
        return;
    }
    cue(ctx, i >= 5);
}
static void purchase(BHPlayerQueryContext *ctx, BHPlayerQueryPacket *packet)
{
    BHEntity *entity = ctx->player, *weapon_entity;
    BHWeaponObjectState *weapon;
    long type = packet->words[1], price = packet->words[4], quantity = packet->words[3], accepted = 0, excess, total,
         oldfine;
    packet->words[5] = 1;
    packet->words[6] = 0;
    if (price)
    {
        long affordable = word(entity, 0x230) / price;
        if (affordable < quantity)
            quantity = affordable;
        if (!quantity)
            return;
    }
    if (type == 20)
    {
        if (scalar(entity, 0x24c) <= quantity * .01)
        {
            accepted = (long)(__int64)((double)scalar(entity, 0x24c) * 100.0f);
            set(entity, 0x24c, 0);
        }
        else
        {
            accepted = quantity;
            set(entity, 0x24c, (double)scalar(entity, 0x24c) - quantity * .01);
        }
        packet->words[6] = accepted;
    }
    else if (type == 21)
    {
        oldfine = word(entity, 0x234);
        put_byte(entity, 0x2ba, 60);
        total = mul(quantity, price);
        if (total < oldfine)
        {
            accepted = quantity;
            put(entity, 0x234, sub(oldfine, total));
        }
        else
        {
            accepted = oldfine / price;
            put(entity, 0x234, 0);
        }
        packet->words[6] = accepted;
        if (price && accepted)
        {
            put_byte(entity, 0x2b9, 60);
            put(entity, 0x230, sub(word(entity, 0x230), mul(accepted, price)));
        }
        if (word(entity, 0x238) && !word(entity, 0x234) && oldfine > 0)
        {
            put(entity, 0x238, 0);
            message(ctx, restored);
        }
        broadcast(ctx);
        return;
    }
    else
    {
        if (!registry(ctx, (unsigned int)type, &weapon_entity, &weapon))
            return;
        if (!packet->words[2])
        {
            if ((type == 3 || type >= 5) && !weapon->common.available_ec)
            {
                weapon->common.available_ec = 1;
                select_weapon(ctx, (unsigned int)type);
            }
            total = add(weapon->common.ammo, quantity);
            excess = total > weapon->common.ammo_limit ? sub(total, weapon->common.ammo_limit) : 0;
            weapon->common.ammo = sub(total, excess);
            accepted = sub(quantity, excess);
            packet->words[6] = accepted;
        }
        else
        {
            if (!weapon->common.available_ec)
            {
                weapon->common.available_ec = 1;
                select_weapon(ctx, (unsigned int)type);
                packet->words[6] = 1;
            }
            else if (price)
                return;
            if (packet->words[3] == 1)
            {
                static const long grants[10] = {200, 100, 50, 5, 75, 1, 1, 1, 1, 1};
                quantity = grants[type];
                total = add(weapon->common.ammo, quantity);
                excess = total > weapon->common.ammo_limit ? sub(total, weapon->common.ammo_limit) : 0;
                weapon->common.ammo = sub(total, excess);
                if (quantity != excess)
                    packet->words[6] = 1;
            }
            accepted = packet->words[6];
        }
    }
    if (price && accepted)
    {
        put_byte(entity, 0x2b9, 60);
        put(entity, 0x230, sub(word(entity, 0x230), mul(accepted, price)));
    }
}
static void transform(BHEntity *entity, double x, double y, double z, BHVec3 *out)
{
    unsigned int row;
    for (row = 0; row < 3; row++)
        ((float *)out)[row] =
            stored(((double)scalar(entity, 0x20 + row * 12) * z + (double)scalar(entity, 0x18 + row * 12) * x) +
                   (double)scalar(entity, 0x1c + row * 12) * y);
}
int __cdecl bh_player_query_dispatch(BHPlayerQueryContext *ctx, BHPlayerQueryPacket *packet)
{
    BHEntity *entity;
    BHVec3 d, a, b, r, normal;
    double x, y, z, length, factor, impulse, t;
    float f;
    float severity, pitch, level, random_scale;
    long old, value;
    unsigned int i;
    unsigned long resource;
    if (!ctx || !packet || !ctx->player || !ctx->weapons || !ctx->weapons->pool || !ctx->sound || !ctx->messages ||
        !ctx->actors || !ctx->fine_warning || !ctx->damage_source || !ctx->lifetime || !ctx->ready || !ctx->target ||
        !ctx->arc || !ctx->mode_count || !ctx->simulation_tick || !ctx->world || !ctx->world_index || !ctx->transport ||
        !ctx->sound_velocity)
        return 0;
    ctx->error = 0;
    entity = ctx->player;
    if (ctx->lifetime->timers.pool != ctx->weapons->pool || ctx->lifetime->sounds != ctx->sound)
    {
        ctx->error = 1;
        return 0;
    }
    if (packet->words[0] == 13 &&
        (packet->words[1] < 0 || (packet->words[1] > 9 && packet->words[1] != 20 && packet->words[1] != 21) ||
         packet->words[4] < 0 || packet->words[3] < 0 || word(entity, 0x230) < 0 ||
         (packet->words[1] == 21 && !packet->words[4])))
    {
        ctx->error = 1;
        return 0;
    }
    ++ctx->calls;
    switch (packet->words[0])
    {
    case 0:
        random_scale = stored((double)packet->floats[1] * .001);
        for (i = 0; i < 3; i++)
        {
            value = rand() - 0x4000;
            ++ctx->random_calls;
            set(entity, 0x2a4 + i * 4,
                (double)scalar(entity, 0x2a4 + i * 4) +
                    (i == 2 ? (double)value * random_scale : stored((double)value * random_scale)));
        }
        if (scalar(entity, 0x240) <= 0 && !protected_player(entity))
        {
            t = (double)packet->floats[1] + scalar(entity, 0x23c);
            set(entity, 0x23c, t);
            *ctx->damage_source = t < 1 ? NULL : (BHEntity *)packet->words[2];
            t = (double)scalar(entity, 0x23c) - (double)packet->floats[1] * (-4.0f);
            if (scalar(entity, 0x254) < t)
                set(entity, 0x254, t);
            if (scalar(entity, 0x23c) > 1)
                set(entity, 0x23c, 1);
        }
        break;
    case 1:
        x = (double)entity->position.x - packet->floats[1];
        y = (double)entity->position.y - packet->floats[2];
        z = (double)entity->position.z - packet->floats[3];
        d.x = stored(x);
        d.y = stored(y);
        d.z = stored(z);
        length = sqrt((z * d.z + y * d.y) + x * d.x);
        // Possible division by zero here
        if (length > 0 && length < packet->floats[5])
        {
            length = stored(length);
            factor =
                (((double)packet->floats[4] / scalar(entity, 0xb4)) * (1 - length / packet->floats[5]) * 30) / length;
            f = stored(factor);
            set(entity, 0x60, (double)scalar(entity, 0x60) + factor * d.x);
            set(entity, 0x64, (double)scalar(entity, 0x64) + (double)f * d.y);
            set(entity, 0x68, (double)scalar(entity, 0x68) + (double)f * d.z);
            set(entity, 0x2a4, (double)scalar(entity, 0x2a4) + (double)d.x * f * .01);
            set(entity, 0x2a8, (double)scalar(entity, 0x2a8) + (double)d.y * f * .01);
            set(entity, 0x2ac, (double)scalar(entity, 0x2ac) + (double)d.z * f * .01);
        }
        break;
    case 2:
        normal.x = packet->floats[4];
        normal.y = packet->floats[5];
        normal.z = packet->floats[6];
        transform(entity, normal.x, normal.y, normal.z, &a);
        x = (double)packet->floats[1] - entity->position.x;
        y = (double)packet->floats[2] - entity->position.y;
        z = stored((double)packet->floats[3] - entity->position.z);
        transform(entity, x, y, z, &b);
        bh_vec3_normalize(&b);
        impulse = (double)scalar(entity, 0xb8) * packet->floats[7] * 900;
        factor = (double)packet->floats[7] * scalar(entity, 0xb4);
        for (i = 0; i < 3; i++)
        {
            ((float *)&a)[i] = stored((double)((float *)&a)[i] * factor * 270);
            set(entity, 0x60 + i * 4,
                (double)scalar(entity, 0x60 + i * 4) -
                    (i == 1 ? stored((double)normal.y * impulse) : (double)((float *)&normal)[i] * impulse));
        }
        bh_vec3_cross(&b, &a, &r);
        for (i = 0; i < 3; i++)
        {
            t = (double)((float *)&r)[i] / scalar(entity, 0x78 + i * 4);
            set(entity, 0x6c + i * 4, (double)scalar(entity, 0x6c + i * 4) - t * .5);
            set(entity, 0x54 + i * 4, (double)scalar(entity, 0x54 + i * 4) * .85);
        }
        severity = packet->floats[8];
        if ((double)severity * .5 > .05)
        {
            resource = (b.z >= .975 || b.z <= -.975) ? ctx->contact_axial : ctx->contact_side;
            level = stored((double)severity * 3.0f);
            pitch = stored(.9 - (double)severity * (-.2));
            ++ctx->sound_requests;
            bh_sound_request(ctx->sound, resource, level, pitch, (BHVec3 *)&packet->floats[1], ctx->sound_velocity);
            if (*ctx->mode_count > 1)
            {
                unsigned char bytes[32], tail[4];
                BHPacketWriter writer;
                int result;
                bh_packet_begin(&writer, bytes, sizeof(bytes), 6, (unsigned short)*ctx->simulation_tick);
                result =
                    bh_packet_write_position(&writer, *ctx->world, *ctx->world_index, (BHVec3 *)&packet->floats[1]);
                if (result > 0)
                {
                    tail[0] = (unsigned char)resource;
                    tail[1] = (unsigned char)(__int64)((double)severity * 128.0f);
                    tail[2] = (unsigned char)(__int64)((double)pitch * 64.0f);
                    tail[3] = 0;
                    bh_packet_append(&writer, tail, 4);
                    ++ctx->packets;
                    bh_network_packet_send(ctx->transport, &writer, 0, 0);
                }
                else if (result < 0)
                    ctx->error = 5;
            }
            if (!protected_player(entity) && scalar(entity, 0x240) == 0)
            {
                if (scalar(entity, 0x250) < 1.5)
                    set(entity, 0x23c, (double)scalar(entity, 0x23c) - (double)severity * (-.2));
                t = (double)scalar(entity, 0x23c) - (double)severity * (-.2);
                if (scalar(entity, 0x254) < t)
                    set(entity, 0x254, t);
            }
            if (scalar(entity, 0x23c) > 1)
                set(entity, 0x23c, 1);
            if (scalar(entity, 0x240) > 0)
                set(entity, 0x23c, 0);
        }
        break;
    case 3:
    case 16:
        for (i = 0; i < 3; i++)
            packet->floats[1 + i] =
                stored((double)((float *)&entity->position)[i] - (double)scalar(entity, 0x30 + i * 4) * 1.5);
        if (packet->words[0] == 16)
        {
            for (i = 0; i < 3; i++)
            {
                packet->floats[4 + i] = scalar(entity, 0x48 + i * 4);
                packet->floats[7 + i] = scalar(entity, 0x60 + i * 4);
            }
        }
        break;
    case 4:
        if (octet(entity, 0x2bb))
            put_byte(entity, 0x2bb, (unsigned char)(octet(entity, 0x2bb) + 1));
        break;
    case 5:
        if (octet(entity, 0x2bb) > 1)
            put_byte(entity, 0x2bb, (unsigned char)(octet(entity, 0x2bb) - 1));
        break;
    case 6:
        if (!octet(entity, 0x2bb))
            entity->step = NULL;
        break;
    case 11:
        for (i = 0; i < 6; i++)
            set(entity, 0x48 + i * 4, (double)scalar(entity, 0x48 + i * 4) * .75);
        for (i = 0; i < 6; i++)
            set(entity, 0x60 + i * 4, (double)scalar(entity, 0x60 + i * 4) * .5);
        break;
    case 12:
        value = packet->words[1];
        if (value > 0)
        {
            put_byte(entity, 0x2b9, 60);
            put(entity, 0x230, add(word(entity, 0x230), value));
        }
        else if (value < 0)
        {
            old = word(entity, 0x234);
            put(entity, 0x234, sub(old, value));
            put_byte(entity, 0x2ba, 60);
            if (!word(entity, 0x238))
            {
                if (word(entity, 0x234) >= 50000 && old < 50000)
                {
                    put_byte(entity, 0x2ba, 60);
                    put(entity, 0x238, 1);
                    message(ctx, warning50);
                }
                else if (word(entity, 0x234) > 29999 && old < 30000)
                {
                    if (!*ctx->fine_warning)
                        message(ctx, warning30);
                    *ctx->fine_warning = 1;
                }
            }
            broadcast(ctx);
        }
        break;
    case 13:
        purchase(ctx, packet);
        break;
    default:
        break;
    }
    if (packet->words[0] == 2 && ctx->contact_physics && !bh_physics_step_read_entity(ctx->contact_physics, entity))
        ctx->error = 6;
    return !ctx->error;
}
void __cdecl bh_player_query(BHEntity *entity, void *p)
{
    if (active && active->player == entity)
        bh_player_query_dispatch(active, (BHPlayerQueryPacket *)p);
}
int __cdecl bh_player_query_bind(BHPlayerQueryContext *ctx)
{
    if (!ctx || active || !ctx->weapons || !ctx->weapons->pool || !ctx->player)
        return 0;
    if (!bh_entity_set_query(ctx->weapons->pool, ctx->player, bh_player_query))
        return 0;
    active = ctx;
    return 1;
}
int __cdecl bh_player_query_unbind(BHPlayerQueryContext *ctx)
{
    if (!ctx || active != ctx)
        return 0;
    if (!bh_entity_set_query(ctx->weapons->pool, ctx->player, NULL))
        return 0;
    active = NULL;
    return 1;
}
