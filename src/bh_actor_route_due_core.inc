/* 00435790 uses only the clock's low word, including the final stamp store.
   The source adapter binds a stable real clock location, never a new global.
   A failed checked advance retains the existing source API's no-stamp rule. */
int bh_actor_route_due_core(BHEntity *actor, BHActorRouteContext *context, const unsigned short *clock)
{
    short delta = (short)*clock;
    delta -= *(unsigned short *)&actor->actor.duration_ticks;
    delta -= *(unsigned short *)&actor->actor.tick;
    if (delta >= 0)
    {
        if (!bh_actor_route_advance(context, actor))
            return 0;
        *(unsigned short *)&actor->actor.tick = (unsigned short)(*clock + delta);
    }
    return 1;
}
