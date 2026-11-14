#include "bh_player_camera.h"
#include "../../Math/bh_rotation.h"
#include "../../Math/bh_round.h"
#include <stddef.h>
#include <string.h>
#include <float.h>

static_assert(sizeof(BHPlayerCameraStart) == 32, "BHPlayerCameraStart size");
static_assert(sizeof(void *) == 4, "player camera pointer size");
static_assert(offsetof(BHEntity, position) == 0xc, "BHEntity.position offset");
static_assert(offsetof(BHEntity, payload) == 0xe4, "BHEntity.payload offset");

static int finite_vector(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static void translate(BHVec3 *v, const BHVec3 *delta, double delta_x)
{
    bh_round_float(&v->x, (double)v->x + delta_x);
    bh_round_float(&v->y, (double)v->y + delta->y);
    bh_round_float(&v->z, (double)v->z + delta->z);
}
BHCameraState *bh_player_camera_state(BHEntity *player)
{
    return player ? &player->player.camera : 0;
}
int bh_player_camera_reset(BHPlayerCameraContext *context, unsigned long index)
{
    BHEntity *player;
    BHCameraState *camera;
    BHCameraInput input;
    BHVec3 position, delta, zero = {0, 0, 0}, axis;
    BHMat34 matrix = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};
    unsigned long i;
    double delta_x;
    if (!context || !context->player || !context->camera || !context->starts || index >= context->start_count)
        return 0;
    player = context->player;
    camera = bh_player_camera_state(player);
    position = context->starts[index].position;
    input.angles = context->starts[index].angles;
    if (!finite_vector(&position) || !finite_vector(&input.angles) || !finite_vector(&player->position) ||
        !finite_vector(&camera->current.position) || !finite_vector(&camera->previous.position) ||
        !finite_vector(&camera->position) || !finite_vector(&camera->current.angles) ||
        !_finite(camera->current.seed_yaw) || camera->current.mode < 0 || camera->current.mode >= 12 ||
        !_finite(camera->elapsed) || !_finite(camera->blend_remaining) || !_finite(camera->look_yaw) ||
        !_finite(camera->look_pitch))
        return 0;
    if (context->player_count > 1)
        bh_round_float(&position.x, (double)position.x - (-1.0 - (double)context->local_player_index * -.25));
    delta_x = (double)position.x - player->position.x;
    bh_round_float(&delta.y, (double)position.y - player->position.y);
    bh_round_float(&delta.z, (double)position.z - player->position.z);
    player->position = position;
    translate(&camera->position, &delta, delta_x);
    translate(&camera->current.position, &delta, delta_x);
    translate(&camera->previous.position, &delta, delta_x);
    player->velocity = zero;
    player->angular_velocity = zero;
    player->acceleration = zero;
    player->angular_acceleration = zero;
    player->euler = input.angles;
    bh_mat34_rotate_y(input.angles.x, &matrix);
    bh_mat34_rotate_x(input.angles.y, &matrix);
    bh_mat34_rotate_z(input.angles.z, &matrix);
    axis.x = 1;
    axis.y = axis.z = 0;
    bh_mat34_transform_direction(&matrix, &axis, &input.basis_x);
    axis.y = 1;
    axis.x = 0;
    bh_mat34_transform_direction(&matrix, &axis, &input.basis_y);
    axis.z = 1;
    axis.y = 0;
    bh_mat34_transform_direction(&matrix, &axis, &input.basis_z);
    player->basis[0] = input.basis_x;
    player->basis[1] = input.basis_y;
    player->basis[2] = input.basis_z;
    matrix.m[0][0] = input.basis_x.x;
    matrix.m[1][0] = input.basis_x.y;
    matrix.m[2][0] = input.basis_x.z;
    matrix.m[0][1] = input.basis_y.x;
    matrix.m[1][1] = input.basis_y.y;
    matrix.m[2][1] = input.basis_y.z;
    matrix.m[0][2] = input.basis_z.x;
    matrix.m[1][2] = input.basis_z.y;
    matrix.m[2][2] = input.basis_z.z;
    matrix.m[0][3] = position.x;
    matrix.m[1][3] = position.y;
    matrix.m[2][3] = position.z;
    player->last_clear = matrix;
    player->player.camera_mode = 1;
    if (!bh_camera_state_set_mode(camera, 1))
        return 0;
    input.target = player->position;
    input.actor_token = (unsigned long)player;
    for (i = 0; i < 50; ++i)
        if (!bh_camera_state_update(context->camera, camera, &input, 0.033333333333333333333, 0, 0))
            return 0;
    player->player.recoil_position = player->position;
    player->player.recoil_velocity = zero;
    return 1;
}
