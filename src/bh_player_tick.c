#include "bh_player_tick.h"
#include "bh_input_evaluate.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static BHPlayerTick *bound_player;
static const BHVec3 zero = {0, 0, 0};
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static unsigned long word(BHEntity *e, unsigned long o)
{
    unsigned long n;
    memcpy(&n, (char *)e + o, 4);
    return n;
}
static void setword(BHEntity *e, unsigned long o, unsigned long n)
{
    memcpy((char *)e + o, &n, 4);
}
static short half(BHEntity *e, unsigned long o)
{
    short n;
    memcpy(&n, (char *)e + o, 2);
    return n;
}
static void sethalf(BHEntity *e, unsigned long o, unsigned short n)
{
    memcpy((char *)e + o, &n, 2);
}
static float number(BHEntity *e, unsigned long o)
{
    float f;
    memcpy(&f, (char *)e + o, 4);
    return f;
}
static void real(BHEntity *e, unsigned long o, float f)
{
    memcpy((char *)e + o, &f, 4);
}
static BHVec3 vector(BHEntity *e, unsigned long o)
{
    BHVec3 v;
    memcpy(&v, (char *)e + o, 12);
    return v;
}
static void setvector(BHEntity *e, unsigned long o, const BHVec3 *v)
{
    memcpy((char *)e + o, v, 12);
}
static unsigned char getbyte(BHEntity *e, unsigned long o)
{
    return *((unsigned char *)e + o);
}
static void setbyte(BHEntity *e, unsigned long o, unsigned char n)
{
    *((unsigned char *)e + o) = n;
}
static int fail(BHPlayerTick *c, int n)
{
    c->error = n;
    return 0;
}
static int key(BHPlayerTick *c, unsigned long index)
{
    unsigned char result;
    c->decoded->values[index] = word(c->player, 0x18c + 4 * index);
    result = bh_player_input_transition(c->decoded, index);
    setword(c->player, 0x18c + 4 * index, c->decoded->values[index]);
    return result;
}
static int input(BHPlayerTick *c)
{
    BHPlayerInputState state;
    unsigned long i;
    if (!c->input || !c->decoded)
        return fail(c, 1);
    state = *c->decoded;
    memcpy(state.values, (char *)c->player + 0x18c, 36 * 4);
    if (!bh_player_input_apply_packet_checked(&state, c->input->state, sizeof(c->input->state)))
        return fail(c, 2);
    for (i = 36; i < 256; ++i)
        if (state.values[i] != c->decoded->values[i])
            return fail(c, 2);
    *c->decoded = state;
    memcpy((char *)c->player + 0x18c, state.values, 36 * 4);
    return 1;
}
static unsigned long sound(BHPlayerTick *c, unsigned long resource, float level, float pitch, const BHVec3 *position,
                           const BHVec3 *velocity)
{
    return bh_sound_request(c->sound, resource, level, pitch, position, velocity);
}
static void restore_volume(BHPlayerTick *c)
{
    if (c->state->paused)
    {
        c->state->paused = 0;
        bh_cd_set_volume(c->media, c->state->saved_cd_volume);
        bh_wave_set_volume(c->media, c->state->saved_wave_volume);
    }
}
static void menu_cue(BHPlayerTick *c, float a, float b)
{
    sound(c, c->resources.menu_cue, 1, a, 0, 0);
    sound(c, c->resources.menu_cue, 1, b, 0, 0);
}
static int camera_reset(BHPlayerTick *c, unsigned long index)
{
    if (!c->camera)
        return fail(c, 1);
    c->camera->player = c->player;
    c->camera->player_count = c->state->mode;
    if (c->network && c->network->packets)
        c->camera->local_player_index = *c->network->packets->local_slot;
    return bh_player_camera_reset(c->camera, index) ? 1 : fail(c, 3);
}
static int return_menu(BHPlayerTick *c)
{
    BHMessagePanelFields *fields;
    unsigned long event = 20;
    if (!c->reset || !c->panel || !c->panel->entity || !c->snapshot || !c->world_reset || !c->menu)
        return fail(c, 1);
    if (!camera_reset(c, 0) || !bh_player_reset(c->reset))
        return fail(c, 3);
    c->panel->messages->ring_start = 0;
    c->panel->messages->ring_count = 0;
    c->panel->messages->last_index = -1;
    c->panel->messages->display_ticks = c->panel->messages->display_phase = 0;
    c->panel->messages->arena_used = 0;
    fields = bh_message_panel_fields(c->panel->entity);
    fields->timer = fields->scale = 0;
    c->panel->mode_identity = 0x00442bf0UL;
    c->panel->entity->step = bh_message_panel_step;
    if (!bh_entity_set_query(c->pool, c->panel->entity, bh_message_panel_query) ||
        !bh_input_set_enabled(c->input, c->snapshot, 0))
        return fail(c, 3);
    if (!bh_entities_query(c->pool, 0, 0, 0, 0, &event) || !bh_world_reset(c->world_reset) ||
        !bh_menu_runtime_return(c->menu))
        return fail(c, 3);
    c->state->mode = 0;
    return 1;
}
static int packet_send(BHPlayerTick *c, unsigned char type, const void *payload, unsigned long n)
{
    if (!c->network || !c->network->transport ||
        !bh_packet_begin(&c->writer, c->packet, 64, type, (unsigned short)c->state->tick) ||
        !bh_packet_append(&c->writer, payload, n))
        return fail(c, 1);
    bh_network_packet_send(c->network->transport, &c->writer, 0, 0);
    c->packet_bytes = c->writer.cursor;
    ++c->packets;
    return 1;
}
static int sound_packet(BHPlayerTick *c, unsigned long resource, float level, float pitch, unsigned long sparks)
{
    BHWorldGeometry *w;
    unsigned long i;
    unsigned char tail[4];
    int result;
    if (c->state->mode < 2)
        return 1;
    if (!c->navigation || !c->network || !c->network->transport)
        return fail(c, 1);
    w = (BHWorldGeometry *)word(c->player, 0xc4);
    if (!w)
        return 1;
    for (i = 0; i < c->navigation->node_count; ++i)
        if (c->navigation->nodes[i].placement == w)
            break;
    if (i == c->navigation->node_count)
        return fail(c, 1);
    if (!bh_packet_begin(&c->writer, c->packet, 64, 6, (unsigned short)c->state->tick))
        return fail(c, 2);
    result = bh_packet_write_position(&c->writer, w, (unsigned short)i, &c->player->position);
    if (result < 0)
        return fail(c, 2);
    if (!result)
        return 1;
    tail[0] = (unsigned char)resource;
    tail[1] = (unsigned char)(__int64)((double)level * 128);
    tail[2] = (unsigned char)(__int64)((double)pitch * 64);
    tail[3] = (unsigned char)sparks;
    if (!bh_packet_append(&c->writer, tail, 4))
        return fail(c, 2);
    bh_network_packet_send(c->network->transport, &c->writer, 0, 0);
    ++c->packets;
    c->packet_bytes = c->writer.cursor;
    return 1;
}
static void __cdecl receive(void *bytes, DWORD n, void *user)
{
    BHPlayerTick *c = (BHPlayerTick *)user;
    if (c->state->mode == 2)
        bh_remote_host_message(c->network, bytes, n, c->game->capacity);
    else
        bh_remote_client_message(c->network, bytes, n, c->game->capacity);
}
static int network(BHPlayerTick *c)
{
    BHPlayerTickState *s = c->state;
    unsigned long i, n = 0;
    unsigned char payload[15], flags;
    unsigned short tick;
    long converted;
    double sum = 0;
    BHVec3 angles;
    BHWorldGeometry *w;
    int result;
    if (!s->mode)
        return 1;
    ++s->tick;
    if (s->mode == 2)
        s->tick = (s->tick & 0xffff0000UL) | (unsigned short)(GetTickCount() / 33);
    if (!s->ending && !input(c))
        return 0;
    if (s->mode < 2)
        return 1;
    if (!c->network || !c->game)
        return fail(c, 1);
    if (s->debug)
    {
        if (!c->network_debug || !c->network->packets)
            return fail(c, 1);
        c->network_debug->mode = s->mode;
        c->network_debug->frame = (short)s->tick;
        c->network_debug->defcon = s->peers;
        c->network_debug->good = c->network->packets->processed;
        c->network_debug->bad = c->network->packets->bad;
        c->network_debug->ugly = c->network->packets->pending;
        if (!bh_network_debug_draw(c->network_debug))
            return fail(c, 3);
    }
    if (s->mode == 2)
    {
        *c->network->packets->local_slot = 0;
        if (!(s->tick & 63))
        {
            if (!c->latency)
                return fail(c, 1);
            for (i = 0; i < 7; ++i)
            {
                converted = (long)(__int64)((long)(short)(unsigned short)s->tick + (double)c->latency[i]);
                tick = (unsigned short)converted;
                memcpy(payload + i * 2, &tick, 2);
            }
            for (i = 1; i < 8; ++i)
                if (c->game->players[i].active)
                {
                    sum += c->latency[i];
                    ++n;
                }
            if (!n)
                return fail(c, 4);
            s->peers = (unsigned long)(__int64)(sum * 1.4 / n);
            payload[14] = (unsigned char)s->peers;
            if (!packet_send(c, 5, payload, 15))
                return 0;
        }
    }
    if (!bh_network_receive(c->game, receive, c))
        return fail(c, 3);
    return 1;
}
static int send_pose(BHPlayerTick *c)
{
    BHPlayerTickState *s = c->state;
    BHEntity *e = c->player;
    BHWorldGeometry *w;
    unsigned long i;
    unsigned short slot;
    unsigned char flags, angle;
    int result;
    if (s->mode < 2)
        return 1;
    if (s->send_delay)
    {
        --s->send_delay;
        return 1;
    }
    s->send_delay = (long)s->peers;
    if (!c->network || !c->navigation)
        return fail(c, 1);
    if (*c->network->packets->local_slot < 0)
        return 1;
    slot = (unsigned short)*c->network->packets->local_slot;
    w = (BHWorldGeometry *)word(e, 0xc4);
    if (!w)
        return 1;
    for (i = 0; i < c->navigation->node_count; ++i)
        if (c->navigation->nodes[i].placement == w)
            break;
    if (i == c->navigation->node_count)
        return fail(c, 1);
    if (!bh_packet_begin(&c->writer, c->packet, 64, 4, (unsigned short)s->tick) ||
        !bh_packet_append(&c->writer, &slot, 2))
        return fail(c, 2);
    result = bh_packet_write_position(&c->writer, w, (unsigned short)i, &e->position);
    if (result < 0)
        return fail(c, 2);
    if (!result)
        return 1;
    for (i = 0; i < 3; ++i)
    {
        angle = (unsigned char)(__int64)((double)number(e, 0x3c + i * 4) * 40.7436637878418);
        if (!bh_packet_append(&c->writer, &angle, 1))
            return fail(c, 2);
    }
    flags = number(e, 0x240) > 0 ? 1 : 0;
    if (word(e, 0x228) != BH_SOUND_INVALID)
        flags |= 2;
    if (half(e, 0x2b4))
        flags |= 4;
    if (!bh_packet_append(&c->writer, &flags, 1))
        return fail(c, 2);
    bh_network_packet_send(c->network->transport, &c->writer, 0, 0);
    ++c->packets;
    c->packet_bytes = c->writer.cursor;
    return 1;
}
static void cue(BHPlayerTick *c, unsigned long resource, float delay)
{
    BHEntity *e;
    BHScriptLifetimeState life;
    if (!c->lifetime)
    {
        c->error = 1;
        return;
    }
    e = bh_entity_acquire(c->pool);
    if (!e)
        return;
    bh_script_lifetime_read(e, &life);
    life.mission_countdown = delay;
    life.mission_resource = resource;
    if (!bh_script_lifetime_bind(c->lifetime, e, &life, 0))
    {
        bh_entity_release(c->pool, e);
        c->error = 1;
        return;
    }
    e->step = bh_mission_sound_tick;
}
static int recoil(BHPlayerTick *c)
{
    BHEntity *e = c->player;
    BHVec3 position = vector(e, 0x298), speed = vector(e, 0x2a4), difference, clamped, right = vector(e, 0x18),
           forward = vector(e, 0x30);
    float length, dx, dy;
    unsigned long i;
    for (i = 0; i < 3; ++i)
        ((float *)&position)[i] = rf((double)((float *)&position)[i] + ((float *)&speed)[i]);
    for (i = 0; i < 3; ++i)
        ((float *)&difference)[i] = rf((double)((float *)&e->position)[i] - ((float *)&position)[i]);
    length = rf(sqrt(((double)difference.z * difference.z + (double)difference.y * difference.y) +
                     (double)difference.x * difference.x));
    clamped = difference;
    if (length > .5)
        for (i = 0; i < 3; ++i)
            ((float *)&clamped)[i] = rf(((float *)&difference)[i] * (.5 / length));
    for (i = 0; i < 3; ++i)
    {
        ((float *)&position)[i] = rf((double)((float *)&clamped)[i] + ((float *)&position)[i]);
        ((float *)&speed)[i] = rf((((double)((float *)&speed)[i] - ((float *)&difference)[i] * (-.6))) * .6);
    }
    setvector(e, 0x298, &position);
    setvector(e, 0x2a4, &speed);
    for (i = 0; i < 3; ++i)
        ((float *)&difference)[i] = rf((double)((float *)&position)[i] - ((float *)&e->position)[i]);
    dx = rf(((double)right.y * difference.y + (double)right.z * difference.z) + (double)right.x * difference.x);
    dy = rf(((double)forward.x * difference.x + (double)forward.y * difference.y) + (double)forward.z * difference.z);
    if (!c->state->mode)
        dx = dy = 0;
    else
    {
        dx = rf((double)dx * (2.0 / 3));
        dy = rf((double)-dy * .25);
    }
    if (!c->feedback || bh_force_feedback_update(c->feedback, GetTickCount(), dx, dy) < 0)
        return fail(c, 3);
    return 1;
}

static int near_contacts(BHPlayerTick *c)
{
    BHEntity *e = c->player, *actor;
    BHWorldGeometry *world = (BHWorldGeometry *)word(e, 0xc4);
    BHPickupRoom *room = 0;
    BHEntityPositionPacket position;
    BHPhysicsContactPacket contact;
    BHRemoteQueryPacket damage;
    BHVec3 d, point, mid;
    unsigned long i, kind, n = 0;
    float distance, root, raw, strength, level, coefficient, particle;
    int random;
    if (!world)
        return 1;
    if (!c->rooms || !c->contacts || !c->flash)
        return fail(c, 1);
    for (i = 0; i < c->room_count; ++i)
        if (c->rooms[i].world == world)
        {
            room = c->rooms + i;
            break;
        }
    if (!room)
        return fail(c, 1);
    actor = room->actors;
    while (actor)
    {
        if (++n > c->pool->capacity || !actor->query)
            return fail(c, 5);
        position.event = 3;
        actor->query(actor, &position);
        for (i = 0; i < 3; ++i)
            ((float *)&d)[i] = rf((double)((float *)&position.position)[i] - ((float *)&e->position)[i]);
        distance = rf(((double)d.z * d.z + (double)d.y * d.y) + (double)d.x * d.x);
        if (distance < .1)
            distance = .1f;
        kind = word(actor, 0xfc);
        if (kind == 4)
            distance = rf((double)distance * .5f);
        else if (kind == 5)
            distance = rf((double)distance * .1666666716337204f);
        else if (kind == 9)
            distance = rf((double)distance * .25f);
        if (distance < 12.25)
        {
            ++c->near_contacts;
            if (kind == 9 && word(actor, 0x12c) == (unsigned long)actor && distance < 6.0025 && !word(actor, 0x134) &&
                !word(actor, 0x138))
            {
                setword(actor, 0x134, (unsigned long)e);
                setword(actor, 0x138, word(actor, 0x128));
                sound(c, c->resources.target_cue, 2, .7f, &e->position, &e->origin);
                sound(c, c->resources.lock_cue, 2, 1, &e->position, &e->origin);
            }
            root = rf(sqrt(distance));
            coefficient = rf(1.0 / root);
            contact.event = 2;
            for (i = 0; i < 3; ++i)
                ((float *)&contact.normal)[i] = rf(((float *)&d)[i] * (double)coefficient);
            raw = rf(1.0 - sqrt(distance) * (1.0 / 3.5));
            level = rf((double)raw + raw);
            strength = rf((double)raw * raw);
            for (i = 0; i < 3; ++i)
            {
                ((float *)&point)[i] = rf((double)((float *)&e->position)[i] - ((float *)&contact.normal)[i] * (-1.75));
                random = rand();
                ((float *)&point)[i] = rf((double)((float *)&point)[i] - ((double)random - 16384) * (-1.0 / 65536));
            }
            contact.point = point;
            contact.impulse = rf((double)strength * 3);
            contact.severity = rf((double)strength + strength);
            if (kind == 4 || kind == 5)
            {
                contact.impulse = rf((double)contact.impulse * 4);
                contact.severity = rf((double)contact.severity * 8);
                level = rf((double)level * 4);
                if (contact.impulse > 1)
                    contact.impulse = 1;
                if (contact.normal.y < -.5)
                    contact.normal.y = -.5f;
            }
            if (!e->query)
                return fail(c, 1);
            e->query(e, &contact);
            for (i = 0; i < 3; ++i)
                ((float *)&contact.normal)[i] = -((float *)&contact.normal)[i];
            contact.impulse = rf((double)strength * (-200));
            contact.severity = 0;
            actor->query(actor, &contact);
            damage.damage.event = 0;
            damage.damage.severity = rf((double)strength * 1.5);
            damage.damage.attacker = e;
            if (number(e, 0x240) > 0)
            {
                damage.damage.severity = rf((double)damage.damage.severity * 3);
                for (i = 0; i < 3; ++i)
                    ((float *)&mid)[i] =
                        rf(((double)((float *)&e->position)[i] + ((float *)&position.position)[i]) * .5);
                {
                    BHEntity *flash = bh_entity_acquire(c->pool);
                    if (flash)
                    {
                        flash->position = mid;
                        bh_flash_flare_bind(c->flash, flash, .1f, 200, 4);
                    }
                }
                sound(c, c->resources.electro, 1, 1.5f, &mid, &zero);
            }
            if (word(actor, 0xfc) != 8)
                actor->query(actor, &damage);
            if (!sound_packet(c, c->resources.collision_side, level, rf(.8 - (double)strength * (-.4)),
                              (unsigned long)(__int64)((double)strength * 5)))
                return 0;
            sound(c, c->resources.collision_side, level, rf(.8 - (double)strength * (-.4)), &point, &zero);
            particle = strength;
            while (particle > .1)
            {
                bh_contact_spark_create(c->contacts, &point, particle, 1);
                particle = rf((double)particle - .2);
            }
        }
        actor = (BHEntity *)word(actor, 0xf4);
    }
    return 1;
}
static void __cdecl ring_refresh(BHEntity *e, void *unused)
{
    BHPlayerTick *c;
    (void)unused;
    memcpy(&c, e->payload + 528, 4);
    if (c && c->contacts && c->camera_yaw)
        bh_impact_ring_draw(c->contacts->queue, c->resources.water_ring, e, c->camera_yaw);
}
static void ring(BHPlayerTick *c, const BHVec3 *position, const BHVec3 *normal, float size)
{
    BHEntity *e;
    BHImpactState *s;
    if (!c->contacts || !c->camera_yaw)
    {
        c->error = 1;
        return;
    }
    e = bh_contact_impact_create(c->contacts, position, normal, 4);
    if (!e)
        return;
    s = bh_contact_impact_state(c->contacts, e);
    s->size = size;
    s->duration = rf((double)size * .1);
    memcpy(e->payload + 528, &c, 4);
    e->refresh = ring_refresh;
}
static int ground(BHPlayerTick *c)
{
    BHEntity *e = c->player;
    BHVec3 start, end, right, forward, up, side, a, b, d, position, velocity, acceleration;
    BHWorldGeometry *cached;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    unsigned long handle, i, image;
    int found = 0, result, n, j, random;
    float fraction, strength, cosine, sine, radius, width, fall, angle, size, rate, alpha;
    double raw_angle;
    if (!getbyte(e, 0x2bb))
        return 1;
    handle = word(e, 0x228);
    if (!half(e, 0x2b4))
    {
        if (!c->world || !c->particles || !c->contacts)
            return fail(c, 1);
        up = vector(e, 0x24);
        right = vector(e, 0x18);
        forward = vector(e, 0x30);
        for (i = 0; i < 3; ++i)
        {
            ((float *)&start)[i] = rf((double)((float *)&e->position)[i] - ((float *)&up)[i] * (-.3));
            ((float *)&end)[i] = rf((double)((float *)&e->position)[i] - ((float *)&up)[i] * (-8.3));
        }
        cached = (BHWorldGeometry *)word(e, 0xc4);
        self.kind = BH_WORLD_HIT_ACTOR;
        self.object.actor = e;
        result = bh_world_segment_run(c->world, &self, &cached, &start, &end, -1, &hit);
        if (result < 0)
            return fail(c, 3);
        if (result)
        {
            fraction = hit.fraction;
            if (!_finite(fraction) || fraction < 0 || fraction > 1)
                return fail(c, 4);
            strength = rf(1.0 - fraction);
            random = rand();
            n = (int)(1 - (1 - (double)fraction) * random * (-1.0 / 8192));
            if (handle == BH_SOUND_INVALID)
            {
                handle = sound(c, c->resources.ground_loop, strength, 1, &hit.point, &e->origin);
                setword(e, 0x228, handle);
            }
            else
                bh_sound_update(c->sound, handle, strength, 1, &hit.point, &e->origin);
            for (j = n; j;)
            {
                --j;
                found = 1;
                random = rand();
                raw_angle = (double)random * .00019175345108273588;
                cosine = rf(cos(raw_angle));
                sine = rf(sin(raw_angle));
                bh_vec3_cross(&right, &hit.normal, &a);
                bh_vec3_cross(&forward, &hit.normal, &b);
                for (i = 0; i < 3; ++i)
                    ((float *)&d)[i] = rf((double)cosine * ((float *)&a)[i] + (double)sine * ((float *)&b)[i]);
                bh_vec3_normalize(&d);
                if (!j && (hit.material_flags & 4))
                {
                    random = rand();
                    if (random < 8192)
                    {
                        random = rand();
                        size = rf(14.0 - (double)fraction * 10);
                        fall = rf((double)random * (1.0 / 8192));
                        for (i = 0; i < 3; ++i)
                            ((float *)&position)[i] = rf((double)fall * ((float *)&d)[i] + ((float *)&hit.point)[i]);
                        ring(c, &position, &hit.normal, size);
                    }
                }
                radius = rf((1 - (double)fraction) * .25);
                width = rf(2.5 - ((double)fraction + fraction));
                fall = rf((1 - (double)fraction) * (-.02));
                for (i = 0; i < 3; ++i)
                {
                    ((float *)&velocity)[i] = rf((double)((float *)&d)[i] * radius);
                    ((float *)&acceleration)[i] = rf((double)((float *)&velocity)[i] * (-1.0 / 15));
                    ((float *)&position)[i] = rf((double)((float *)&d)[i] * width + ((float *)&hit.point)[i]);
                }
                acceleration.y = rf((double)acceleration.y + fall);
                image = c->resources.smoke_image;
                size = .5f;
                rate = 20;
                alpha = radius;
                if (hit.material_flags & 4)
                {
                    image = c->resources.water_cloud;
                    size = .7f;
                    rate = 30;
                    alpha = strength;
                }
                random = rand();
                angle = rf((double)random * .00019175345108273588);
                bh_script_particle_create(c->particles, image, 3, &position, &velocity, &acceleration, size, angle,
                                          rate, 0, .8f, .9f, rf((1 - (double)fraction) * .5), alpha);
                ++c->ground_particles;
            }
        }
    }
    if (!found && handle != BH_SOUND_INVALID)
    {
        bh_sound_stop(c->sound, handle);
        setword(e, 0x228, BH_SOUND_INVALID);
    }
    return !c->error;
}
static int weapons(BHPlayerTick *c)
{
    BHEntity *e = c->player, *w;
    BHWeaponObjectState *state;
    unsigned long i, j, old, to, slot;
    int first, held, k;
    if (!c->weapons || !c->weapons->factory)
        return fail(c, 1);
    for (j = 0; j < 2; ++j)
        if (key(c, 27 + j) == 0)
        {
            old = getbyte(e, 0x2b6 + j);
            if (old < j * 5 || old > j * 5 + 4)
                return fail(c, 1);
            to = old;
            for (i = 0; i < 5; ++i)
            {
                to = to + 1;
                if (to > j * 5 + 4)
                    to = j * 5;
                setbyte(e, 0x2b6 + j, (unsigned char)to);
                if (!bh_weapon_registry_resolve(&c->weapons->factory->registry, to, &w, &state))
                    return fail(c, 1);
                if (state->common.available_ec)
                    break;
            }
            if (i == 5)
                return fail(c, 5);
            if (old == to)
                sound(c, c->resources.no_weapon, 1, 1, &e->position, &e->origin);
            else if (!bh_weapon_dispatch(c->weapons, old, BH_WEAPON_CANCEL) ||
                     !bh_weapon_dispatch(c->weapons, to, BH_WEAPON_READY))
                return fail(c, 3);
        }
    for (i = 0; i < 10; ++i)
        if (key(c, 12 + i) == 0)
        {
            slot = i < 5 ? 0 : 1;
            old = getbyte(e, 0x2b6 + slot);
            if (old != i)
            {
                if (!bh_weapon_registry_resolve(&c->weapons->factory->registry, i, &w, &state))
                    return fail(c, 1);
                if (!state->common.available_ec)
                    sound(c, c->resources.no_weapon, 1, 1, &e->position, &e->origin);
                else
                {
                    if (!bh_weapon_dispatch(c->weapons, old, BH_WEAPON_CANCEL))
                        return fail(c, 3);
                    setbyte(e, 0x2b6 + slot, (unsigned char)i);
                    if (!bh_weapon_dispatch(c->weapons, i, BH_WEAPON_READY))
                        return fail(c, 3);
                }
            }
        }
    for (j = 0; j < 2; ++j)
        if (!bh_weapon_dispatch(c->weapons, getbyte(e, 0x2b6 + j), BH_WEAPON_TICK))
            return fail(c, 3);
    for (j = 0; j < 2; ++j)
    {
        first = held = 0;
        for (i = 0; i < 3; ++i)
        {
            slot = j ? (i == 0 ? 3 : i == 1 ? 5 : 4) : (i == 0 ? 22 : i == 1 ? 24 : 23);
            k = key(c, slot);
            if (!k)
                first = held = 1;
            else if (k == 2)
                held = 1;
        }
        if (first && !bh_weapon_dispatch(c->weapons, getbyte(e, 0x2b6 + j), BH_WEAPON_TRIGGER))
            return fail(c, 3);
        if (held && !bh_weapon_dispatch(c->weapons, getbyte(e, 0x2b6 + j), BH_WEAPON_FIRE))
            return fail(c, 3);
    }
    return 1;
}
static int camera_mode(BHPlayerTick *c, long mode)
{
    return bh_camera_state_set_mode(bh_player_camera_state(c->player), mode) ? 1 : fail(c, 3);
}
static int death(BHPlayerTick *c)
{
    BHPlayerTickState *s = c->state;
    BHEntity *e = c->player;
    BHVec3 position, acceleration, v;
    float distance, best = 9e37f;
    unsigned long i, j, index = 0;
    if (!s->death_frames)
        return 1;
    position = e->position;
    for (i = 0; i < 3; ++i)
        ((float *)&position)[i] = rf((double)((float *)&position)[i] + (rand() - 16384) * (1.0 / 16384));
    ++s->death_frames;
    if (s->death_frames == 75 || s->death_frames == 95)
    {
        if (!c->explosions)
            return fail(c, 1);
        acceleration = vector(e, 0x60);
        if (s->death_frames == 75)
        {
            sound(c, c->resources.death_cue, 1, 1.5f, &e->position, &e->origin);
            bh_script_explosion_create(c->explosions, &position, 1, 10, .85f, 0);
            s->saved_model = word(e, 0x21c);
            setword(e, 0x21c, c->resources.death_model);
            bh_sound_stop(c->sound, word(e, 0x224));
            setword(e, 0x224, BH_SOUND_INVALID);
        }
        else
        {
            setword(e, 0x21c, 0);
            bh_script_explosion_create(c->explosions, &e->position, 1.2f, 14, .85f, 0);
        }
        setvector(e, 0x60, &acceleration);
        if (c->explosions->error)
            return fail(c, 3);
    }
    if (s->death_frames > 75)
        for (j = 0; j < 2; ++j)
            for (i = 0; i < 3; ++i)
                real(e, 0x48 + j * 24 + i * 4, rf((double)number(e, 0x48 + j * 24 + i * 4) * .9));
    if (s->death_frames == 200)
    {
        if (s->mode == 1 && !s->ending)
        {
            s->confirmation = 0;
            return return_menu(c) ? 2 : 0;
        }
        if (!c->camera || c->camera->start_count < 8 || !c->reset)
            return fail(c, 1);
        setword(e, 0x21c, s->saved_model);
        s->confirmation = s->death_frames = 0;
        for (i = 0; i < 8; ++i)
        {
            for (j = 0; j < 3; ++j)
                ((float *)&v)[j] =
                    rf((double)((float *)&e->position)[j] - ((const float *)&c->camera->starts[i].position)[j]);
            distance = rf(sqrt(((double)v.z * v.z + (double)v.y * v.y) + (double)v.x * v.x));
            if (distance < best)
            {
                best = distance;
                index = i;
            }
        }
        if (!camera_reset(c, index) || !bh_player_reset(c->reset))
            return fail(c, 3);
        real(e, 0x240, 5);
        real(e, 0x244, 1);
        real(e, 0x248, 5);
        sethalf(e, 0x2b0, 0);
        sethalf(e, 0x2b2, 0);
        if (!camera_mode(c, 0))
            return 0;
    }
    return 1;
}
static int camera_cheats(BHPlayerTick *c)
{
    BHEntity *e = c->player, *w;
    BHWeaponObjectState *weapon;
    BHCameraInput in;
    static const long modes[6] = {0, 1, 7, 9, 8, 4};
    unsigned long i, bits;
    long ammo;
    if (getbyte(e, 0x2bb))
    {
        for (i = 0; i < 6; ++i)
            if (!key(c, 6 + i))
            {
                sethalf(e, 0x2b0, (unsigned short)modes[i]);
                if (!camera_mode(c, modes[i]))
                    return 0;
                sound(c, c->resources.ui_cue, 1, 2, 0, 0);
            }
        if (!c->camera || !c->camera->camera)
            return fail(c, 1);
        in.target = e->position;
        in.angles = vector(e, 0x3c);
        in.basis_x = vector(e, 0x18);
        in.basis_y = vector(e, 0x24);
        in.basis_z = vector(e, 0x30);
        in.actor_token = (unsigned long)e;
        if (!bh_camera_state_update(c->camera->camera, bh_player_camera_state(e), &in, 1.0 / 30, number(e, 0x1f0),
                                    number(e, 0x1f4)))
            return fail(c, 3);
    }
    if (!key(c, 31) && c->state->mode == 1)
        setword(e, 0x21c, c->resources.cheat_model);
    if (!key(c, 29) && c->state->mode == 1)
        for (i = 0; i < 10; ++i)
        {
            if (!bh_weapon_registry_resolve(&c->weapons->factory->registry, i, &w, &weapon))
                return fail(c, 1);
            weapon->common.available_ec = 1;
            bits = (unsigned long)weapon->common.ammo + 999;
            memcpy(&ammo, &bits, 4);
            if (ammo > weapon->common.ammo_limit)
                ammo = weapon->common.ammo_limit;
            weapon->common.ammo = ammo;
        }
    if (!key(c, 30) && c->state->mode == 1)
        c->state->god_mode = !c->state->god_mode;
    if (c->state->god_mode)
    {
        real(e, 0x23c, 0);
        real(e, 0x24c, 0);
        real(e, 0x240, 1);
    }
    return 1;
}
static int damage_timers(BHPlayerTick *c)
{
    BHEntity *e = c->player, *w;
    BHWeaponObjectState *weapon;
    BHPlayerTickState *s = c->state;
    BHVec3 velocity, acceleration = {0, -.002f, 0}, forward = vector(e, 0x30), motion = vector(e, 0x48);
    float speed, spin, angle, pitch, timer;
    unsigned long i, image, handle;
    short countdown;
    speed = rf(((double)forward.x * motion.x + (double)forward.z * motion.z) + (double)forward.y * motion.y);
    bh_sound_update(c->sound, word(e, 0x220), rf(.3 + (double)number(e, 0x2f8) * .05), rf(.5 + fabs(speed) * .01),
                    &e->position, &motion);
    if (half(e, 0x2b4) && rand() < 16384 && s->death_frames < 95)
    {
        if (!c->particles)
            return fail(c, 1);
        for (i = 0; i < 3; ++i)
            ((float *)&velocity)[i] = rf((rand() - 16384) * (1.0 / 262144));
        image = number(e, 0x24c) == 1 ? c->resources.second_smoke : c->resources.smoke_image;
        spin = rf((rand() - 16384) * (1.0 / 16384));
        angle = rf(rand() * (1.0 / 512));
        bh_script_particle_create(c->particles, image, 3, &e->position, &velocity, &acceleration, .5f, angle, 12, spin,
                                  .9f, .95f, 2, .3f);
    }
    if (!half(e, 0x2b4))
    {
        if (word(e, 0x23c) == 0x3f800000UL)
        {
            if (s->mode > 1)
            {
                pitch = rf(.75 - (double)rand() * (-1.0 / 65536));
                if (!sound_packet(c, c->resources.death_cue, 0, pitch, 0))
                    return 0;
            }
            if (getbyte(e, 0x2bb))
                setword(e, 0x224, sound(c, c->resources.death_loop, .8f, 1, 0, 0));
            sethalf(e, 0x2b4, 1);
            real(e, 0x24c, rf((double)number(e, 0x24c) + .2));
            if (s->mode > 1)
            {
                if (!c->scores || !c->damage_source || !c->network || !c->network->packets)
                    return fail(c, 1);
                c->scores->mode = s->mode;
                c->scores->frame = (unsigned short)s->tick;
                c->scores->local_player = e;
                if (!bh_network_scores_death(c->scores, *c->network->packets->local_slot, *c->damage_source))
                    return fail(c, 3);
            }
            if (number(e, 0x24c) >= 1)
            {
                real(e, 0x24c, 1);
                if (!s->death_frames)
                    s->death_frames = 1;
                s->confirmation = 0;
                sound(c, c->resources.death_cue, 2, .8f, &e->position, &motion);
            }
            sethalf(e, 0x2b2, (unsigned short)-3);
        }
    }
    else if (number(e, 0x23c) < .5 && number(e, 0x24c) < 1)
    {
        if (getbyte(e, 0x2bb) && word(e, 0x224) != BH_SOUND_INVALID)
            bh_sound_stop(c->sound, word(e, 0x224));
        setword(e, 0x224, BH_SOUND_INVALID);
        sethalf(e, 0x2b4, 0);
        if (s->mode > 1)
        {
            timer = number(e, 0x240);
            if (timer == 0)
            {
                real(e, 0x244, 1);
                real(e, 0x240, 5);
                real(e, 0x248, 5);
            }
            else if (timer < 5)
            {
                real(e, 0x240, 5);
                real(e, 0x248, 5);
            }
        }
        sethalf(e, 0x2b2, 30);
        if (number(e, 0x24c) >= .8 && !s->repair_warning)
        {
            s->repair_warning = 1;
            if (!c->panel || !c->panel->messages)
                return fail(c, 1);
            if (!bh_message_enqueue(
                    c->panel->messages,
                    "4Hey Buddy!\n\nYour car looks\npretty wrecked!\n\n\nDrop by one of our\nrepair shops and we'll\nmake you an offer.\n"))
                return fail(c, 3);
        }
    }
    real(e, 0x23c, rf((double)number(e, 0x23c) * .995));
    countdown = half(e, 0x2b2);
    if (countdown)
    {
        if (countdown < 0)
        {
            ++countdown;
            sethalf(e, 0x2b2, (unsigned short)countdown);
            if (!countdown && !camera_mode(c, s->death_frames ? 8 : 9))
                return 0;
        }
        else
        {
            --countdown;
            sethalf(e, 0x2b2, (unsigned short)countdown);
            if (!countdown && !camera_mode(c, half(e, 0x2b0)))
                return 0;
        }
    }
    timer = number(e, 0x240);
    if (timer > 0)
    {
        motion = vector(e, 0x48);
        handle = word(e, 0x22c);
        if (handle == BH_SOUND_INVALID)
        {
            sound(c, c->resources.boost_start, .6f, .6f, &e->position, &motion);
            setword(e, 0x22c, sound(c, c->resources.boost_loop, .6f, .6f, &e->position, &motion));
        }
        else
            bh_sound_update(c->sound, handle, .6f, .6f, &e->position, &motion);
        if (timer <= number(e, 0x248))
        {
            real(e, 0x248, rf((double)timer * .9));
            pitch = rf(1.2 - (double)number(e, 0x248) * (1.0 / 30));
            sound(c, c->resources.menu_cue, .5f, pitch, 0, 0);
            sound(c, c->resources.menu_cue, .5f, rf((double)pitch * .5), 0, 0);
            sound(c, c->resources.ui_cue, .5f, pitch, 0, 0);
        }
        if (number(e, 0x244) > 0)
            real(e, 0x244, rf((double)number(e, 0x244) - 1.0 / 30));
        timer = rf((double)timer - 1.0 / 30);
        real(e, 0x240, timer);
        real(e, 0x23c, 0);
        if (timer < 1 && timer >= .9666666666666667)
        {
            bh_sound_stop(c->sound, word(e, 0x22c));
            setword(e, 0x22c, BH_SOUND_INVALID);
            sound(c, c->resources.boost_end, .6f, .6f, &e->position, &motion);
        }
        if (timer <= 0)
        {
            real(
                e, 0x240,
                0); /* Source lifetime fix: the original threshold stop can be followed by a restart during the final second. Retire the owned voice at actual expiry. */
            handle = word(e, 0x22c);
            if (handle != BH_SOUND_INVALID)
            {
                bh_sound_stop(c->sound, handle);
                setword(e, 0x22c, BH_SOUND_INVALID);
            }
        }
    }
    if (!(getbyte(e, 0x2b8) & 7))
    {
        if (!bh_weapon_registry_resolve(&c->weapons->factory->registry, 0, &w, &weapon))
            return fail(c, 1);
        if (weapon->common.ammo < 200)
        {
            ++weapon->common.ammo;
            if (weapon->common.ammo > weapon->common.ammo_limit)
                weapon->common.ammo = weapon->common.ammo_limit;
        }
    }
    if (half(e, 0x2b4))
        sethalf(e, 0x2b4, (unsigned short)(half(e, 0x2b4) + 1));
    return 1;
}
int __cdecl bh_player_tick(BHPlayerTick *c)
{
    BHEntity *e;
    BHPlayerTickState *s;
    BHMessageState *messages;
    int k, result;
    long last;
    if (!c || !c->player || !c->state)
        return 0;
    c->error = 0;
    ++c->calls;
    e = c->player;
    s = c->state;
    if (s->paused)
    {
        if (!input(c))
            return 0;
        if (!key(c, 35) || !key(c, 33))
        {
            if (!s->mode)
                return 1;
            if (s->confirmation)
            {
                restore_volume(c);
                menu_cue(c, .7f, .5f);
                s->confirmation = 0;
            }
        }
        if (!s->mode)
            return 1;
        k = key(c, 34);
        if ((k != 0 || !s->confirmation) && s->ending != 2)
            return 1;
        restore_volume(c);
        if (!s->ending)
            menu_cue(c, .7f, .9f);
        s->ending = s->confirmation = 0;
        return return_menu(c);
    }
    if (s->world_mode == 1)
        return 1;
    if (!c->pool || !c->body || !c->physics || !c->force || !c->decoded || !c->sound)
        return fail(c, 1);
    if (s->force != bh_player_force_control_a && s->force != bh_player_force_control_b &&
        s->force != bh_player_force_control_c && s->force != bh_player_force_control_d)
        return fail(c, 1);
    setbyte(e, 0x2b8, (unsigned char)(getbyte(e, 0x2b8) + 1));
    if (!network(c))
        return 0;
    if (c->body->entity != e || c->force->body != c->body || !bh_physics_step_read_entity(c->body->physics, e))
        return fail(c, 1);
    c->body->world = (BHWorldGeometry *)word(e, 0xc4);
    if (!bh_physics_step(c->physics, e, 1.0f / 30) || !s->force(c->force, 1.0f / 30, half(e, 0x2b4)) ||
        !bh_physics_step_publish(c->body) || !recoil(c) || !send_pose(c))
        return fail(c, c->error ? c->error : 3);
    if (!s->damage_warning)
    {
        if (number(e, 0x23c) >= .6 && !half(e, 0x2b4))
        {
            s->damage_warning = 1;
            cue(c, c->resources.damage_warning, .5f);
        }
    }
    else if (number(e, 0x23c) <= .05)
        s->damage_warning = 0;
    if (!s->hull_warning)
    {
        if (number(e, 0x24c) >= .8)
        {
            s->hull_warning = 1;
            cue(c, c->resources.hull_warning, 1);
        }
    }
    else if (number(e, 0x24c) <= .2)
        s->hull_warning = 0;
    if (c->error || !bh_player_aim_update(c->body) || !near_contacts(c) || !ground(c))
        return fail(c, c->error ? c->error : 3);
    if (!key(c, 32))
    {
        if (!c->panel || !c->panel->messages)
            return fail(c, 1);
        messages = c->panel->messages;
        last = messages->last_index;
        if (last == -1)
            last = (messages->ring_count - 1 + messages->ring_start) % 8;
        else if (last == messages->ring_start)
            last = -1;
        else
        {
            --last;
            if (last < 0)
                last += 8;
        }
        messages->last_index = last;
        messages->display_phase = 1;
        messages->display_ticks = 40;
        sound(c, c->resources.ui_cue, 1, 2, 0, 0);
    }
    if (!key(c, 33) && s->mode && number(e, 0x24c) < 1)
    {
        menu_cue(c, .8f, .6f);
        s->confirmation = 1;
        if (s->mode == 1 && !s->paused)
        {
            if (!c->media)
                return fail(c, 1);
            s->paused = 1;
            s->saved_cd_volume = (float)bh_cd_audio_volume(c->media->audio);
            bh_cd_set_volume(c->media, 0);
            s->saved_wave_volume = (float)bh_wave_audio_volume(c->media->audio);
            bh_wave_set_volume(c->media, 0);
        }
    }
    k = key(c, 35);
    if (!k && s->mode && s->confirmation)
    {
        restore_volume(c);
        menu_cue(c, .7f, .5f);
        s->confirmation = 0;
    }
    if ((k != 0 || s->mode) && s->mode)
    {
        k = key(c, 34);
        if ((!k && s->confirmation) || s->ending == 2)
        {
            restore_volume(c);
            if (!s->ending)
                menu_cue(c, .7f, .9f);
            s->ending = s->confirmation = 0;
            return return_menu(c);
        }
    }
    result = death(c);
    if (!result)
        return 0;
    if (result == 2)
        return 1;
    if (!weapons(c) || !camera_cheats(c) || !damage_timers(c))
        return 0;
    if (!bh_physics_step_read_entity(c->body->physics, e))
        return fail(c, 3);
    c->body->world = (BHWorldGeometry *)word(e, 0xc4);
    return !c->error;
}
int __cdecl bh_player_tick_bind(BHPlayerTick *c, BHEntity *e)
{
    BHEntity *p;
    unsigned long n = 0;
    if (!c || !e || !c->pool || bound_player)
        return 0;
    for (p = c->pool->active; p && n++ < c->pool->capacity; p = p->next)
        if (p == e)
            break;
    if (p != e)
        return 0;
    c->player = e;
    c->generation = e->generation;
    bound_player = c;
    e->step = bh_player_step;
    return 1;
}
BHPlayerTick *__cdecl bh_player_tick_context(BHEntity *e)
{
    BHPlayerTick *c = bound_player;
    if (!c || c->player != e || !c->pool)
        return 0;
    return bh_entity_is_active(c->pool, e) && e->generation == c->generation ? c : 0;
}
int __cdecl bh_player_tick_unbind(BHPlayerTick *c)
{
    if (!c || bound_player != c)
        return 0;
    if (bh_player_tick_context(c->player) && c->player->step == bh_player_step)
        c->player->step = 0;
    bound_player = 0;
    return 1;
}
void __cdecl bh_player_step(BHEntity *e)
{
    BHPlayerTick *c = bh_player_tick_context(e);
    if (c)
        bh_player_tick(c);
}
