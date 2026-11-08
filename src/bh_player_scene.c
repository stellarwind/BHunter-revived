#include "bh_player_scene.h"
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif
#include "bh_camera_queue.h"
#include "bh_horizon_queue.h"
#include "bh_material_queue.h"
#include "bh_quad_queue.h"
#include "bh_screen_quad.h"
#include "bh_draw_submit.h"
#include "bh_queue_reserve.h"
#include "bh_round.h"
#include "bh_pickup.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef char scene_player_pointer_size[sizeof(void *) == 4 ? 1 : -1];
typedef char scene_player_payload_offset[offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
typedef char scene_random_range[RAND_MAX == 32767 ? 1 : -1];

static BHVec3 vector_at(const BHEntity *entity, unsigned long offset)
{
    BHVec3 v;
    memcpy(&v, (const unsigned char *)entity + offset, 12);
    return v;
}
static float float_at(const BHEntity *entity, unsigned long offset)
{
    float v;
    memcpy(&v, (const unsigned char *)entity + offset, 4);
    return v;
}
static long long_at(const BHEntity *entity, unsigned long offset)
{
    long v;
    memcpy(&v, (const unsigned char *)entity + offset, 4);
    return v;
}
static unsigned long word_at(const BHEntity *entity, unsigned long offset)
{
    unsigned long v;
    memcpy(&v, (const unsigned char *)entity + offset, 4);
    return v;
}
static unsigned char byte_at(const BHEntity *entity, unsigned long offset)
{
    return *((const unsigned char *)entity + offset);
}
static void put_float(BHEntity *entity, unsigned long offset, float value)
{
    memcpy((unsigned char *)entity + offset, &value, 4);
}
static void put_byte(BHEntity *entity, unsigned long offset, unsigned char value)
{
    *((unsigned char *)entity + offset) = value;
}
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
    BHVec3 pos = vector_at(entity, 0xc), right = vector_at(entity, 0x18);
    BHVec3 up = vector_at(entity, 0x24), forward = vector_at(entity, 0x30);
    BHVec3 aim_offset = vector_at(entity, 0x28c), aim_vector = vector_at(entity, 0x280);
    BHVec3 center, axis, normal;
    float gauge, velocity, alpha, span, shift;
    unsigned long i;
    normal = direction(&right, &up, &forward, &aim_offset);
    normal.x = -normal.x;
    normal.y = -normal.y;
    normal.z = -normal.z;
    if (long_at(entity, 0xe8) == 0)
    {
        axis.x = -up.x;
        axis.y = -up.y;
        axis.z = -up.z;
        center =
            combined(&pos, &right, &up, &forward, stored((double)aim_offset.x * 1.1),
                     stored(-.30000001192092896 - (double)aim_offset.y * -1.1), stored((double)aim_offset.z * 1.1));
        queued(scene, bh_quad_queue(scene->queue, 3, scene->aim_image, &center, &axis, &normal, .25f, .25f, 0, 0, .5f,
                                    .00390625f, .9921875f, .00390625f, .9921875f));
        gauge = float_at(entity, 0x23c);
        if (!(gauge >= 0))
            gauge = 0;
        else if (gauge > 1)
            gauge = 1;
        velocity =
            stored(((double)vector_at(entity, 0x48).x * forward.x + (double)vector_at(entity, 0x48).y * forward.y +
                    (double)vector_at(entity, 0x48).z * forward.z) *
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
    state->gauge_a = float_at(entity, 0x24c);
    state->gauge_b = float_at(entity, 0x23c);
    state->weapons = ammunition;
    state->weapon_count = 10;
    state->primary = byte_at(entity, 0x2b6);
    state->secondary = byte_at(entity, 0x2b7);
    state->flags = byte_at(entity, 0x2b8);
    state->countdown_a = byte_at(entity, 0x2b9);
    state->countdown_b = byte_at(entity, 0x2ba);
    state->counter_a = long_at(entity, 0x230);
    state->counter_b = long_at(entity, 0x234);
    state->pending = long_at(entity, 0x238);
}
static void status_publish(BHPlayerScene *scene, const BHHudStatusState *state)
{
    put_byte(scene->player, 0x2b9, state->countdown_a);
    put_byte(scene->player, 0x2ba, state->countdown_b);
}

static void particles(BHPlayerScene *scene)
{
    BHEntity *entity = scene->player;
    float effect = float_at(entity, 0x240), remaining = float_at(entity, 0x244);
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
    pos = vector_at(entity, 0xc);
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
        BHVec3 motion = vector_at(entity, 0x2c0 + i * 12);
        BHVec3 from = vector_at(entity, 0x2fc + i * 24);
        BHVec3 to = vector_at(entity, 0x308 + i * 24);
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
    unsigned long resource = word_at(entity, 0x21c);
    BHVec3 position = vector_at(entity, 0xc), angles = vector_at(entity, 0x3c);
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
        if (byte_at(entity, 0x2b4) & 0x10)
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
        word_at(scene->player, 0x258 + index * 4) != (unsigned long)weapon)
        return fail(scene, BH_PLAYER_SCENE_WEAPON);
    switch (method)
    {
    case 0x0047d680UL: /* verified shared RET method */
        return 1;
    case 0x00432ff0UL:
        arc_player.position = vector_at(scene->player, 0xc);
        arc_player.forward = vector_at(scene->player, 0x30);
        arc_player.aim = vector_at(scene->player, 0x280);
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
            word_at(scene->player, 0x258 + index * 4) != (unsigned long)weapon)
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
    active = byte_at(entity, 0x2bb) == 1 && *scene->mode_count != 0;
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
        flag = (float)byte_at(entity, 0x2b8);
        oscillation = stored(sin((double)flag * .02454369260617026) * .1);
        queued(scene, bh_material_queue_words(scene->queue, 5, scene->material_image, 1, bits(.05f), 0,
                                              bits(stored((double)flag * .00390625)), 0, bits(.05f),
                                              bits(stored((double)oscillation - (double)flag * -.00390625))));
        for (i = 0; i < 8; ++i)
            if (bh_scene_background_step(scene->background, i, eye) < 0)
                fail(scene, BH_PLAYER_SCENE_OWNER);
        if (*scene->frame_count < 150 || *scene->mode_count > 1)
        {
            flash = float_at(entity, 0x254);
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
                gauge = float_at(entity, 0x24c);
                component = (long)((1.0 - (double)gauge) * -127.0);
                color = (0xff00UL - (unsigned long)component) << 8;
                if ((double)amount > .05)
                    queued(scene, bh_fade_queue(scene->queue, amount, color));
                put_float(entity, 0x254, stored((.65 - (double)float_at(entity, 0x23c) * -.35) * flash));
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
        if (!bh_hud_markers_draw(scene->markers, *scene->actor_head, &entity->position,
                                 (const unsigned char *)entity + 0x2b8))
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
        if (!selected(scene, byte_at(entity, 0x2b6)))
            return 0;
        if (!selected(scene, byte_at(entity, 0x2b7)))
            return 0;
        for (i = 0; i < 10; ++i)
            ammunition[i] = 0;
        status_state(scene, &status, ammunition);
        if (!status_weapons(scene, &status, ammunition))
            return 0;
#ifdef BH_SDL_BACKEND
        attached[4] = bh_interpolation_mark();
#endif
        if (long_at(entity, 0xe8) == 0)
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
        if (!bh_scene_owner_step(scene->scene, eye, angles, 60, .01f, 25000, *scene->display_width,
                                 *scene->display_height) &&
            scene->scene->error != BH_SCENE_OWNER_INACTIVE &&
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
