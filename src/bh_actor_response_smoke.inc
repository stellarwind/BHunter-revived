/* Route/smoke subpath439bfe..439dcd of00439910. Not another original owner.
   Returns -1 for a checked source pursuit failure,0 without smoke,1 with smoke. */
int __cdecl bh_actor_response_route_smoke(BHEntity *e, BHActorResponseContext *c, BHVec3 *death_position)
{
    BHVec3 position, velocity, acceleration;
    float left, t, angular, angle;
    int draw;
    if (number(e, 0x180) >= number(e, 0x184))
    {
        left = response_query_stored((double)number(e, 0x180) - number(e, 0x184));
        if (!bh_actor_pursuit_route(c->attack->pursuit, e))
        {
            c->error = 2;
            return -1;
        }
        *(volatile float *)((char *)e + 0x180) = left;
    }
    if (number(e, 0x194) < 1)
    {
        draw = rand();
        if ((double)number(e, 0x194) * 5000 <= draw) return 0;
    }
    acceleration.x = 0;acceleration.y = -.002f;acceleration.z = 0;
    velocity.x = response_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.y = response_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.z = response_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    t = response_query_stored((double)number(e, 0x180) / number(e, 0x184));
    bh_camera_curve(t, &position, (BHVec3 *)((char *)e + 0x120), (BHVec3 *)((char *)e + 0x144),
                    (BHVec3 *)((char *)e + 0x150), (BHVec3 *)((char *)e + 0x12c));
    position.x = response_query_stored((double)number(e, 0x15c) + position.x);
    position.y = response_query_stored((double)number(e, 0x160) + position.y);
    position.z = response_query_stored((double)number(e, 0x164) + position.z);
    *death_position = position;
    angular = response_query_stored((double)(rand() - 16384) * (1.0 / 16384.0));
    angle = response_query_stored((double)rand() * (1.0 / 512.0));
    bh_script_particle_create(c->damage->particles, c->damage->smoke_resource, 3, &position, &velocity,
                              &acceleration, .5f, angle, 12, angular, .9f, .95f, 2, .3f);
    return 1;
}
