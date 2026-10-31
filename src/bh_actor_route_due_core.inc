/* 00435790 uses only the clock's low word, including the final stamp store.
   The source adapter binds a stable real clock location, never a new global.
   A failed checked advance retains the existing source API's no-stamp rule. */
int __cdecl bh_actor_route_due_core(BHEntity *actor, BHActorRouteContext *context, const unsigned short *clock)
{
    short delta = (short)*clock;
    delta -= *(unsigned short *)((char *)actor + 0x1e6);
    delta -= *(unsigned short *)((char *)actor + 0x1e4);
    if (delta >= 0)
    {
        if (!bh_actor_route_advance(context, actor))
            return 0;
        *(unsigned short *)((char *)actor + 0x1e4) = (unsigned short)(*clock + delta);
    }
    return 1;
}
