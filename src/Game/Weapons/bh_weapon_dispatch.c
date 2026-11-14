#include "bh_weapon_dispatch.h"
#include "bh_weapon_latch.h"
#include "bh_weapon_timer.h"
#include <string.h>
int bh_weapon_dispatch(BHWeaponDispatch *weapon_dispatch, unsigned int selection, BHWeaponSlot slot)
{
    BHEntity *weapon_entity;
    BHWeaponObjectState *weapon;
    BHWeaponHudArcPlayer p;
    unsigned long id;
    int result = 0;
    if (!weapon_dispatch || !weapon_dispatch->factory || !weapon_dispatch->player || slot < BH_WEAPON_READY || slot > BH_WEAPON_HUD ||
        ((unsigned long)slot & 3) ||
        !bh_weapon_registry_resolve(&weapon_dispatch->factory->registry, selection, &weapon_entity, &weapon))
    {
        if (weapon_dispatch)
            weapon_dispatch->error = 1;
        return 0;
    }
    weapon_dispatch->error = 0;
    ++weapon_dispatch->calls;
    switch (slot)
    {
    case BH_WEAPON_READY:
        id = weapon->ready_identity;
        break;
    case BH_WEAPON_CANCEL:
        id = weapon->cancel_identity;
        break;
    case BH_WEAPON_TRIGGER:
        id = weapon->trigger_identity;
        break;
    case BH_WEAPON_FIRE:
        id = weapon->fire_identity;
        break;
    case BH_WEAPON_TICK:
        id = weapon->tick_identity;
        break;
    case BH_WEAPON_HUD:
        id = weapon->hud_identity;
        break;
    default:
        return 0;
    }
    weapon_dispatch->identity = id;
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
        if (weapon_dispatch->ready)
            result = bh_weapon_ready_reset(weapon_dispatch->ready, weapon_entity, weapon_dispatch->player);
        break;
    case 0x004310c0UL:
        if (weapon_dispatch->target)
            result = bh_weapon_target_ready(weapon_dispatch->target, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00431130UL:
        if (weapon_dispatch->target)
            result = bh_weapon_target_cancel(weapon_dispatch->target, weapon_entity);
        break;
    case 0x00431180UL:
        if (weapon_dispatch->target)
            result = bh_weapon_target_update(weapon_dispatch->target, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00430d80UL:
        if (weapon_dispatch->target)
            result = bh_weapon_homing_track(weapon_dispatch->target, weapon_entity, weapon_dispatch->player);
        break;
    case 0x004324a0UL:
        if (weapon_dispatch->arc)
            result = bh_weapon_arc_activate(weapon_dispatch->arc, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00432510UL:
        if (weapon_dispatch->arc)
            result = bh_weapon_arc_deactivate(weapon_dispatch->arc, weapon_entity);
        break;
    case 0x00432c60UL:
        if (weapon_dispatch->arc)
            result = bh_weapon_arc_track(weapon_dispatch->arc, weapon_entity, weapon_dispatch->player);
        break;
    case 0x0042f730UL:
    case 0x004309d0UL:
    case 0x00432560UL:
        if (weapon_dispatch->empty)
            result = bh_weapon_empty_trigger(weapon_dispatch->empty, id, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00433dc0UL:
        if (weapon_dispatch->empty)
            result = bh_weapon_index9_trigger(weapon_dispatch->empty, weapon_entity, weapon_dispatch->player);
        break;
    case 0x004305f0UL:
        if (weapon_dispatch->fire)
            result = bh_weapon_fire_ballistic(weapon_dispatch->fire, weapon_entity, weapon_dispatch->player);
        break;
    case 0x0042f7a0UL:
        if (weapon_dispatch->pulse)
            result = bh_weapon_pulse_fire(weapon_dispatch->pulse, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00430a30UL:
        if (weapon_dispatch->fire)
            result = bh_weapon_fire_homing(weapon_dispatch->fire, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00431db0UL:
        if (weapon_dispatch->fire)
            result = bh_weapon_area_start(weapon_dispatch->fire, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00431f00UL:
        if (weapon_dispatch->fire)
            result = bh_weapon_area_tick(weapon_dispatch->fire, weapon_entity, weapon_dispatch->player);
        break;
    case 0x004327a0UL:
        if (weapon_dispatch->arc_fire)
            result = bh_weapon_arc_fire(weapon_dispatch->arc_fire, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00431810UL:
        if (weapon_dispatch->effects)
            result = bh_weapon_effect_orbit_fire(weapon_dispatch->effects, weapon_entity, weapon_dispatch->player);
        break;
    case 0x004331c0UL:
        if (weapon_dispatch->effects)
            result = bh_weapon_effect_split_fire(weapon_dispatch->effects, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00433850UL:
        if (weapon_dispatch->effects)
            result = bh_weapon_effect_accelerated_fire(weapon_dispatch->effects, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00433b30UL:
        if (weapon_dispatch->pulse_launch)
            result = bh_weapon_pulse_launch(weapon_dispatch->pulse_launch, weapon_entity, weapon_dispatch->player);
        break;
    case 0x00432ff0UL:
        if (!weapon_dispatch->arc_hud)
            break;
        p.position = weapon_dispatch->player->position;
        p.forward = weapon_dispatch->player->basis[2];
        p.aim = weapon_dispatch->player->player.aim_direction;
        result = bh_weapon_hud_arc(weapon_dispatch->arc_hud, &weapon->common.hud, &p);
        break;
    case 0x004314f0UL:
        if (weapon_dispatch->target_hud)
            result = bh_weapon_hud_target_draw(weapon_dispatch->target_hud, &weapon->common.hud);
        break;
    default:
        weapon_dispatch->error = 2;
        return 0;
    }
    if (!result)
        weapon_dispatch->error = 3;
    return result;
}
