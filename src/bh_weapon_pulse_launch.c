#include "bh_weapon_pulse_launch.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
int __cdecl bh_weapon_pulse_launch(BHWeaponPulseLaunchContext *c, BHEntity *w, BHEntity *p)
{
    BHWeaponCommonState *s;
    BHWeaponFireContext *f;
    BHVec3 up, forward, muzzle, velocity, recoil;
    BHWorldGeometry *world;
    float timer;
    unsigned long i;
    if (!c || !c->fire || !c->projectile || !w || !p || !bh_math_environment_supported())
        return 0;
    f = c->fire;
    if (!f->sound || !f->transport || !c->projectile->sync_identity)
        return 0;
    s = bh_weapon_common_state(w);
    f->error = 0;
    memcpy(&timer, &s->word_f0, 4);
    if (!(timer <= 0))
        return 1;
    if (!s->ammo)
        return bh_weapon_fire_empty(f, p);
    s->ammo = (long)((unsigned long)s->ammo - 1);
    timer = 1.5f;
    memcpy(&s->word_f0, &timer, 4);
    bh_sound_request(f->sound, c->sound, 1, .8f, &p->position, &p->origin);
    memcpy(&up, (unsigned char *)p + 0x24, 12);
    memcpy(&forward, (unsigned char *)p + 0x280, 12);
    for (i = 0; i < 3; i++)
    {
        ((float *)&muzzle)[i] = stored((double)((float *)&p->position)[i] + ((float *)&up)[i]);
        ((float *)&velocity)[i] = stored((double)((float *)&forward)[i] * 15);
    }
    if (f->peers > 1)
        *c->projectile->sync_identity = ((unsigned long)f->local_id | 0x4000UL) << 16 | f->sender;
    memcpy(&world, (unsigned char *)p + 0xc4, 4);
    bh_pulse_projectile_create(c->projectile, world, p, &muzzle, &p->origin, &velocity);
    if (f->peers > 1)
        bh_weapon_fire_packet(f, 1, 0, 8, NULL);
    memcpy(&recoil, (unsigned char *)p + 0x2a4, 12);
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (-6));
    memcpy((unsigned char *)p + 0x2a4, &recoil, 12);
    ++f->shots;
    return f->error == 0;
}
