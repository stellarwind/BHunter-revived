#include "bh_player_scene.h"
#ifdef BH_SDL_BACKEND
#include "../../Graphics/bh_render_interpolation.h"
#endif
#include "../../Camera/bh_camera_queue.h"
#include "../../Graphics/bh_horizon_queue.h"
#include "../../Graphics/bh_material_queue.h"
#include "../../Graphics/Geometry/bh_quad_queue.h"
#include "../../Graphics/Geometry/bh_screen_quad.h"
#include "../../Graphics/bh_draw_submit.h"
#include "../../Core/bh_queue_reserve.h"
#include "../../Math/bh_round.h"
#include "../Items/bh_pickup.h"
#include "../Actors/bh_remote_packets.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

static_assert(sizeof(void *) == 4, "scene player pointer size");
static_assert(offsetof(BHEntity, payload) == 0xe4, "BHEntity.payload offset");
static_assert(RAND_MAX == 32767, "scene random range");

static float stored(double n)
{
    float f;
    bh_round_float(&f, n);
    return f;
}
static unsigned long bits(float value)
{
    unsigned long u;
    memcpy(&u, &value, 4);
    return u;
}
static int random_value(BHPlayerScene *scene)
{
    ++scene->random_calls;
    return rand();
}
static void queued(BHPlayerScene *scene, int result)
{
    ++scene->attempted;
    if (result > 0)
        ++scene->queued;
    else if (result == 0)
        ++scene->dropped;
    else if (scene->error == BH_PLAYER_SCENE_OK)
        scene->error = BH_PLAYER_SCENE_QUEUE;
}
static int fail(BHPlayerScene *scene, BHPlayerSceneError error)
{
    if (scene)
        scene->error = error;
    return 0;
}
static int finite_vec(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static BHVec3 combined(const BHVec3 *position, const BHVec3 *right, const BHVec3 *up, const BHVec3 *forward, float x,
                       float y, float z)
{
    BHVec3 v;
    v.x = stored((((double)up->x * y + (double)forward->x * z) + (double)right->x * x) + position->x);
    v.y = stored((((double)up->y * y + (double)forward->y * z) + (double)right->y * x) + position->y);
    v.z = stored((((double)up->z * y + (double)forward->z * z) + (double)right->z * x) + position->z);
    return v;
}
static BHVec3 direction(const BHVec3 *right, const BHVec3 *up, const BHVec3 *forward, const BHVec3 *local)
{
    BHVec3 v;
    v.x = stored(((double)forward->x * local->z + (double)up->x * local->y) + (double)right->x * local->x);
    v.y = stored(((double)up->y * local->y + (double)forward->y * local->z) + (double)right->y * local->x);
    v.z = stored(((double)right->z * local->x + (double)up->z * local->y) + (double)forward->z * local->z);
    return v;
}
static void aim(BHPlayerScene *scene)
{
    BHEntity *entity = scene->player;
    BHVec3 pos = entity->position, right = entity->basis[0];
    BHVec3 up = entity->basis[1], forward = entity->basis[2];
    BHVec3 aim_offset = entity->player.aim_offset, aim_vector = entity->player.aim_direction;
    BHVec3 center, axis, normal;
    float gauge, velocity, alpha, span, shift;
    unsigned long i;
    normal = direction(&right, &up, &forward, &aim_offset);
    normal.x = -normal.x;
    normal.y = -normal.y;
    normal.z = -normal.z;
    if (entity->player.camera.current.mode == 0)
    {
        axis.x = -up.x;
        axis.y = -up.y;
        axis.z = -up.z;
        center =
            combined(&pos, &right, &up, &forward, stored((double)aim_offset.x * 1.1),
                     stored(-.30000001192092896 - (double)aim_offset.y * -1.1), stored((double)aim_offset.z * 1.1));
        queued(scene, bh_quad_queue(scene->queue, 3, scene->aim_image, &center, &axis, &normal, .25f, .25f, 0, 0, .5f,
                                    .00390625f, .9921875f, .00390625f, .9921875f));
        gauge = entity->player.damage;
        if (!(gauge >= 0))
            gauge = 0;
        else if (gauge > 1)
            gauge = 1;
        velocity = stored(((double)entity->velocity.x * forward.x + (double)entity->velocity.y * forward.y +
                           (double)entity->velocity.z * forward.z) *
                          .006666666666666667);
        if (!(velocity >= 0))
            velocity = 0;
        else if (velocity > 1)
            velocity = 1;
        if (gauge > 0 || velocity > 0)
        {
            shift = stored((-.3 - (double)(velocity - gauge) * .05275) - (double)aim_offset.y * -1.1);
            center = combined(&pos, &right, &up, &forward, stored((double)aim_offset.x * 1.1), shift,
                              stored((double)aim_offset.z * 1.1));
            alpha = stored((velocity + gauge) * .1055);
            span = stored(.5 - (double)velocity * .422);
            queued(scene, bh_quad_queue(scene->queue, 3, scene->aim_overlay_image, &center, &axis, &normal, .25f, alpha,
                                        0, 0, .5f, .00390625f, .9921875f, span, stored((velocity + gauge) * .422)));
        }
    }
    else
    {
        axis.x = -up.x;
        axis.y = -up.y;
        axis.z = -up.z;
        center = combined(&pos, &right, &up, &forward, 0, -.30000001192092896f, 0);
        center.x = stored((double)center.x - (double)aim_vector.x * -10.0);
        center.y = stored((double)center.y - (double)aim_vector.y * -10.0);
        center.z = stored((double)center.z - (double)aim_vector.z * -10.0);
        for (i = 0; i < 3; ++i)
        {
            if (i)
            {
                float step = i == 1 ? -10.0f : -20.0f;
                center.x = stored((double)center.x - (double)aim_vector.x * step);
                center.y = stored((double)center.y - (double)aim_vector.y * step);
                center.z = stored((double)center.z - (double)aim_vector.z * step);
            }
            queued(scene, bh_quad_queue(scene->queue, 3, scene->aim_image, &center, &axis, &normal, 3, 3, 0, 0, .6f,
                                        .00390625f, .9921875f, .00390625f, .9921875f));
        }
    }
}

static void status_state(BHPlayerScene *scene, BHHudStatusState *state, const BHHudWeaponAmmo *const *ammunition)
{
    BHEntity *entity = scene->player;
    state->gauge_a = entity->player.wear;
    state->gauge_b = entity->player.damage;
    state->weapons = ammunition;
    state->weapon_count = 10;
    state->primary = entity->player.selected_weapon[0];
    state->secondary = entity->player.selected_weapon[1];
    state->flags = entity->player.frame_phase;
    state->countdown_a = entity->player.cash_flash;
    state->countdown_b = entity->player.fine_flash;
    state->counter_a = entity->player.cash;
    state->counter_b = entity->player.fine;
    state->pending = entity->player.bounty_pending;
}
static void status_publish(BHPlayerScene *scene, const BHHudStatusState *state)
{
    scene->player->player.cash_flash = state->countdown_a;
    scene->player->player.fine_flash = state->countdown_b;
}

static void particles(BHPlayerScene *scene)
{
    BHEntity *entity = scene->player;
    float effect = entity->player.shield_time, remaining = entity->player.shield_fade;
    float size, alpha, fade;
    BHVec3 pos, offset, up = {0, -1, 0}, normal;
    unsigned long i;
    if (!(effect > 0))
        return;
    size = stored(3.0 - sin((double)effect * 3.0) * -.5);
    if (!(remaining > 0))
        fade = effect < 2 ? stored((double)effect * .5) : 1;
    else
        fade = stored(1.0 - (double)remaining);
    alpha = stored((double)fade * .3);
    pos = entity->position;
    for (i = 0; i < 32; ++i)
    {
        offset.x = (float)(random_value(scene) - 0x4000);
        offset.y = (float)(random_value(scene) - 0x4000);
        offset.z = (float)(random_value(scene) - 0x4000);
        bh_vec3_normalize(&offset);
        offset.x = stored((double)offset.x * size);
        offset.y = stored((double)offset.y * size);
        offset.z = stored((double)offset.z * size);
        normal = offset;
        offset.x = stored((double)pos.x + offset.x);
        offset.y = stored((double)pos.y + offset.y);
        offset.z = stored((double)pos.z + offset.z);
        queued(scene, bh_draw_oriented(scene->queue, 3, scene->particle_image, &offset, &up, &normal, size, size,
                                       stored((double)random_value(scene) * .0001953125), 8, alpha));
    }
}

static void trail(BHPlayerScene *scene)
{
    unsigned long i;
    BHEntity *entity = scene->player;
    for (i = 0; i < 4; ++i)
    {
        BHVec3 motion = entity->player.propulsion.thrust.nozzles[i];
        BHVec3 from = entity->player.nozzle_trail[i].from;
        BHVec3 to = entity->player.nozzle_trail[i].to;
        int random = random_value(scene);
        float width, alpha;
        width = stored(
            sqrt((double)motion.z * motion.z + ((double)motion.x * motion.x - (double)motion.y * motion.y * -4.0)) +
            .5);
        alpha = stored(.5 - (double)random * -1.52587890625e-05);
        queued(scene, bh_draw_segment(scene->queue, 3, scene->trail_image, &from, &to, .5f, width, 1, alpha, 0));
    }
}

static unsigned long tail(BHPlayerScene *scene)
{
    BHEntity *entity = scene->player;
    unsigned long anchor = BH_DRAW_END;
    unsigned long resource = entity->player.model_resource;
    BHVec3 position = entity->position, angles = entity->euler;
#ifdef BH_SDL_BACKEND
    unsigned long render_mark = bh_interpolation_mark();
#endif
    if (resource)
    {
        anchor = scene->queue->count;
        queued(scene, bh_draw_submit_resource(scene->queue, 0, resource, &position, &angles, 1));
        if (anchor == scene->queue->count)
            anchor = BH_DRAW_END;
    }
#ifdef BH_SDL_BACKEND
    /* The tail is emitted after a visibility-dependent world/HUD command list.
       Its identity must follow the player, never its offset in that list. */
    bh_interpolation_stamp(render_mark, (unsigned long)entity, entity->generation);
#endif
    if (*scene->frame_count < 75)
    {
        if ((unsigned char)entity->player.disabled_frames & 0x10)
        {
            BHDrawCommand *command = bh_queue_reserve(scene->queue, 8);
            ++scene->attempted;
            if (command)
            {
                ++scene->queued;
                command->kind = BH_DRAW_STATE;
                command->words[0] = 3;
            }
            else if (scene->queue->error == BH_DRAW_FULL)
                ++scene->dropped;
            else if (scene->error == BH_PLAYER_SCENE_OK)
                scene->error = BH_PLAYER_SCENE_QUEUE;
            command = bh_queue_reserve(scene->queue, 8);
            ++scene->attempted;
            if (command)
            {
                float half = (float)(*scene->display_width / 2);
                ++scene->queued;
                command->kind = BH_DRAW_SCREEN_QUAD;
                command->resource = scene->overlay_image;
                command->words[0] = bits(half);
                command->words[1] = bits(stored((double)*scene->display_height * .75));
                command->words[2] = bits(half);
                command->words[3] = bits(half);
                command->words[4] = 0;
                command->words[5] = bits(1);
                command->words[6] = 0;
            }
            else if (scene->queue->error == BH_DRAW_FULL)
                ++scene->dropped;
            else if (scene->error == BH_PLAYER_SCENE_OK)
                scene->error = BH_PLAYER_SCENE_QUEUE;
        }
#ifdef BH_SDL_BACKEND
        render_mark = bh_interpolation_mark();
#endif
        queued(scene, bh_draw_submit_resource(scene->queue, 0, scene->cockpit_resource, &position, &angles, 1));
#ifdef BH_SDL_BACKEND
        bh_interpolation_stamp(render_mark, (unsigned long)entity, entity->generation);
#endif
        trail(scene);
    }
    return anchor;
}

static int selected(BHPlayerScene *scene, unsigned char index)
{
    BHEntity *weapon;
    BHWeaponObjectState *state;
    unsigned long method;
    BHWeaponHudArcPlayer arc_player;
    if (!bh_weapon_registry_hud(scene->weapons, index, &weapon, &state, &method) ||
        scene->player->player.weapons[index] != weapon)
        return fail(scene, BH_PLAYER_SCENE_WEAPON);
    switch (method)
    {
    case 0x0047d680UL: /* verified shared RET method */
        return 1;
    case 0x00432ff0UL:
        arc_player.position = scene->player->position;
        arc_player.forward = scene->player->basis[2];
        arc_player.aim = scene->player->player.aim_direction;
        if (!bh_weapon_hud_arc(scene->arc, &state->common.hud, &arc_player))
            return fail(scene, BH_PLAYER_SCENE_OWNER);
        scene->random_calls += scene->arc->random_calls;
        return 1;
    case 0x004314f0UL:
        scene->target->viewport_width = *scene->display_width;
        if (!bh_weapon_hud_target_draw(scene->target, &state->common.hud))
            return fail(scene, BH_PLAYER_SCENE_OWNER);
        return 1;
    default:
        return fail(scene, BH_PLAYER_SCENE_WEAPON);
    }
}
static int status_weapons(BHPlayerScene *scene, BHHudStatusState *status, const BHHudWeaponAmmo **ammunition)
{
    unsigned char chosen[2];
    unsigned long i;
    chosen[0] = status->primary;
    chosen[1] = status->secondary;
    for (i = 0; i < 2; ++i)
    {
        BHEntity *weapon;
        BHWeaponObjectState *state;
        unsigned long identity, index = chosen[i];
        if (!bh_weapon_registry_hud(scene->weapons, index, &weapon, &state, &identity) ||
            scene->player->player.weapons[index] != weapon)
            return fail(scene, BH_PLAYER_SCENE_WEAPON);
        ammunition[index] = (const BHHudWeaponAmmo *)weapon;
    }
    return 1;
}

int bh_player_scene_draw(BHPlayerScene *scene)
{
    BHEntity *entity;
    BHCameraState *camera;
    BHHudStatusState status;
    const BHHudWeaponAmmo *ammunition[10];
    unsigned long i;
    int active;
#ifdef BH_SDL_BACKEND
    unsigned long render_mark, attached[6], anchor;
#endif
    if (!scene || !(entity = scene->player) || !scene->queue || !scene->frame_count || !scene->display_width ||
        !scene->display_height || !scene->mode_count)
        return fail(scene, BH_PLAYER_SCENE_ARGUMENT);
    scene->error = BH_PLAYER_SCENE_OK;
    scene->attempted = scene->queued = scene->dropped = scene->random_calls = 0;
    active = entity->player.vehicle == 1 && *scene->mode_count != 0;
    if (active)
    {
        float flag, flash, amount, gauge, oscillation;
        BHVec3 *eye, *angles, *velocity;
        if (!scene->sound || !scene->hud || !scene->hud->frame || !scene->markers || !scene->radar ||
            !scene->world_status || !scene->flat_status || !scene->scene || !scene->background || !scene->actor_head ||
            !scene->pickup_head || !scene->weapons || !scene->arc || !scene->target || !scene->confirmation_gate ||
            (*scene->mode_count > 1 && (!scene->score_draw || !scene->scores)))
            return fail(scene, BH_PLAYER_SCENE_BINDING);
        if (!scene->hud->attachment || scene->hud->attachment->queue != scene->queue ||
            scene->radar->queue != scene->queue || !scene->markers->projection ||
            scene->markers->projection->queue != scene->queue || scene->markers->radar != scene->radar ||
            scene->world_status->hud != scene->hud || scene->flat_status->hud != scene->hud ||
            scene->background->queue != scene->queue || scene->arc->queue != scene->queue ||
            !scene->target->projection || scene->target->projection->queue != scene->queue ||
            (*scene->mode_count > 1 && scene->score_draw->queue != scene->queue))
            return fail(scene, BH_PLAYER_SCENE_BINDING);
        camera = bh_player_camera_state(entity);
        eye = &camera->position;
        angles = &camera->angles;
        velocity = &camera->velocity;
        if (!finite_vec(eye) || !finite_vec(angles) || !finite_vec(velocity))
            return fail(scene, BH_PLAYER_SCENE_ARGUMENT);
        if (!bh_sound_listener(scene->sound, 0, 1, eye, angles, velocity))
            fail(scene, BH_PLAYER_SCENE_OWNER);
#ifdef BH_SDL_BACKEND
        render_mark = bh_interpolation_mark();
#endif
        queued(scene, bh_camera_queue_pose(scene->queue, eye, angles));
#ifdef BH_SDL_BACKEND
        bh_interpolation_stamp(render_mark, (unsigned long)camera, entity->generation);
#endif
        queued(scene, bh_horizon_queue(scene->queue, scene->horizon_image, 5000, 5000, 25000, 12500, 0x804000));
        flag = (float)entity->player.frame_phase;
        oscillation = stored(sin((double)flag * .02454369260617026) * .1);
        queued(scene, bh_material_queue_words(scene->queue, 5, scene->material_image, 1, bits(.05f), 0,
                                              bits(stored((double)flag * .00390625)), 0, bits(.05f),
                                              bits(stored((double)oscillation - (double)flag * -.00390625))));
        for (i = 0; i < 8; ++i)
            if (bh_scene_background_step(scene->background, i, eye) < 0)
                fail(scene, BH_PLAYER_SCENE_OWNER);
        if (*scene->frame_count < 150 || *scene->mode_count > 1)
        {
            flash = entity->player.damage_flash;
            if (!((double)flash > .1))
            {
                queued(scene, bh_fade_queue(scene->queue, 0, 0));
            }
            else
            {
                unsigned long color;
                long component;
                amount = stored((double)flash * flash);
                if ((double)amount > .9)
                    amount = .9f;
                gauge = entity->player.wear;
                component = (long)((1.0 - (double)gauge) * -127.0);
                color = (0xff00UL - (unsigned long)component) << 8;
                if ((double)amount > .05)
                    queued(scene, bh_fade_queue(scene->queue, amount, color));
                entity->player.damage_flash = stored((.65 - (double)entity->player.damage * -.35) * flash);
            }
        }
        else
        {
            amount = stored((double)(*scene->frame_count - 150) * .019999999552965164);
            if ((double)amount > 1)
                amount = 1;
            if ((double)amount > .05)
                queued(scene, bh_fade_queue(scene->queue, amount, 0));
        }
        aim(scene);
#ifdef BH_SDL_BACKEND
        attached[0] = bh_interpolation_mark();
#endif
        scene->hud->attachment->viewport_width = *scene->display_width;
        if (!bh_hud_setup(scene->hud, (const BHHudPlayer *)entity))
            fail(scene, BH_PLAYER_SCENE_OWNER);
#ifdef BH_SDL_BACKEND
        attached[1] = bh_interpolation_mark();
#endif
#ifdef BH_SDL_BACKEND
        attached[2] = bh_interpolation_mark();
#endif
        scene->markers->display_width = *scene->display_width;
        if (!bh_hud_markers_draw(scene->markers, *scene->actor_head, &entity->position, &entity->player.frame_phase))
            fail(scene, BH_PLAYER_SCENE_OWNER);
        {
            BHEntity *pickup = *scene->pickup_head;
            unsigned long visited = 0;
            while (pickup)
            {
                BHEntity *next;
                if (visited++ >= scene->pickup_limit)
                {
                    fail(scene, BH_PLAYER_SCENE_BINDING);
                    break;
                }
                if (!bh_radar_marker(scene->radar, &entity->position, &pickup->position, 2, 0, 0))
                    fail(scene, BH_PLAYER_SCENE_OWNER);
                next = bh_pickup_state(pickup)->global_next;
                pickup = next;
            }
        }
#ifdef BH_SDL_BACKEND
        attached[3] = bh_interpolation_mark();
#endif
        /* Resolve each selector only when its synchronous callback is due. */
        if (!selected(scene, entity->player.selected_weapon[0]))
            return 0;
        if (!selected(scene, entity->player.selected_weapon[1]))
            return 0;
        for (i = 0; i < 10; ++i)
            ammunition[i] = 0;
        status_state(scene, &status, ammunition);
        if (!status_weapons(scene, &status, ammunition))
            return 0;
#ifdef BH_SDL_BACKEND
        attached[4] = bh_interpolation_mark();
#endif
        if (entity->player.camera.current.mode == 0)
        {
            if (!bh_hud_world_draw(scene->world_status, &status))
                fail(scene, BH_PLAYER_SCENE_OWNER);
        }
        else
        {
            scene->flat_status->viewport_height = *scene->display_height;
            if (!bh_hud_flat_draw(scene->flat_status, &status))
                fail(scene, BH_PLAYER_SCENE_OWNER);
        }
#ifdef BH_SDL_BACKEND
        attached[5] = bh_interpolation_mark();
#endif
        status_publish(scene, &status);
        if (*scene->mode_count > 1)
        {
            scene->score_draw->display_width = *scene->display_width;
            scene->score_draw->near_z = scene->hud->frame->near_z;
            if (!bh_hud_scores_draw(scene->score_draw, scene->scores))
                fail(scene, BH_PLAYER_SCENE_OWNER);
        }
        if (bh_scene_owner_step(scene->scene, eye, angles, scene->scene->view_fov_degrees, .01f, 25000,
                                *scene->display_width, *scene->display_height))
        {
            if (*scene->mode_count > 1 && scene->remote_packets)
                bh_remote_draw_scene(scene->remote_packets);
        }
        else if (scene->scene->error != BH_SCENE_OWNER_INACTIVE &&
                 !(scene->scene->error == BH_SCENE_OWNER_QUEUE && scene->queue->error == BH_DRAW_FULL))
            fail(scene, BH_PLAYER_SCENE_OWNER);
        if (*scene->confirmation_gate)
        {
            float half = stored((double)*scene->display_width * .5);
            float size = stored((double)*scene->display_width * .2);
            queued(scene, bh_screen_quad_queue(scene->queue, 7, scene->confirmation_image, half,
                                               stored((double)*scene->display_height * .5), size, size, 0, 0, 1));
        }
        particles(scene);
    }
#ifdef BH_SDL_BACKEND
    anchor = tail(scene);
    if (active && anchor != BH_DRAW_END)
        for (i = 0; i < 6; i += 2)
            bh_interpolation_attach(attached[i], attached[i + 1], anchor);
#else
    tail(scene);
#endif
    return scene->error == BH_PLAYER_SCENE_OK;
}
