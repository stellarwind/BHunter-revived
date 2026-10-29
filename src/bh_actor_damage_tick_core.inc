/* Complete004357d0 source core. Source clock/registry/legacy-angle bindings
   remain explicit adapters. No original image code enters this target. */
#define DAMAGE_TICK_FLOAT(o) (*(float *)((char *)e + (o)))
#define DAMAGE_TICK_STORE(o, v) (*(volatile float *)((char *)e + (o)) = damage_query_stored(v))
#define DAMAGE_TICK_VEC(o) ((BHVec3 *)((char *)e + (o)))
#define DAMAGE_TICK_CURVE(t, p) \
    bh_camera_curve((t), (p), DAMAGE_TICK_VEC(0x120), DAMAGE_TICK_VEC(0x144), \
                    DAMAGE_TICK_VEC(0x150), DAMAGE_TICK_VEC(0x12c))
static float damage_tick_phase(BHActorDamageContext *c, BHEntity *e)
{
    unsigned short elapsed = (unsigned short)(bh_actor_route_now(c->route) - *(unsigned short *)((char *)e + 0x1e4));
    return damage_query_stored((double)(short)elapsed / *(short *)((char *)e + 0x1e6));
}
void __cdecl bh_actor_damage_tick_core(BHEntity *e, BHActorDamageContext *c)
{
    BHVec3 position, death_position, velocity, acceleration, before, at, after, spin;
    BHPickupRoom *room;
    BHEntity *previous, *next, *owner;
    double dx, dy, dz, distance, damping;
    float x, y, z, length, t, angular, angle, strength;
    unsigned long random, material;
    int draw;
    long loot = 0, token;
    DAMAGE_TICK_STORE(0x194, (double)DAMAGE_TICK_FLOAT(0x194) * .9975);
    dx = (double)DAMAGE_TICK_FLOAT(0x15c) - DAMAGE_TICK_FLOAT(0x168) * (-1.0 / 30.0);
    dy = (double)DAMAGE_TICK_FLOAT(0x160) - DAMAGE_TICK_FLOAT(0x16c) * (-1.0 / 30.0);
    dz = (double)DAMAGE_TICK_FLOAT(0x164) - DAMAGE_TICK_FLOAT(0x170) * (-1.0 / 30.0);
    x = damage_query_stored(dx);y = damage_query_stored(dy);z = damage_query_stored(dz);
    distance = sqrt((dx * x + dy * y) + dz * z);
    DAMAGE_TICK_STORE(0x15c, x);
    length = damage_query_stored(distance);
    DAMAGE_TICK_STORE(0x160, y);
    DAMAGE_TICK_STORE(0x164, z);
    damping = (.99 - distance * distance * .001) - (double)length * .01;
    if (!(damping >= 0)) damping = 0;
    DAMAGE_TICK_STORE(0x15c, damping * dx);
    DAMAGE_TICK_STORE(0x160, damping * y);
    DAMAGE_TICK_STORE(0x164, damping * z);
    DAMAGE_TICK_STORE(0x168, (double)DAMAGE_TICK_FLOAT(0x168) * .7);
    DAMAGE_TICK_STORE(0x16c, (double)DAMAGE_TICK_FLOAT(0x16c) * .7);
    DAMAGE_TICK_STORE(0x170, (double)DAMAGE_TICK_FLOAT(0x170) * .7);
    bh_actor_route_due(c->route, e);
    if (DAMAGE_TICK_FLOAT(0x194) < 1)
    {
        draw = rand();
        if ((double)DAMAGE_TICK_FLOAT(0x194) * 5000 <= draw) goto resume;
    }
    acceleration.x = 0;acceleration.y = -.002f;acceleration.z = 0;
    velocity.x = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.y = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.z = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    t = damage_tick_phase(c, e);
    DAMAGE_TICK_CURVE(t, &position);
    position.x = damage_query_stored((double)DAMAGE_TICK_FLOAT(0x15c) + position.x);
    position.y = damage_query_stored((double)DAMAGE_TICK_FLOAT(0x160) + position.y);
    position.z = damage_query_stored((double)DAMAGE_TICK_FLOAT(0x164) + position.z);
    death_position = position;
    angular = damage_query_stored((double)(rand() - 16384) * (1.0 / 16384.0));
    angle = damage_query_stored((double)rand() * (1.0 / 512.0));
    bh_script_particle_create(c->particles, c->smoke_resource, 3, &position, &velocity, &acceleration,
                              .5f, angle, 12, angular, .9f, .95f, 2, .3f);
    if (DAMAGE_TICK_FLOAT(0x194) >= 1 && c->mode != 3)
    {
        if (!c->legacy_death_angles)
        {
            c->error = BH_ACTOR_ROUTE_ARGUMENT;
            return;
        }
        bh_entity_set_query(c->pool, e, 0);
        bh_sound_stop(c->route->sound, word(e, 0x104));
        t = damage_tick_phase(c, e);
        DAMAGE_TICK_CURVE(damage_query_stored((double)t - 1.0 / 30.0), &before);
        DAMAGE_TICK_CURVE(t, &at);
        DAMAGE_TICK_CURVE(damage_query_stored((double)t - (-1.0 / 30.0)), &after);
        bh_actor_damage_death_velocity(&after, &at, &velocity);
        bh_script_debris_create(c->fragments, (rand() & 7) + 1, &death_position, 8, 10, 1);
        strength = damage_query_stored(.75 - (double)rand() * (-1.0 / 65536.0));
        bh_sound_request(c->route->sound, c->death_sound, 2, strength, &death_position, &velocity);
        random = random_step(word(e, 0x1e8));
        spin.x = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        spin.y = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        spin.z = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        set_word(e, 0x1e8, random);
        bh_remote_choose_slot(c->loot_rows, c->loot_row_count, 0, -1, -1, &loot);
        random = word(e, 0x1e8);
        token = (long)word(e, 0x100);
        owner = (BHEntity *)word(e, 0x1ac);
        material = word(e, 0x114);
        strength = damage_query_stored(1.0 - (double)(random & 32767) * (-1.0 / 16384.0));
        bh_target_construct_moving(c->targets, &death_position, c->legacy_death_angles, &velocity, &spin,
                                   material, 10000, strength, owner, token, loot, 0);
        room = damage_query_room(c, e);
        if (!room)
        {
            c->error = BH_ACTOR_ROUTE_TOPOLOGY;
            return;
        }
        previous = (BHEntity *)word(e, 0xf8);
        if (previous) set_word(previous, 0xf4, word(e, 0xf4));
        else room->actors = (BHEntity *)word(e, 0xf4);
        next = (BHEntity *)word(e, 0xf4);
        if (next) set_word(next, 0xf8, word(e, 0xf8));
        bh_entity_release(c->pool, e);
        c->error = BH_ACTOR_ROUTE_OK;
        return;
    }
resume:
    if (DAMAGE_TICK_FLOAT(0x194) < .03 && word(e, 0x198) == 0x3f800000UL && word(e, 0x19c) == 0x3f800000UL)
        e->step = normal_tick;
    c->error = BH_ACTOR_ROUTE_OK;
}
#undef DAMAGE_TICK_FLOAT
#undef DAMAGE_TICK_STORE
#undef DAMAGE_TICK_VEC
#undef DAMAGE_TICK_CURVE
