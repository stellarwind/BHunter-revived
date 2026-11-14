#include "bh_flash_flare.h"
#include "../../Core/bh_queue_reserve.h"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <float.h>
#include <math.h>
#include <string.h>
typedef struct FlareRow
{
    float position, size, alpha;
    unsigned long texture;
} FlareRow;
static const FlareRow rows[14] = {{1, -1, 1, 0},     {-1, 300, .1f, 1},  {1.7f, 150, .1f, 2}, {-1.1f, 80, .2f, 3},
                                  {.3f, 70, .5f, 3}, {-.7f, 70, .5f, 2}, {.5f, 60, .4f, 2},   {-.5f, 60, .4f, 3},
                                  {.7f, 50, .3f, 3}, {-.3f, 50, .3f, 2}, {.4f, 40, .2f, 2},   {-.4f, 40, .2f, 3},
                                  {.8f, 40, .1f, 2}, {-.9f, 40, .1f, 3}};
static float number(const unsigned long *p)
{
    float x;
    memcpy(&x, p, 4);
    return x;
}
static void store(unsigned long *p, float x)
{
    memcpy(p, &x, 4);
}
static int greater(double a, double b)
{
    return !_isnan(a) && !_isnan(b) && a > b;
}
static int greater_equal(double a, double b)
{
    return !_isnan(a) && !_isnan(b) && a >= b;
}
static BHFlashFlareContext *context(BHEntity *entity)
{
    BHFlashFlareContext *flash_context;
    memcpy(&flash_context, &entity->bindings.flash_flare, 4);
    return flash_context;
}
int bh_flash_flare_bind(BHFlashFlareContext *flash_context, BHEntity *entity, float lifetime, float strength, float size)
{
    if (!flash_context || !flash_context->pool || !flash_context->queue || !flash_context->queue->commands ||
        !bh_script_flash_bind(flash_context->pool, entity, lifetime))
        return 0;
    memcpy(&entity->flash.strength, &strength, 4);
    memcpy(&entity->flash.size, &size, 4);
    memcpy(&entity->bindings.flash_flare, &flash_context, 4);
    entity->refresh = bh_flash_flare_refresh;
    return 1;
}
void bh_flash_flare_refresh(BHEntity *entity, void *packet)
{
    BHFlashFlareContext *flash_context = context(entity);
    float life, strength, size, value;
    (void)packet;
    memcpy(&life, &entity->flash.countdown, 4);
    memcpy(&strength, &entity->flash.strength, 4);
    memcpy(&size, &entity->flash.size, 4);
    bh_round_float(&value, (1.0 - (double)life * 10.0) * strength);
    bh_flare_queue(flash_context->queue, &entity->position, size, value, flash_context->textures[0],
                   flash_context->textures[1], flash_context->textures[2], flash_context->textures[3], 0);
}
int bh_flare_queue(BHDrawQueue *queue, const BHVec3 *position, float size, float strength, unsigned long first,
                   unsigned long second, unsigned long third, unsigned long fourth, float angle)
{
    BHDrawCommand *command;
    if (!position)
        return -1;
    command = bh_queue_reserve(queue, 7);
    if (!command)
        return queue && queue->error == BH_DRAW_FULL ? 0 : -1;
    command->kind = BH_DRAW_FLARE;
    command->resource = 0;
    memcpy(command->words, position, 12);
    store(command->words + 3, size);
    store(command->words + 4, strength);
    store(command->words + 5, angle);
    command->words[6] = first;
    command->words[7] = second;
    command->words[8] = third;
    command->words[9] = fourth ? fourth : third;
    return 1;
}
int bh_flare_execute(BHFlareSubmit *submit, BHDrawCommand *command, unsigned long *next)
{
    float x, y, scale, norm, dx, dy, gain, remaining, size, alpha, sx, sy, angle;
    double normalized, threshold, deltax, deltay, radius, weight, product, n;
    long halfx, halfy;
    unsigned long i, id;
    BHTextureImage *image;
    int ok = 1;
    if (!submit || !submit->camera || !submit->view || !submit->screen || !submit->screen->draw || !submit->screen->lookup ||
        !command || !next || command->kind != BH_DRAW_FLARE || submit->width < 2 || submit->height <= 0 ||
        !(submit->camera->focal > 0) || !(submit->view->near_z > 0) || !bh_math_environment_supported())
        return 0;
    submit->attempted = submit->emitted = submit->culled = submit->failures = 0;
    x = number(command->words);
    y = number(command->words + 1);
    scale = number(command->words + 2);
    if (!bh_project_point_scale(submit->camera, submit->view, &x, &y, &scale))
    {
        ++submit->culled;
        *next = command->next;
        return 1;
    }
    halfx = submit->width / 2;
    halfy = submit->height / 2;
    threshold = 1.0 / (double)scale;
    normalized = (double)number(command->words + 4) / halfx;
    bh_round_float(&norm, normalized);
    store(command->words + 4, norm);
    if (greater(threshold, normalized))
    {
        ++submit->culled;
        *next = command->next;
        return 1;
    }
    deltax = (double)x - halfx;
    deltay = (double)y - halfy;
    bh_round_float(&dx, deltax);
    bh_round_float(&dy, deltay);
    radius = sqrt(deltax * dx + deltay * dy);
    if (greater_equal(radius, (double)submit->height))
    {
        ++submit->culled;
        *next = command->next;
        return 1;
    }
    weight = ((double)norm - threshold) / (double)norm;
    bh_round_float(&gain, weight * (1.0 - radius / (double)submit->height));
    product = (double)gain * 14.0;
    bh_round_float(&remaining, product);
    if (greater(product, 2.0) && !command->words[8] && !command->words[9])
        remaining = 2;
    angle = number(command->words + 5);
    /* Normal positive-camera input yields at most14 entries. An invalid source
    magnitude never reads beyond the complete original immutable table. */
    if (greater(remaining, 14))
    {
        ++submit->failures;
        *next = command->next;
        return 0;
    }
    i = 0;
    while (greater(remaining, 0))
    {
        id = command->words[6 + rows[i].texture];
        if (id)
        {
            n = greater(1.0, remaining) ? (double)remaining * gain : gain;
            bh_round_float(&alpha, n * rows[i].alpha);
            n = rows[i].size;
            if (n < 0)
                n = -((n * (double)number(command->words + 3)) * scale);
            bh_round_float(&size, (n * (double)submit->width) * .0015625);
            bh_round_float(&sx, (double)dx * rows[i].position + halfx);
            bh_round_float(&sy, (double)dy * rows[i].position + halfy);
            ++submit->attempted;
            image = submit->screen->lookup(id, submit->screen->lookup_user);
            if (!image || !bh_screen_quad_draw(submit->screen->draw, image, sx, sy, size, size, angle, 0, alpha))
            {
                ++submit->failures;
                ok = 0;
            }
            else if (submit->screen->draw->emitted)
                ++submit->emitted;
            else
                ++submit->culled;
        }
        ++i;
        n = (double)remaining - 1.0;
        bh_round_float(&remaining, n);
        if (!greater(n, 0))
            break;
    }
    *next = command->next;
    return ok;
}
