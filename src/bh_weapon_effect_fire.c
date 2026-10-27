#include "bh_weapon_effect_fire.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static int valid(BHWeaponEffectFireContext *c, BHEntity *w, BHEntity *p)
{
    return c && c->fire && c->projectile && c->projectile->pending_identity && c->fire->sound && c->fire->transport &&
           w && p && bh_math_environment_supported();
}
static void sync(BHWeaponEffectFireContext *c)
{
    BHWeaponFireContext *f = c->fire;
    if (f->peers > 1)
        *c->projectile->pending_identity = ((unsigned long)f->local_id | 0x4000UL) << 16 | f->sender;
}
static int alternate(BHWeaponEffectFireContext *c, BHEntity *w, BHEntity *p, int split)
{
    BHWeaponCommonState *s = bh_weapon_common_state(w);
    BHWeaponFireContext *f = c->fire;
    BHVec3 right, up, side, forward, muzzle, velocity, recoil;
    BHWorldGeometry *world;
    BHEntity *target;
    unsigned long i, phase;
    float timer;
    double dot, gain;
    phase = s->word_e8;
    memcpy(&timer, phase ? &s->word_f0 : &s->word_f4, 4);
    f->error = 0;
    if (timer > 0)
        return 1;
    if (!s->ammo)
        return bh_weapon_fire_empty(f, p);
    target = s->hud.charge >= 48 ? s->hud.target : NULL;
    s->ammo = (long)((unsigned long)s->ammo - 1);
    s->word_e8 = phase ? 0 : 1;
    timer = split ? .3f : 1;
    memcpy(phase ? &s->word_f0 : &s->word_f4, &timer, 4);
    memcpy(&right, (unsigned char *)p + 0x18, 12);
    memcpy(&up, (unsigned char *)p + 0x24, 12);
    memcpy(&side, (unsigned char *)p + 0x30, 12);
    memcpy(&forward, (unsigned char *)p + 0x280, 12);
    gain = phase ? 1 : -1;
    for (i = 0; i < 3; i++)
    {
        double coordinate;
        if (!phase)
            coordinate = (double)((float *)&p->position)[i] + ((double)((float *)&up)[i] * .5 - ((float *)&right)[i]);
        else if (split)
            coordinate = ((double)((float *)&p->position)[i] + (double)((float *)&up)[i] * .5) + ((float *)&right)[i];
        else
            coordinate = ((double)((float *)&p->position)[i] + ((float *)&right)[i]) + (double)((float *)&up)[i] * .5;
        ((float *)&muzzle)[i] = stored(coordinate);
    }
    memcpy(&world, (unsigned char *)p + 0xc4, 4);
    sync(c);
    if (split)
    {
        for (i = 0; i < 3; i++)
            ((float *)&velocity)[i] = stored((double)((float *)&p->origin)[i] - (double)((float *)&forward)[i] * (-60));
        dot = ((double)side.y * velocity.y + (double)side.z * velocity.z) + (double)side.x * velocity.x;
        if (dot < 10)
        {
            dot = 10 - dot;
            for (i = 0; i < 3; i++)
                ((float *)&velocity)[i] = stored((double)((float *)&side)[i] * dot + ((float *)&velocity)[i]);
        }
        bh_effect_split_create(c->projectile, world, p, &muzzle, &velocity, 3, 10, c->split_model, 1);
        if (f->peers > 1)
            bh_weapon_fire_packet(f, 1, 0, 6, NULL);
    }
    else
    {
        bh_effect_orbit_create(c->projectile, world, p, &muzzle, &p->origin, &right, &up, &forward, 8, target);
        if (f->peers > 1)
            bh_weapon_fire_packet(f, 1, 0, 3, target);
    }
    memcpy(&recoil, (unsigned char *)p + 0x2a4, 12);
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] =
            stored(((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (split ? -3 : -4)) -
                   (double)((float *)&right)[i] * (phase ? -.5 : .5));
    memcpy((unsigned char *)p + 0x2a4, &recoil, 12);
    ++f->shots;
    return f->error == 0;
}
int __cdecl bh_weapon_effect_orbit_fire(BHWeaponEffectFireContext *c, BHEntity *w, BHEntity *p)
{
    if (!valid(c, w, p) || !c->projectile->orbit)
        return 0;
    return alternate(c, w, p, 0);
}
int __cdecl bh_weapon_effect_split_fire(BHWeaponEffectFireContext *c, BHEntity *w, BHEntity *p)
{
    if (!valid(c, w, p) || !c->projectile->split)
        return 0;
    return alternate(c, w, p, 1);
}
int __cdecl bh_weapon_effect_accelerated_fire(BHWeaponEffectFireContext *c, BHEntity *w, BHEntity *p)
{
    BHWeaponCommonState *s;
    BHWeaponFireContext *f;
    BHVec3 up, forward, muzzle, acc, recoil;
    BHWorldGeometry *world;
    float timer;
    unsigned long i;
    if (!valid(c, w, p) || !c->projectile->accelerated)
        return 0;
    s = bh_weapon_common_state(w);
    f = c->fire;
    f->error = 0;
    memcpy(&timer, &s->word_f0, 4);
    if (!(timer <= 0))
        return 1;
    if (!s->ammo)
        return bh_weapon_fire_empty(f, p);
    s->ammo = (long)((unsigned long)s->ammo - 1);
    timer = 2;
    memcpy(&s->word_f0, &timer, 4);
    bh_sound_request(f->sound, c->shot_sound, 1, .6f, &p->position, &p->origin);
    bh_sound_request(f->sound, c->shot_sound, 1, 1.3f, &p->position, &p->origin);
    memcpy(&up, (unsigned char *)p + 0x24, 12);
    memcpy(&forward, (unsigned char *)p + 0x280, 12);
    for (i = 0; i < 3; i++)
    {
        ((float *)&muzzle)[i] = stored((double)((float *)&p->position)[i] + ((float *)&up)[i]);
        ((float *)&acc)[i] = stored((double)((float *)&forward)[i] * 20);
    }
    acc.y = stored((double)forward.y * 20 - (-2));
    sync(c);
    memcpy(&world, (unsigned char *)p + 0xc4, 4);
    bh_effect_accelerated_create(c->projectile, world, p, &muzzle, &p->origin, &acc);
    if (f->peers > 1)
        bh_weapon_fire_packet(f, 1, 0, 7, NULL);
    memcpy(&recoil, (unsigned char *)p + 0x2a4, 12);
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (-10));
    memcpy((unsigned char *)p + 0x2a4, &recoil, 12);
    ++f->shots;
    return f->error == 0;
}
