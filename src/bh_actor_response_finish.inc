/* Remaining death/healthy return of00439910. Source lifetime/legacy-angle and
   checked registry adapters are explicit; they are not new original owners. */
#define RESPONSE_FINISH_VEC(o) ((BHVec3 *)((char *)e + (o)))
#define RESPONSE_FINISH_CURVE(t, p)                                                                                    \
    bh_camera_curve((t), (p), RESPONSE_FINISH_VEC(0x120), RESPONSE_FINISH_VEC(0x144), RESPONSE_FINISH_VEC(0x150),      \
                    RESPONSE_FINISH_VEC(0x12c))
void __cdecl bh_actor_response_finish(BHEntity *e, BHActorResponseContext *c, const BHVec3 *death_position, int smoked)
{
    BHActorDamageContext *d = c->damage;
    BHVec3 before, at, after, velocity, spin;
    BHEntity *mission, *owner;
    unsigned long type, token, material, special;
    float t, strength;
    int draw;
    long first = -1, second = -1, row, loot = 0;
    short slot;
    struct
    {
        long event;
        BHEntity *actor;
        long tag;
    } packet;
    if (!smoked || number(e, 0x194) < 1)
    {
        if (number(e, 0x194) < .05 && word(e, 0x198) == 0x3f800000UL && word(e, 0x19c) == 0x3f800000UL)
            e->step = bh_actor_vehicle_tick;
        return;
    }
    if (!d->legacy_death_angles)
    {
        c->error = 1;
        return;
    }
    bh_entity_set_query(d->pool, e, 0);
    bh_sound_stop(d->route->sound, word(e, 0x104));
    t = response_query_stored((double)number(e, 0x180) / number(e, 0x184));
    RESPONSE_FINISH_CURVE(response_query_stored((double)t - 1.0 / 30.0), &before);
    RESPONSE_FINISH_CURVE(t, &at);
    RESPONSE_FINISH_CURVE(response_query_stored((double)t - (-1.0 / 30.0)), &after);
    /* 00439e7a..00439edc has the same retained-X/spilled-YZ contract as
       00435bf9..00435c52. Its original-byte oracle is recorded separately. */
    bh_actor_damage_death_velocity(&after, &at, &velocity);
    bh_script_debris_create(d->fragments, (rand() & 7) + 1, death_position, 8, 10, 1);
    strength = response_query_stored(.75 - (double)rand() * (-1.0 / 65536.0));
    bh_sound_request(d->route->sound, d->death_sound, 2, strength, death_position, &velocity);
    spin.x = response_query_stored(((double)rand() - 16384) * (1.0 / 4096.0));
    spin.y = response_query_stored(((double)rand() - 16384) * (1.0 / 4096.0));
    spin.z = response_query_stored(((double)rand() - 16384) * (1.0 / 4096.0));
    slot = short_value(e, 0x1dc);
    if (!slot)
        first = 10;
    else if (slot == 1)
        first = 12;
    else if (slot == 2)
        first = 11;
    slot = short_value(e, 0x1de);
    if (!slot)
        second = 5;
    else if (slot == 1)
        second = 3;
    else if (slot == 2)
        second = 6;
    type = word(e, 0xfc);
    row = type == 6 ? 3 : type == 1 ? 1 : 2;
    if (!bh_remote_choose_slot(d->loot_rows, d->loot_row_count, row, first, second, &loot))
    {
        c->error = 1;
        return;
    }
    /* The original reloads kind after loot, and again after cue allocation. */
    if (word(e, 0xfc) == 6 && (mission = bh_entity_acquire(d->pool)) != 0)
    {
        if (!bh_script_lifetime_bind_context(c->mission, mission))
        {
            bh_entity_release(d->pool, mission);
            c->error = 1;
            return;
        }
        *(volatile float *)((char *)mission + 0x180) = 1.5f;
        *(volatile unsigned long *)((char *)mission + 0x104) = c->mission_resource;
        *(BHEntityStep volatile *)&mission->step = bh_mission_sound_tick;
    }
    special = word(e, 0xfc) == 6;
    token = word(e, 0x100);
    owner = (BHEntity *)word(e, 0x1ac);
    draw = rand();
    material = word(e, 0x114);
    strength = response_query_stored(1.0 - (double)draw * (-1.0 / 16384.0));
    bh_target_construct_moving(d->targets, death_position, d->legacy_death_angles, &velocity, &spin, material, 10000,
                               strength, owner, (long)token, loot, special);
    packet.event = 17;
    packet.actor = e;
    bh_entities_query(d->pool, 0, 0, 0, 0, &packet);
    if (word(e, 0x11c))
    {
        packet.event = 14;
        packet.tag = (long)word(e, 0x11c);
        bh_entities_query(d->pool, 0, 0, 0, 0, &packet);
    }
    if (unlink_actor(c, e))
        bh_entity_release(d->pool, e);
}
#undef RESPONSE_FINISH_VEC
#undef RESPONSE_FINISH_CURVE
