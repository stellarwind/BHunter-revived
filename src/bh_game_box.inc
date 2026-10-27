/* Isolated diagnostic level adapter. Not an original gameplay implementation.
   Only level setup and recorded input are synthetic; all mechanic owners are real. */
static BHGameApp *box_observer;
static struct BoxTraining
{
    BHEntity *second, *missiles;
    unsigned long second_generation, missile_generation;
    unsigned long station, complete, next_mission, picked, second_seen, missile_projectiles, locked, complete_tick,
        next_live, missile_removed;
    long ammo;
    float charge;
} box_training;
static BHEntity *box_action(BHGameApp *a, long op, long a0, long a1, long a2, long a3, long a4, BHEntity *next)
{
    long args[5], selectors[5] = {-1, -1, -1, -1, -1};
    BHEntity *e;
    BHScriptVMState state;
    args[0] = a0;
    args[1] = a1;
    args[2] = a2;
    args[3] = a3;
    args[4] = a4;
    e = bh_script_vm_create(&a->vm, 0, op, args, selectors);
    if (e && next)
    {
        bh_script_vm_read(e, &state);
        state.next = next;
        if (!bh_script_vm_bind(&a->vm, e, &state))
            return 0;
    }
    return e;
}
static int box_wait(BHGameApp *a, long event, BHEntity *action, BHEntity *actor)
{
    BHEntity *e = bh_entity_acquire(&a->pool);
    BHScriptWaitState s;
    BHScriptWaitPacket p;
    if (!e || !action)
        return 0;
    memset(&s, 0, sizeof(s));
    s.fired = action;
    s.remaining = 1;
    s.expected = event;
    s.variable_remaining = s.variable_expected = -1;
    if (!bh_script_wait_bind(&a->lifetime.timers, e, &s))
        return 0;
    p.event = 8;
    p.actor = actor;
    p.expected = 0;
    e->query(e, &p);
    return !a->lifetime.timers.error;
}
static unsigned long box_emissions(BHGameApp *a, unsigned long index)
{
    if (index == 1)
        return a->weapon_pulse.shots;
    if (index == 4)
        return a->weapon_arc_fire.shots;
    if (index == 6)
        return a->weapon_fire.bursts;
    return a->weapon_fire.shots;
}
static void __cdecl box_event(BHEntity *e, void *raw)
{
    long *p = (long *)raw;
    (void)e;
    if (!box_observer || p[0] != 14)
        return;
    if (p[2] == 2013)
        box_observer->box_objective = 1;
    if (p[2] == 2020)
        box_training.station = 1;
    if (p[2] == 2021)
    {
        box_training.complete = 1;
        box_training.complete_tick = box_observer->box_ticks;
    }
    if (p[2] == 2030)
        box_training.next_mission = 1;
}
static int box_setup(BHGameApp *a)
{
    unsigned long i;
    BHEntity *e, *action;
    BHWeaponObjectState *weapon;
    BHVec3 p = {0, 0, -45};
    if (!a->box_level)
        return 1;
    if (!a->hardware || a->editor_world.count != 4 || a->navigation.node_count != 2)
        return failure(a, 14, "Box level requires isolated assets and HAL");
    memset(&box_training, 0, sizeof(box_training));
    srand(1979);
    box_observer = a;
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
    {
        e = a->bootstrap.menus[i].object.entity;
        e->step = 0;
        e->refresh = 0;
        bh_input_set_enabled(a->bootstrap.menus[i].object.input, &a->snapshot, 0);
    }
    a->player_state.mode = 1;
    a->player_state.world_mode = 2;
    a->player_state.paused = 0;
    a->player_state.ending = 0;
    a->editor_world.mode = 2;
    a->starts[0].position.x = 0;
    a->starts[0].position.y = 0;
    a->starts[0].position.z = -60;
    a->starts[0].angles = a->zero;
    a->player_camera.player_count = 1;
    if (!bh_player_camera_reset(&a->player_camera, 0) || !bh_physics_step_read_entity(&a->player_motion, a->player) ||
        !bh_input_set_enabled(a->player_tick.input, &a->snapshot, 1))
        return failure(a, 14, "Box player initialization failed");
    /* Fixed diagnostic bindings, isolated from the saved user profile. */
    bh_player_input_bind_axis(a->player_tick.input, 0, "", 0, 0, VK_LEFT, VK_RIGHT, a->box_auto ? 1 : 0, 0);
    bh_player_input_bind_axis(a->player_tick.input, 1, "", 0, 0, VK_DOWN, VK_UP, a->box_auto ? 3 : 0, 0);
    bh_player_input_bind_axis(a->player_tick.input, 2, "", 0, 0, 'S', 'W', 0, 0);
    bh_player_input_bind_key(a->player_tick.input, 22, "", 0, 0, VK_CONTROL);
    bh_player_input_bind_key(a->player_tick.input, 3, "", 0, 0, VK_SPACE);
    for (i = 0; i < 10; ++i)
        bh_player_input_bind_key(a->player_tick.input, 12 + i, "", 0, 0, i == 9 ? '0' : '1' + i);
    a->collection.enabled[0] = a->collection.enabled[1] = 0;
    a->box_pickup = bh_pickup_basic_spawn(&a->pickups, &p, 0, 50, 50, 0, 0);
    if (!a->box_pickup)
        return failure(a, 14, "Box pickup creation failed");
    a->box_pickup_generation = a->box_pickup->generation;
    p.x = -15;
    bh_pickup_spawn(&a->pickups, &p, 1, 1, 1, 1, 0, 0);
    if (!a->box_auto)
        for (i = 0; i < 10; ++i)
        {
            p.x = -30 + 15 * (long)(i % 5);
            p.y = 0;
            p.z = 20 + 20 * (long)(i / 5);
            if (!bh_pickup_spawn(&a->pickups, &p, i, 1, 1, 1, 0, 0))
                return failure(a, 14, "Box weapon pickup creation failed");
        }
    /* Actual route AI, damage, wreck and loot pipeline; fixed seed is level data. */
    memset(a->availability[3], 0, sizeof(a->availability[3]));
    a->availability[3][0] = 1;
    a->availability[3][31] = 1;
    a->box_ai = bh_actor_construct_room(&a->actor_construct, a->rooms[2].world, 6, 5000, 10, 2013, 12345);
    if (!a->box_ai || a->actor_construct.error)
        return failure(a, 14, "Box AI creation failed");
    a->box_target_generation = a->box_ai->generation;
    /* Original waitfor -> VM broadcast chain, with a diagnostic result observer. */
    /* Original POWERUP/SENSOR2/LAUNCH2 chain, relocated into a straight box.
    Target1 death -> five missiles + station sensor -> faster target -> relay. */
    action = box_action(a, 10, 1, 4, 0, 2020, 0, 0);
    action = box_action(a, 24, 2020, 0, 0, 0, 0, action);
    action = box_action(a, 13, 5, 5, 5, 0, 0, action);
    if (!box_wait(a, 2013, action, a->nodes[3]->entity))
        return failure(a, 14, "Box missile/sensor wait failed");
    action = box_action(a, 17, 1, 6, 5, 30, 2021, 0);
    if (!box_wait(a, 2020, action, a->nodes[2]->entity))
        return failure(a, 14, "Box second target wait failed");
    {
        BHScriptTimerState timer;
        BHEntity *relay;
        action = box_action(a, 18, 0, 2030, 0, 0, 0, 0);
        relay = bh_entity_acquire(&a->pool);
        memset(&timer, 0, sizeof(timer));
        timer.fired = action;
        timer.remaining = 1;
        timer.low = timer.high = 10;
        timer.variable_remaining = timer.variable_low = timer.variable_high = -1;
        if (!relay || !bh_script_timer_bind(&a->lifetime.timers, relay, &timer, 1) ||
            !box_wait(a, 2021, relay, a->nodes[2]->entity))
            return failure(a, 14, "Box completion relay failed");
    }
    action = box_action(a, 17, 1, 11, 50, 35, 2031, 0);
    if (!box_wait(a, 2030, action, a->nodes[2]->entity))
        return failure(a, 14, "Box next assignment wait failed");
    e = bh_entity_acquire(&a->pool);
    if (!e || !bh_entity_set_query(&a->pool, e, box_event))
        return failure(a, 14, "Box event observer failed");
    if (a->lifetime.timers.error)
        return failure(a, 14, "Box objective activation failed");
    bh_weapon_registry_resolve(&a->weapons.registry, 0, &e, &weapon);
    a->box_trace = fopen("box-trace.csv", "w");
    if (!a->box_trace)
        return failure(a, 14, "Box trace open failed");
    fprintf(
        a->box_trace,
        "tick,x,y,z,vx,vy,vz,contacts,shots,damage_packets,ammo,pickup_alive,ai_alive,ai_x,ai_y,ai_z,objective,drop_seen,drop_taken,drop_x,drop_y,drop_z,collected_mask,fired_mask,primary,secondary,spawned_mask,station,missiles_picked,missile_ammo,second_seen,second_alive,complete,next_mission,missile_projectiles,locked,charge,progress,missile_removed,recycled_successors\n");
    bh_message_enqueue(
        &a->messages,
        "1BOX TEST\nW/S: thrust/brake\nArrows: steering\nCtrl/Space: primary/secondary\n1-5/6-0: select weapon\nDestroy first target.\nCollect missiles ahead.\nDestroy faster target.\nEsc: exit this test.");
    stage(a, "Box level: direct gameplay; actual AI, projectiles, pickup and objective callbacks");
    return 1;
}
static void box_inputs(BHGameApp *a)
{
    BHEntityPositionPacket p;
    BHVec3 d, right, up, forward;
    double x, y, z, yaw, pitch, distance;
    int aim = 0, combat = 0;
    if (!a->box_auto)
        return;
    memset(a->snapshot.current, 0, sizeof(a->snapshot.current));
    a->snapshot.axes[0] = a->snapshot.axes[1] = 0;
    if (a->box_ticks < 35)
        a->snapshot.current['W'] = 0x80;
    if (a->box_ticks >= 35)
    {
        if (a->box_ai->generation == a->box_target_generation && a->box_ai->query)
        {
            p.event = 3;
            a->box_ai->query(a->box_ai, &p);
            aim = combat = 1;
        }
        else if (a->box_drop && a->box_drop->generation == a->box_drop_generation)
        {
            p.position = a->box_drop->position;
            aim = 1;
        }
        else if (!box_training.picked)
        {
            p.position.x = 0;
            p.position.y = 0;
            p.position.z = 5;
            aim = 1;
        }
        else if (box_training.second && box_training.second->generation == box_training.second_generation &&
                 box_training.second->query)
        {
            p.event = 3;
            box_training.second->query(box_training.second, &p);
            aim = 1;
            combat = 2;
        }
        if (aim)
        {
            d.x = p.position.x - a->player->position.x;
            d.y = p.position.y - a->player->position.y;
            d.z = p.position.z - a->player->position.z;
            memcpy(&right, (char *)a->player + 0x18, 12);
            memcpy(&up, (char *)a->player + 0x24, 12);
            memcpy(&forward, (char *)a->player + 0x30, 12);
            x = d.x * right.x + d.y * right.y + d.z * right.z;
            y = d.x * up.x + d.y * up.y + d.z * up.z;
            z = d.x * forward.x + d.y * forward.y + d.z * forward.z;
            yaw = atan2(x, z) * 1.5;
            pitch = atan2(y, sqrt(x * x + z * z)) * 1.5;
            /* Bounded input amplitudes belong to this test driver, not game arithmetic. */
            a->snapshot.axes[0] = (float)(yaw > 1 ? 1 : yaw < -1 ? -1 : yaw);
            a->snapshot.axes[1] = (float)(pitch > 1 ? 1 : pitch < -1 ? -1 : pitch);
            distance = sqrt(x * x + y * y + z * z);
            if (!combat && z > 0 && fabs(yaw) < .3 && fabs(pitch) < .3 && distance > 4)
                a->snapshot.current['W'] = 0x80;
        }
        if (combat == 1)
            a->snapshot.current[VK_CONTROL] = 0x80;
        if (combat == 2 && box_training.charge >= 48 && a->box_ticks % 60 < 4)
            a->snapshot.current[VK_SPACE] = 0x80;
    }
}
static void box_observe(BHGameApp *a)
{
    BHEntity *e, *drop, *missile_weapon;
    BHWeaponObjectState *weapon, *missile_state;
    BHEntityPositionPacket p;
    BHVec3 dp = {0, 0, 0};
    int alive, pickup, second_alive = 0;
    unsigned long tag;
    short progress;
    if (!a->box_level)
        return;
    ++a->box_ticks;
    if (!box_training.missiles)
        for (drop = a->pickups.head; drop; drop = bh_pickup_state(drop)->global_next)
            if (bh_pickup_state(drop)->type == 5)
            {
                box_training.missiles = drop;
                box_training.missile_generation = drop->generation;
                break;
            }
    if (box_training.missiles && box_training.missiles->generation != box_training.missile_generation)
        box_training.missile_removed = 1;
    for (e = a->pool.active; e; e = e->next)
    {
        if (e->query == bh_actor_response_query || e->query == bh_actor_damage_query)
        {
            memcpy(&tag, (char *)e + 0x11c, 4);
            if (tag == 2021)
            {
                box_training.second = e;
                box_training.second_generation = e->generation;
                box_training.second_seen = 1;
                second_alive = 1;
            }
            if (tag == 2031)
                box_training.next_live = 1;
        }
        if (e->step == bh_effect_orbit_tick)
            ++box_training.missile_projectiles;
    }
    if (bh_weapon_registry_resolve(&a->weapons.registry, 5, &missile_weapon, &missile_state))
    {
        box_training.ammo = missile_state->common.ammo;
        box_training.charge = (float)missile_state->common.hud.charge;
        if (box_training.ammo > 0)
            box_training.picked = 1;
        if (box_training.charge >= 48)
            box_training.locked = 1;
    }
    memcpy(&progress, a->config.settings + 2, 2);
    alive = a->box_ai->generation == a->box_target_generation && a->box_ai->query != NULL;
    pickup = a->box_pickup->generation == a->box_pickup_generation;
    memset(&p, 0, sizeof(p));
    p.event = 3;
    if (alive)
        a->box_ai->query(a->box_ai, &p);
    if (!bh_weapon_registry_resolve(&a->weapons.registry, 0, &e, &weapon))
    {
        failure(a, 14, "Box inventory binding failed");
        return;
    }
    if (!a->box_drop_seen)
        for (drop = a->pickups.head; drop; drop = bh_pickup_state(drop)->global_next)
        {
            if (bh_pickup_state(drop)->fade > 0)
            {
                a->box_drop = drop;
                a->box_drop_generation = drop->generation;
                a->box_drop_seen = 1;
                a->box_drop_ammo = weapon->common.ammo;
                break;
            }
        }
    if (a->box_drop)
    {
        if (a->box_drop->generation == a->box_drop_generation)
            dp = a->box_drop->position;
        else if (weapon->common.ammo > a->box_drop_ammo)
            a->box_drop_taken = 1;
    }
    fprintf(
        a->box_trace,
        "%lu,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%lu,%lu,%lu,%ld,%d,%d,%.9g,%.9g,%.9g,%d,%d,%d,%.9g,%.9g,%.9g,%lu,%lu,%u,%u,%lu,%lu,%lu,%ld,%lu,%d,%lu,%lu,%lu,%lu,%.0f,%d,%lu,%lu\n",
        a->box_ticks, a->player->position.x, a->player->position.y, a->player->position.z, a->player_motion.velocity.x,
        a->player_motion.velocity.y, a->player_motion.velocity.z, a->physics.contacts, a->weapon_fire.shots,
        a->ballistic.bindings.damage_packets, weapon->common.ammo, pickup, alive, p.position.x, p.position.y,
        p.position.z, a->box_objective, a->box_drop_seen, a->box_drop_taken, dp.x, dp.y, dp.z, a->box_collected_mask,
        a->box_fired_mask, *((unsigned char *)a->player + 0x2b6), *((unsigned char *)a->player + 0x2b7),
        a->box_spawned_mask, box_training.station, box_training.picked, box_training.ammo, box_training.second_seen,
        second_alive, box_training.complete, box_training.next_mission && box_training.next_live,
        box_training.missile_projectiles, box_training.locked, box_training.charge, progress,
        box_training.missile_removed, a->pool.recycled_successors);
    fflush(a->box_trace);
    if (a->box_auto && box_training.next_mission && box_training.next_live &&
        a->box_ticks > box_training.complete_tick + 330)
        a->quit = 1;
    if (a->box_auto && a->box_ticks >= 6000)
    {
        failure(a, 14, "Box training sequence did not complete within6000ticks");
        a->quit = 1;
    }
}
