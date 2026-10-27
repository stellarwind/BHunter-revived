#include "bh_player_query.h"
#include "bh_round.h"
#include "bh_physics_step.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
static BHPlayerQueryContext *active;
typedef char query_abi[sizeof(void *) == 4 && sizeof(BHPlayerQueryPacket) == 40 ? 1 : -1];
static long word(BHEntity *e, unsigned int o)
{
    long v;
    memcpy(&v, (unsigned char *)e + o, 4);
    return v;
}
static float scalar(BHEntity *e, unsigned int o)
{
    float v;
    memcpy(&v, (unsigned char *)e + o, 4);
    return v;
}
static void put(BHEntity *e, unsigned int o, long v)
{
    memcpy((unsigned char *)e + o, &v, 4);
}
static void set(BHEntity *e, unsigned int o, double d)
{
    float v;
    bh_round_float(&v, d);
    memcpy((unsigned char *)e + o, &v, 4);
}
static float stored(double d)
{
    float v;
    bh_round_float(&v, d);
    return v;
}
static void put_byte(BHEntity *e, unsigned int o, unsigned char v)
{
    ((unsigned char *)e)[o] = v;
}
static unsigned char octet(BHEntity *e, unsigned int o)
{
    return ((unsigned char *)e)[o];
}
static int protected_player(BHEntity *e)
{
    unsigned short v;
    memcpy(&v, (unsigned char *)e + 0x2b4, 2);
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
static void message(BHPlayerQueryContext *c, const char *s)
{
    if (!bh_message_enqueue(c->messages, s))
        c->error = 2;
}
static void broadcast(BHPlayerQueryContext *c)
{
    BHEntity *e = *c->actors;
    unsigned long n = 0;
    BHPlayerQueryPacket p;
    while (e)
    {
        if (++n > c->weapons->pool->capacity)
        {
            c->error = 3;
            return;
        }
        if (word(e, 0xfc) == 1)
        {
            p.words[0] = 15;
            if (!e->query)
            {
                c->error = 3;
                return;
            }
            ++c->broadcasts;
            e->query(e, &p);
        }
        memcpy(&e, (unsigned char *)e + 0xf0, 4);
    }
}
static const char warning30[] =
    "1Listen, Hunter!\n\nYour unpaid fines\nexceed $ 30.000.\n\nBetter come down and\npay your debts...\n\nNOW !!!\n";
static const char warning50[] =
    "1That's it !\n\nYour fines exceed\n$ 50.000.\n\nWe put a bounty on\nyour head until you\npay.\n\nBHunter scum!\n";
static const char restored[] =
    "1Congratulations,\nHunter.\n\nYou have paid back\nyour debts and\nregained your\ncivil rights.\n\nNow keep your\npath clean!\n";
static int registry(BHPlayerQueryContext *c, unsigned int i, BHEntity **e, BHWeaponObjectState **s)
{
    BHEntity *p;
    if (!bh_weapon_registry_resolve(c->weapons, i, e, s))
    {
        c->error = 1;
        return 0;
    }
    memcpy(&p, (unsigned char *)c->player + 0x258 + i * 4, 4);
    if (p != *e)
    {
        c->error = 1;
        return 0;
    }
    return 1;
}
static void cue(BHPlayerQueryContext *c, int secondary)
{
    BHEntity *e = bh_entity_acquire(c->weapons->pool);
    BHScriptLifetimeContext *life = c->lifetime;
    if (!e)
        return;
    /* Same private binding consumed by the real bh_mission_sound_tick owner.
    No original field besides104/180 is initialized by this allocation. */
    memcpy(e->payload + 400, &life, 4);
    put(e, 0x104, (long)(secondary ? c->secondary_cue : c->primary_cue));
    set(e, 0x180, 2);
    e->step = bh_mission_sound_tick;
}
static void select_weapon(BHPlayerQueryContext *c, unsigned int i)
{
    BHEntity *old, *now;
    BHWeaponObjectState *os, *ns;
    unsigned int slot = i < 5 ? 0x2b6 : 0x2b7, previous = octet(c->player, slot);
    if (previous == i)
        return;
    if (!registry(c, previous, &old, &os) || !registry(c, i, &now, &ns))
        return;
    switch (os->method_120)
    {
    case 0x47d680UL:
        break;
    case 0x431130UL:
        if (!bh_weapon_target_cancel(c->target, old))
            c->error = 4;
        break;
    case 0x432510UL:
        if (!bh_weapon_arc_deactivate(c->arc, old))
            c->error = 4;
        break;
    default:
        c->error = 4;
        return;
    }
    put_byte(c->player, slot, (unsigned char)i);
    switch (ns->method_11c)
    {
    case 0x431050UL:
        if (!bh_weapon_ready_reset(c->ready, now, c->player))
            c->error = 4;
        break;
    case 0x4310c0UL:
        if (!bh_weapon_target_ready(c->target, now, c->player))
            c->error = 4;
        break;
    case 0x4324a0UL:
        if (!bh_weapon_arc_activate(c->arc, now, c->player))
            c->error = 4;
        break;
    default:
        c->error = 4;
        return;
    }
    cue(c, i >= 5);
}
static void purchase(BHPlayerQueryContext *c, BHPlayerQueryPacket *p)
{
    BHEntity *e = c->player, *w;
    BHWeaponObjectState *s;
    long type = p->words[1], price = p->words[4], n = p->words[3], accepted = 0, excess, total, oldfine;
    p->words[5] = 1;
    p->words[6] = 0;
    if (price)
    {
        long affordable = word(e, 0x230) / price;
        if (affordable < n)
            n = affordable;
        if (!n)
            return;
    }
    if (type == 20)
    {
        if (scalar(e, 0x24c) <= n * .01)
        {
            accepted = (long)(__int64)((double)scalar(e, 0x24c) * 100.0f);
            set(e, 0x24c, 0);
        }
        else
        {
            accepted = n;
            set(e, 0x24c, (double)scalar(e, 0x24c) - n * .01);
        }
        p->words[6] = accepted;
    }
    else if (type == 21)
    {
        oldfine = word(e, 0x234);
        put_byte(e, 0x2ba, 60);
        total = mul(n, price);
        if (total < oldfine)
        {
            accepted = n;
            put(e, 0x234, sub(oldfine, total));
        }
        else
        {
            accepted = oldfine / price;
            put(e, 0x234, 0);
        }
        p->words[6] = accepted;
        if (price && accepted)
        {
            put_byte(e, 0x2b9, 60);
            put(e, 0x230, sub(word(e, 0x230), mul(accepted, price)));
        }
        if (word(e, 0x238) && !word(e, 0x234) && oldfine > 0)
        {
            put(e, 0x238, 0);
            message(c, restored);
        }
        broadcast(c);
        return;
    }
    else
    {
        if (!registry(c, (unsigned int)type, &w, &s))
            return;
        if (!p->words[2])
        {
            if ((type == 3 || type >= 5) && !s->common.available_ec)
            {
                s->common.available_ec = 1;
                select_weapon(c, (unsigned int)type);
            }
            total = add(s->common.ammo, n);
            excess = total > s->common.ammo_limit ? sub(total, s->common.ammo_limit) : 0;
            s->common.ammo = sub(total, excess);
            accepted = sub(n, excess);
            p->words[6] = accepted;
        }
        else
        {
            if (!s->common.available_ec)
            {
                s->common.available_ec = 1;
                select_weapon(c, (unsigned int)type);
                p->words[6] = 1;
            }
            else if (price)
                return;
            if (p->words[3] == 1)
            {
                static const long grants[10] = {200, 100, 50, 5, 75, 1, 1, 1, 1, 1};
                n = grants[type];
                total = add(s->common.ammo, n);
                excess = total > s->common.ammo_limit ? sub(total, s->common.ammo_limit) : 0;
                s->common.ammo = sub(total, excess);
                if (n != excess)
                    p->words[6] = 1;
            }
            accepted = p->words[6];
        }
    }
    if (price && accepted)
    {
        put_byte(e, 0x2b9, 60);
        put(e, 0x230, sub(word(e, 0x230), mul(accepted, price)));
    }
}
static void transform(BHEntity *e, double x, double y, double z, BHVec3 *out)
{
    unsigned int row;
    for (row = 0; row < 3; row++)
        ((float *)out)[row] = stored(((double)scalar(e, 0x20 + row * 12) * z + (double)scalar(e, 0x18 + row * 12) * x) +
                                     (double)scalar(e, 0x1c + row * 12) * y);
}
int __cdecl bh_player_query_dispatch(BHPlayerQueryContext *c, BHPlayerQueryPacket *p)
{
    BHEntity *e;
    BHVec3 d, a, b, r, normal;
    double x, y, z, length, factor, impulse, t;
    float f;
    float severity, pitch, level, random_scale;
    long old, value;
    unsigned int i;
    unsigned long resource;
    if (!c || !p || !c->player || !c->weapons || !c->weapons->pool || !c->sound || !c->messages || !c->actors ||
        !c->fine_warning || !c->damage_source || !c->lifetime || !c->ready || !c->target || !c->arc || !c->mode_count ||
        !c->simulation_tick || !c->world || !c->world_index || !c->transport || !c->sound_velocity)
        return 0;
    c->error = 0;
    e = c->player;
    if (c->lifetime->timers.pool != c->weapons->pool || c->lifetime->sounds != c->sound)
    {
        c->error = 1;
        return 0;
    }
    if (p->words[0] == 13 &&
        (p->words[1] < 0 || (p->words[1] > 9 && p->words[1] != 20 && p->words[1] != 21) || p->words[4] < 0 ||
         p->words[3] < 0 || word(e, 0x230) < 0 || (p->words[1] == 21 && !p->words[4])))
    {
        c->error = 1;
        return 0;
    }
    ++c->calls;
    switch (p->words[0])
    {
    case 0:
        random_scale = stored((double)p->floats[1] * .001);
        for (i = 0; i < 3; i++)
        {
            value = rand() - 0x4000;
            ++c->random_calls;
            set(e, 0x2a4 + i * 4,
                (double)scalar(e, 0x2a4 + i * 4) +
                    (i == 2 ? (double)value * random_scale : stored((double)value * random_scale)));
        }
        if (scalar(e, 0x240) <= 0 && !protected_player(e))
        {
            t = (double)p->floats[1] + scalar(e, 0x23c);
            set(e, 0x23c, t);
            *c->damage_source = t < 1 ? NULL : (BHEntity *)p->words[2];
            t = (double)scalar(e, 0x23c) - (double)p->floats[1] * (-4.0f);
            if (scalar(e, 0x254) < t)
                set(e, 0x254, t);
            if (scalar(e, 0x23c) > 1)
                set(e, 0x23c, 1);
        }
        break;
    case 1:
        x = (double)e->position.x - p->floats[1];
        y = (double)e->position.y - p->floats[2];
        z = (double)e->position.z - p->floats[3];
        d.x = stored(x);
        d.y = stored(y);
        d.z = stored(z);
        length = sqrt((z * d.z + y * d.y) + x * d.x);
        if (length < p->floats[5])
        {
            length = stored(length);
            factor = (((double)p->floats[4] / scalar(e, 0xb4)) * (1 - length / p->floats[5]) * 30) / length;
            f = stored(factor);
            set(e, 0x60, (double)scalar(e, 0x60) + factor * d.x);
            set(e, 0x64, (double)scalar(e, 0x64) + (double)f * d.y);
            set(e, 0x68, (double)scalar(e, 0x68) + (double)f * d.z);
            set(e, 0x2a4, (double)scalar(e, 0x2a4) + (double)d.x * f * .01);
            set(e, 0x2a8, (double)scalar(e, 0x2a8) + (double)d.y * f * .01);
            set(e, 0x2ac, (double)scalar(e, 0x2ac) + (double)d.z * f * .01);
        }
        break;
    case 2:
        normal.x = p->floats[4];
        normal.y = p->floats[5];
        normal.z = p->floats[6];
        transform(e, normal.x, normal.y, normal.z, &a);
        x = (double)p->floats[1] - e->position.x;
        y = (double)p->floats[2] - e->position.y;
        z = stored((double)p->floats[3] - e->position.z);
        transform(e, x, y, z, &b);
        bh_vec3_normalize(&b);
        impulse = (double)scalar(e, 0xb8) * p->floats[7] * 900;
        factor = (double)p->floats[7] * scalar(e, 0xb4);
        for (i = 0; i < 3; i++)
        {
            ((float *)&a)[i] = stored((double)((float *)&a)[i] * factor * 270);
            set(e, 0x60 + i * 4,
                (double)scalar(e, 0x60 + i * 4) -
                    (i == 1 ? stored((double)normal.y * impulse) : (double)((float *)&normal)[i] * impulse));
        }
        bh_vec3_cross(&b, &a, &r);
        for (i = 0; i < 3; i++)
        {
            t = (double)((float *)&r)[i] / scalar(e, 0x78 + i * 4);
            set(e, 0x6c + i * 4, (double)scalar(e, 0x6c + i * 4) - t * .5);
            set(e, 0x54 + i * 4, (double)scalar(e, 0x54 + i * 4) * .85);
        }
        severity = p->floats[8];
        if ((double)severity * .5 > .05)
        {
            resource = (b.z >= .975 || b.z <= -.975) ? c->contact_axial : c->contact_side;
            level = stored((double)severity * 3.0f);
            pitch = stored(.9 - (double)severity * (-.2));
            ++c->sound_requests;
            bh_sound_request(c->sound, resource, level, pitch, (BHVec3 *)&p->floats[1], c->sound_velocity);
            if (*c->mode_count > 1)
            {
                unsigned char bytes[32], tail[4];
                BHPacketWriter writer;
                int result;
                bh_packet_begin(&writer, bytes, sizeof(bytes), 6, (unsigned short)*c->simulation_tick);
                result = bh_packet_write_position(&writer, *c->world, *c->world_index, (BHVec3 *)&p->floats[1]);
                if (result > 0)
                {
                    tail[0] = (unsigned char)resource;
                    tail[1] = (unsigned char)(__int64)((double)severity * 128.0f);
                    tail[2] = (unsigned char)(__int64)((double)pitch * 64.0f);
                    tail[3] = 0;
                    bh_packet_append(&writer, tail, 4);
                    ++c->packets;
                    bh_network_packet_send(c->transport, &writer, 0, 0);
                }
                else if (result < 0)
                    c->error = 5;
            }
            if (!protected_player(e) && scalar(e, 0x240) == 0)
            {
                if (scalar(e, 0x250) < 1.5)
                    set(e, 0x23c, (double)scalar(e, 0x23c) - (double)severity * (-.2));
                t = (double)scalar(e, 0x23c) - (double)severity * (-.2);
                if (scalar(e, 0x254) < t)
                    set(e, 0x254, t);
            }
            if (scalar(e, 0x23c) > 1)
                set(e, 0x23c, 1);
            if (scalar(e, 0x240) > 0)
                set(e, 0x23c, 0);
        }
        break;
    case 3:
    case 16:
        for (i = 0; i < 3; i++)
            p->floats[1 + i] = stored((double)((float *)&e->position)[i] - (double)scalar(e, 0x30 + i * 4) * 1.5);
        if (p->words[0] == 16)
        {
            for (i = 0; i < 3; i++)
            {
                p->floats[4 + i] = scalar(e, 0x48 + i * 4);
                p->floats[7 + i] = scalar(e, 0x60 + i * 4);
            }
        }
        break;
    case 4:
        if (octet(e, 0x2bb))
            put_byte(e, 0x2bb, (unsigned char)(octet(e, 0x2bb) + 1));
        break;
    case 5:
        if (octet(e, 0x2bb) > 1)
            put_byte(e, 0x2bb, (unsigned char)(octet(e, 0x2bb) - 1));
        break;
    case 6:
        if (!octet(e, 0x2bb))
            e->step = NULL;
        break;
    case 11:
        for (i = 0; i < 6; i++)
            set(e, 0x48 + i * 4, (double)scalar(e, 0x48 + i * 4) * .75);
        for (i = 0; i < 6; i++)
            set(e, 0x60 + i * 4, (double)scalar(e, 0x60 + i * 4) * .5);
        break;
    case 12:
        value = p->words[1];
        if (value > 0)
        {
            put_byte(e, 0x2b9, 60);
            put(e, 0x230, add(word(e, 0x230), value));
        }
        else if (value < 0)
        {
            old = word(e, 0x234);
            put(e, 0x234, sub(old, value));
            put_byte(e, 0x2ba, 60);
            if (!word(e, 0x238))
            {
                if (word(e, 0x234) >= 50000 && old < 50000)
                {
                    put_byte(e, 0x2ba, 60);
                    put(e, 0x238, 1);
                    message(c, warning50);
                }
                else if (word(e, 0x234) > 29999 && old < 30000)
                {
                    if (!*c->fine_warning)
                        message(c, warning30);
                    *c->fine_warning = 1;
                }
            }
            broadcast(c);
        }
        break;
    case 13:
        purchase(c, p);
        break;
    default:
        break;
    }
    if (p->words[0] == 2 && c->contact_physics && !bh_physics_step_read_entity(c->contact_physics, e))
        c->error = 6;
    return !c->error;
}
void __cdecl bh_player_query(BHEntity *e, void *p)
{
    if (active && active->player == e)
        bh_player_query_dispatch(active, (BHPlayerQueryPacket *)p);
}
int __cdecl bh_player_query_bind(BHPlayerQueryContext *c)
{
    if (!c || active || !c->weapons || !c->weapons->pool || !c->player)
        return 0;
    if (!bh_entity_set_query(c->weapons->pool, c->player, bh_player_query))
        return 0;
    active = c;
    return 1;
}
int __cdecl bh_player_query_unbind(BHPlayerQueryContext *c)
{
    if (!c || active != c)
        return 0;
    if (!bh_entity_set_query(c->weapons->pool, c->player, NULL))
        return 0;
    active = NULL;
    return 1;
}
