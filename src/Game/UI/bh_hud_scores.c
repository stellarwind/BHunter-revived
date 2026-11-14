#include "bh_hud_scores.h"
#include "../../Graphics/Textures/bh_atlas_text.h"
#include "../../Math/bh_round.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <float.h>

static_assert(offsetof(BHEntity, payload) == 0xe4, "BHEntity.payload offset");
static_assert(sizeof(long) == 4 && sizeof(int) == 4 && sizeof(short) == 2, "scores word size");
static long score(const BHEntity *entity)
{
    short value;
    memcpy(&value, &entity->remote.score, 2);
    return value;
}
static long row_y(long index)
{
    unsigned long value = (unsigned long)index * 16UL + 2UL;
    long result;
    memcpy(&result, &value, 4);
    return result;
}
static void text_result(BHHudScoreText *hud_score_text, int result)
{
    ++hud_score_text->attempted;
    if (result > 0)
        ++hud_score_text->queued;
    else if (!result)
        ++hud_score_text->dropped;
    else
        hud_score_text->error = BH_HUD_SCORES_QUEUE;
}
static void row(BHHudScoreText *hud_score_text, const char *name, long amount, long y)
{
    char buffer[128];
    long width;
    text_result(hud_score_text, bh_text_queue_text(hud_score_text->queue, hud_score_text->page, name, 1, y));
    sprintf(buffer, "%d", (int)amount);
    width = bh_atlas_page_text_width(hud_score_text->page, (const unsigned char *)buffer);
    text_result(hud_score_text, bh_text_queue_text(hud_score_text->queue, hud_score_text->page, buffer, 126 - width, y));
}
int bh_hud_scores_text(BHHudScoreText *hud_score_text, const BHHudScores *hud_scores)
{
    unsigned long i;
    int cleared = 0;
    if (!hud_score_text || !hud_scores || !hud_score_text->queue || !hud_scores->remote || !hud_scores->local_name)
    {
        if (hud_score_text)
            hud_score_text->error = BH_HUD_SCORES_ARGUMENT;
        return 0;
    }
    hud_score_text->error = BH_HUD_SCORES_OK;
    hud_score_text->attempted = hud_score_text->queued = hud_score_text->dropped = 0;
    for (i = 0; i < 8; ++i)
        if (hud_scores->remote[i])
        {
            if (!cleared)
                text_result(hud_score_text, bh_text_queue_clear(hud_score_text->queue, hud_score_text->page, 0));
            cleared = 1;
            row(hud_score_text, hud_scores->remote[i]->remote.name, score(hud_scores->remote[i]), row_y((long)i));
        }
    if (!cleared)
        text_result(hud_score_text, bh_text_queue_clear(hud_score_text->queue, hud_score_text->page, 0));
    row(hud_score_text, hud_scores->local_name, hud_scores->local_score, row_y(hud_scores->local_index));
    return hud_score_text->error == BH_HUD_SCORES_OK;
}
static void draw_row(BHHudScoreDraw *hud_score_draw, long index, long ordinal, int local, float unit)
{
    float x, y, width, v, depth;
    int result;
    bh_round_float(&x, (double)(hud_score_draw->display_width / 2));
    bh_round_float(&y, ((double)ordinal - (-.5)) * (double)unit);
    bh_round_float(&width, (double)unit * 8.0);
    bh_round_float(&v, (double)index * .125);
    bh_round_float(&depth, (double)hud_score_draw->near_z - (-.01));
    result = bh_pickup_rect_queue(hud_score_draw->queue, 3, hud_score_draw->page_image, x, y, width, unit, 0, 0, local ? 1.0f : .5f, 0, 1, v,
                                  .125f, depth);
    ++hud_score_draw->attempted;
    if (result > 0)
        ++hud_score_draw->queued;
    else if (!result)
        ++hud_score_draw->dropped;
    else
        hud_score_draw->error = BH_HUD_SCORES_QUEUE;
}
int bh_hud_scores_draw(BHHudScoreDraw *hud_score_draw, const BHHudScores *hud_scores)
{
    long upper = 100000, best, index, ordinal = 0;
    unsigned long i;
    float unit;
    if (!hud_score_draw || !hud_scores || !hud_score_draw->queue || !hud_scores->remote || !_finite(hud_score_draw->near_z))
    {
        if (hud_score_draw)
            hud_score_draw->error = BH_HUD_SCORES_ARGUMENT;
        return 0;
    }
    hud_score_draw->error = BH_HUD_SCORES_OK;
    hud_score_draw->attempted = hud_score_draw->queued = hud_score_draw->dropped = 0;
    bh_round_float(&unit, (double)(hud_score_draw->display_width / 40));
    for (;;)
    {
        best = -100000;
        index = -1;
        for (i = 0; i < 8; ++i)
            if ((long)i != hud_scores->local_index && hud_scores->remote[i])
            {
                long value = score(hud_scores->remote[i]);
                if (best < value && value < upper)
                {
                    best = value;
                    index = (long)i;
                }
            }
        if (best < hud_scores->local_score && hud_scores->local_score < upper)
        {
            index = hud_scores->local_index;
            best = hud_scores->local_score;
        }
        if (index < 0)
            return hud_score_draw->error == BH_HUD_SCORES_OK;
        for (i = 0; i < 8; ++i)
            if ((long)i != hud_scores->local_index && hud_scores->remote[i] && score(hud_scores->remote[i]) == best)
                draw_row(hud_score_draw, (long)i, ordinal++, 0, unit);
        if (hud_scores->local_score == best)
            draw_row(hud_score_draw, hud_scores->local_index, ordinal++, 1, unit);
        upper = best;
    }
}
