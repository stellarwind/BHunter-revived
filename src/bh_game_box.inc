/* Isolated diagnostic level adapter. Not an original gameplay implementation.
   Only level setup and recorded input are synthetic; all mechanic owners are real. */
static BHGameApp *box_observer;
static BHEntity *box_aggressors[7];
static unsigned long box_aggressor_generations[7], box_aggressor_count;
static const BHVec3 box_ring_positions[7] = {{-80,-8,0},{80,8,0},{0,8,80},
                                           {0,-8,-80},{-80,8,80},{80,0,-80},{-80,0,-80}};
static FILE *box_motion;
static FILE *box_routes;
static void __cdecl box_route_capture(const BHEntity *e, const BHActorRouteState *after,
                                     const BHWorldGeometry *world, long incoming, long outgoing)
{
    BHActorRouteState before;
    unsigned long header[4];
    if (!box_routes) return;
    bh_actor_route_read(e, &before);
    header[0]=incoming;header[1]=outgoing;header[2]=world->navigation.count;header[3]=world->query_flags;
    fwrite(header,sizeof(header),1,box_routes);
    fwrite(&world->minimum,sizeof(BHVec3),1,box_routes);
    fwrite(&world->maximum,sizeof(BHVec3),1,box_routes);
    fwrite(&before,sizeof(before),1,box_routes);
    fwrite(world->navigation.portals,sizeof(BHPortal),6,box_routes);
    fwrite(after,sizeof(*after),1,box_routes);
}
static int box_collision_probes(BHGameApp *a)
{
    /* Known planes in the five-room asset, tested through the live BSP,
       portal traversal and obstacle owners before spawning test actors. */
    static const BHVec3 ends[7] = {{200,12,0},{-200,12,0},{0,12,200},
        {0,12,-200},{0,40,0},{0,-40,0},{20,0,-30}};
    /* Segment owner returns the original 0.05 normal-offset contact. */
    static const BHVec3 expected[7] = {{119.95f,12,0},{-119.95f,12,0},{0,12,119.95f},
        {0,12,-119.95f},{0,19.95f,0},{0,-19.95f,0},{20,0,-15.95f}};
    BHVec3 start;
    BHWorldGeometry *cached;
    BHWorldSegmentHit hit;
    unsigned long i;
    int result, passed = 1;
    FILE *f;
    if (a->navigation.node_count < 5 || a->navigation.node_count == 8) return 1;
    f = fopen("box-collision.csv", "w");
    if (!f) return failure(a,14,"Collision capture open failed");
    fprintf(f,"probe,result,kind,x,y,z,fraction,error,passed\n");
    for(i=0;i<7;++i)
    {
        int valid;
        start.x=0;start.y=12;start.z=0;
        if(i==6){start.x=20;start.y=0;start.z=-10;}
        cached=0;memset(&hit,0,sizeof(hit));
        result=bh_world_segment_run(&a->segment,0,&cached,&start,ends+i,0,&hit);
        valid=result==1 && hit.object.kind==BH_WORLD_HIT_GEOMETRY &&
            !memcmp(&hit.point,expected+i,sizeof(BHVec3));
        fprintf(f,"%lu,%d,%d,%.9g,%.9g,%.9g,%.9g,%d,%d\n",i,result,
            hit.object.kind,hit.point.x,hit.point.y,hit.point.z,hit.fraction,a->segment.error,valid);
        if(!valid)passed=0;
    }
    fclose(f);
    return passed ? 1 : failure(a,14,"Box collision plane comparison failed");
}
static int box_aggression_setup(BHGameApp *a)
{
    static const BHVec3 positions[5] = {{-25, -8, -40}, {25, 8, -40},
                                      {-25, 8, 40}, {25, -8, 40}, {0, 12, 40}};
    static const BHVec3 junction_positions[7] = {{-80,-8,0},{80,8,0},{-25,8,80},
                                               {25,-8,-90},{-20,12,20},{-20,0,0},{20,0,0}};
    int ring = a->navigation.node_count == 8 && !a->navigation.junction_count;
    const BHVec3 *points = ring ? box_ring_positions : a->navigation.node_count >= 5 ? junction_positions : positions;
    unsigned long i, j;
    BHActorRouteState s;
    if (!box_collision_probes(a)) return 0;
    box_aggressor_count = a->navigation.node_count >= 5 ? 7 : 5;
    box_routes=fopen("box-routes.bin","wb");
    if(!box_routes) return failure(a,14,"Route capture open failed");
    a->actor_route.promotion_capture=box_route_capture;
    for (i = 0; i < box_aggressor_count; ++i)
    {
        BHEntity *e = bh_actor_construct_position(&a->actor_construct, points + i,
                                                  ring ? (i < 3 ? 6 : i < 5 ? 2 : 1) :
                                                  i < 4 ? 11 : i == 4 ? 6 : i == 5 ? 1 : 2,
                                                  5000, ring ? 24 + i * 2 : 30, 0, 12345 + i);
        if (!e || a->actor_construct.error)
            return failure(a, 14, "Aggression box actor creation failed");
        box_aggressors[i] = e;
        box_aggressor_generations[i] = e->generation;
        /* Diagnostic initial positions only. Original constructors choose room centers.
           All subsequent target, route and attack updates use production owners. */
        bh_actor_route_read(e, &s);
        for (j = 0; j < 4; ++j)
            s.controls[j] = points[i];
        s.portal_center = points[i];
        bh_actor_route_write(e, &s);
    }
    a->box_trace = fopen("box-trace.csv", "w");
    if (!a->box_trace)
        return failure(a, 14, "Aggression box trace open failed");
    box_motion = fopen("box-motion.bin", "wb");
    if (!box_motion)
        return failure(a, 14, "Aggression motion capture open failed");
    fprintf(a->box_trace, "tick,actor,x,y,z,phase,duration,local_rng,target_player,forced,fire_frame,damage,attacks,specials,beams,attack_error,route_error,response_error,player_dead,target_actor,current_room,next_room,speed_scale\n");
    stage(a, ring ? "Tunnel ring: three neutral NPCs, two moving targets, two hunters;900 ticks" :
                   "Junction box: seven actors, five connected junctions and four corner routes;900 ticks");
    return 1;
}
static void box_aggression_observe(BHGameApp *a)
{
    unsigned long i, j, target, forced;
    long target_actor;
    int response_error = a->actor_response.error;
    short fire_frame, dead;
    float damage;
    BHActorRouteState s;
    BHEntityPositionPacket p;
    struct {unsigned long tick, actor; float phase, duration; BHVec3 controls[4], offset; float motion[10];} sample;
    memcpy(&dead, (char *)a->player + 0x2b4, 2);
    for (i = 0; i < box_aggressor_count; ++i)
    {
        BHEntity *e = box_aggressors[i];
        if (!bh_entity_is_active(&a->pool, e) || e->generation != box_aggressor_generations[i] || !e->query)
        {
            if (a->navigation.node_count == 8)
            {
                /* Keep the diagnostic populated after real combat retirement. */
                e=bh_actor_construct_position(&a->actor_construct,box_ring_positions+i,
                    i<3?6:i<5?2:1,5000,24+i*2,0,12345+i+a->box_ticks);
                if(!e || a->actor_construct.error){failure(a,14,"Ring respawn failed");return;}
                box_aggressors[i]=e;box_aggressor_generations[i]=e->generation;
            }
            else {failure(a, 14, "Aggression box actor retired");return;}
        }
        p.event = 3;
        e->query(e, &p);
        bh_actor_route_read(e, &s);
        sample.tick=a->box_ticks; sample.actor=i; sample.phase=s.phase; sample.duration=s.duration;
        memcpy(sample.controls,s.controls,sizeof(sample.controls));
        memcpy(&sample.offset,(char*)e+0x15c,12);
        memset(sample.motion,0,sizeof(sample.motion));
        {long event=16;memcpy(sample.motion,&event,4);}
        e->query(e,sample.motion);
        if (fwrite(&sample,sizeof(sample),1,box_motion)!=1)
        {
            failure(a,14,"Aggression motion capture write failed");return;
        }
        memcpy(&target, (char *)e + 0x1b8, 4);
        memcpy(&forced, (char *)e + 0x1d4, 4);
        memcpy(&fire_frame, (char *)e + 0x1e0, 2);
        memcpy(&damage, (char *)e + 0x194, 4);
        target_actor = target == (unsigned long)a->player ? -2 : -1;
        for (j = 0; j < box_aggressor_count; ++j)
            if (target == (unsigned long)box_aggressors[j]) target_actor = (long)j;
        fprintf(a->box_trace, "%lu,%lu,%.9g,%.9g,%.9g,%.9g,%.9g,%lu,%d,%lu,%d,%.9g,%lu,%lu,%lu,%d,%d,%d,%d,%ld,%ld,%ld,%.9g\n",
                a->box_ticks, i, p.position.x, p.position.y, p.position.z, s.phase, s.duration, s.random,
                target == (unsigned long)a->player, forced, fire_frame, damage,
                a->actor_attack.attacks, a->actor_attack.specials, a->actor_attack.beams,
                a->actor_attack.error, a->actor_route.error, response_error ? response_error : a->actor_response.error, dead,
                target_actor, bh_navigation_find(&a->navigation,s.current), bh_navigation_find(&a->navigation,s.next),s.speed_scale);
    }
    fflush(a->box_trace);
    if (a->box_ticks >= (a->navigation.node_count >= 8 ? 900UL : 150UL))
    {
        fclose(box_motion);box_motion=0;
        fclose(box_routes);box_routes=0;
        a->quit = 1;
    }
}
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
    if (!a->hardware || !((a->editor_world.count == 4 && a->navigation.node_count == 2) ||
        (a->box_auto == 2 && ((a->editor_world.count == 7 && a->navigation.node_count == 5) ||
         (a->editor_world.count == 11 && a->navigation.node_count == 9 && a->navigation.junction_count == 5) ||
         (a->editor_world.count == 9 && a->navigation.node_count == 8 && a->navigation.junction_count == 0)))))
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
    if (a->box_auto == 2)
        return box_aggression_setup(a);
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
    if (a->box_auto == 2)
    {
        if (box_aggressor_count == 7 && a->navigation.node_count != 8 && a->box_ticks == 30)
        {
            struct {long event; float damage; BHEntity *source;} hit;
            hit.event = 0; hit.damage = .3f; hit.source = a->player;
            box_aggressors[4]->query(box_aggressors[4], &hit);
        }
        Sleep(33);
        return;
    }
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
    if (a->box_auto == 2)
    {
        box_aggression_observe(a);
        return;
    }
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
