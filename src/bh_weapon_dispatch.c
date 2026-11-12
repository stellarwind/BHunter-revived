#include "bh_weapon_dispatch.h"
#include "bh_weapon_latch.h"
#include "bh_weapon_timer.h"
#include <string.h>
int bh_weapon_dispatch(BHWeaponDispatch *c, unsigned int selection, BHWeaponSlot slot)
{
    BHEntity *weapon_entity;
    BHWeaponObjectState *weapon;
    BHWeaponHudArcPlayer p;
    unsigned long id;
    int result = 0;
    if (!c || !c->factory || !c->player || slot < BH_WEAPON_READY || slot > BH_WEAPON_HUD ||
        ((unsigned long)slot & 3) ||
        !bh_weapon_registry_resolve(&c->factory->registry, selection, &weapon_entity, &weapon))
    {
        if (c)
            c->error = 1;
        return 0;
    }
    c->error = 0;
    ++c->calls;
    memcpy(&id, (char *)weapon + slot - 0xe8, 4);
    c->identity = id;
    switch (id)
    {
    case 0x0047d680UL:
        return 1;
    case 0x004309b0UL:
        result = bh_weapon_delay_tick(weapon_entity);
        break;
    case 0x00433170UL:
        result = bh_weapon_latch_tick(weapon_entity);
        break;
    case 0x00433820UL:
    case 0x00433b00UL:
        result = bh_weapon_single_timer_tick(weapon_entity);
        break;
    case 0x00431050UL:
        if (c->ready)
            result = bh_weapon_ready_reset(c->ready, weapon_entity, c->player);
        break;
    case 0x004310c0UL:
        if (c->target)
            result = bh_weapon_target_ready(c->target, weapon_entity, c->player);
        break;
    case 0x00431130UL:
        if (c->target)
            result = bh_weapon_target_cancel(c->target, weapon_entity);
        break;
    case 0x00431180UL:
        if (c->target)
            result = bh_weapon_target_update(c->target, weapon_entity, c->player);
        break;
    case 0x00430d80UL:
        if (c->target)
            result = bh_weapon_homing_track(c->target, weapon_entity, c->player);
        break;
    case 0x004324a0UL:
        if (c->arc)
            result = bh_weapon_arc_activate(c->arc, weapon_entity, c->player);
        break;
    case 0x00432510UL:
        if (c->arc)
            result = bh_weapon_arc_deactivate(c->arc, weapon_entity);
        break;
    case 0x00432c60UL:
        if (c->arc)
            result = bh_weapon_arc_track(c->arc, weapon_entity, c->player);
        break;
    case 0x0042f730UL:
    case 0x004309d0UL:
    case 0x00432560UL:
        if (c->empty)
            result = bh_weapon_empty_trigger(c->empty, id, weapon_entity, c->player);
        break;
    case 0x00433dc0UL:
        if (c->empty)
            result = bh_weapon_index9_trigger(c->empty, weapon_entity, c->player);
        break;
    case 0x004305f0UL:
        if (c->fire)
            result = bh_weapon_fire_ballistic(c->fire, weapon_entity, c->player);
        break;
    case 0x0042f7a0UL:
        if (c->pulse)
            result = bh_weapon_pulse_fire(c->pulse, weapon_entity, c->player);
        break;
    case 0x00430a30UL:
        if (c->fire)
            result = bh_weapon_fire_homing(c->fire, weapon_entity, c->player);
        break;
    case 0x00431db0UL:
        if (c->fire)
            result = bh_weapon_area_start(c->fire, weapon_entity, c->player);
        break;
    case 0x00431f00UL:
        if (c->fire)
            result = bh_weapon_area_tick(c->fire, weapon_entity, c->player);
        break;
    case 0x004327a0UL:
        if (c->arc_fire)
            result = bh_weapon_arc_fire(c->arc_fire, weapon_entity, c->player);
        break;
    case 0x00431810UL:
        if (c->effects)
            result = bh_weapon_effect_orbit_fire(c->effects, weapon_entity, c->player);
        break;
    case 0x004331c0UL:
        if (c->effects)
            result = bh_weapon_effect_split_fire(c->effects, weapon_entity, c->player);
        break;
    case 0x00433850UL:
        if (c->effects)
            result = bh_weapon_effect_accelerated_fire(c->effects, weapon_entity, c->player);
        break;
    case 0x00433b30UL:
        if (c->pulse_launch)
            result = bh_weapon_pulse_launch(c->pulse_launch, weapon_entity, c->player);
        break;
    case 0x00432ff0UL:
        if (!c->arc_hud)
            break;
        p.position = c->player->position;
        p.forward = c->player->basis[2];
        p.aim = c->player->player.aim_direction;
        if (c->arc_hud)
            result = bh_weapon_hud_arc(c->arc_hud, &weapon->common.hud, &p);
        break;
    case 0x004314f0UL:
        if (c->target_hud)
            if (c->target_hud)
                result = bh_weapon_hud_target_draw(c->target_hud, &weapon->common.hud);
        break;
    default:
        c->error = 2;
        return 0;
    }
    if (!result)
        c->error = 3;
    return result;
}
