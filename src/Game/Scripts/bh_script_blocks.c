#include "bh_script_blocks.h"
#include "../../Graphics/Textures/bh_texture_controls.h"
#include "../../Assets/bh_vfile.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_rotation.h"
#include "../../Camera/bh_camera_queue.h"
#include "../../Graphics/Textures/bh_gdi_text.h"
#include "../../Graphics/Geometry/bh_screen_quad.h"
#include "../../Graphics/bh_draw_submit.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
int bh_script_blocks_init(BHScriptBlocks *script_blocks, BHScriptParse *parser, BHRenderResources *resources, const char *prefix,
                          BHBspSearch search)
{
    if (!script_blocks || !parser || !resources || !prefix)
        return 0;
    memset(script_blocks, 0, sizeof(*script_blocks));
    script_blocks->blocks = (BHScriptBlock *)calloc(600, sizeof(BHScriptBlock));
    if (!script_blocks->blocks)
        return 0;
    script_blocks->capacity = 600;
    script_blocks->parser = parser;
    script_blocks->resources = resources;
    script_blocks->collision_prefix = prefix;
    script_blocks->collision_search = search;
    return 1;
}
void bh_script_blocks_dispose(BHScriptBlocks *script_blocks)
{
    unsigned long i;
    if (!script_blocks)
        return;
    for (i = 0; i < script_blocks->capacity; ++i)
        if (script_blocks->blocks[i].collision.bsp.allocator.release)
            bh_collision_model_dispose(&script_blocks->blocks[i].collision);
    free(script_blocks->blocks);
    memset(script_blocks, 0, sizeof(*script_blocks));
}
int bh_script_blocks_load(BHScriptBlocks *script_blocks, const char *path)
{
    BHVFile *file;
    char line[513], *newline;
    BHScriptBlock *script_block;
    unsigned long length;
    BHTextureContext *textures;
    if (!script_blocks || !script_blocks->blocks || !script_blocks->capacity || script_blocks->capacity > 600 || !script_blocks->parser || !script_blocks->parser->vm || !script_blocks->resources ||
        !script_blocks->resources->models || !path)
    {
        if (script_blocks)
            script_blocks->error = 1;
        return 0;
    }
    script_blocks->error = 0;
    if (script_blocks->count)
        return 1;
    file = bh_vfile_open(path, "r");
    if (!file)
        return 0;
    script_blocks->parser->vm->script_loading = 1;
    while (script_blocks->count < script_blocks->capacity && bh_vfile_gets(line, 512, file))
    {
        newline = strchr(line, '\n');
        if (newline)
            *newline = 0;
        if (!_stricmp(line, "end"))
            break;
        if (line[0] == ';')
            continue;
        length = (unsigned long)strlen(line);
        if (length >= 16)
        {
            script_blocks->error = 1;
            break;
        }
        script_block = script_blocks->blocks + script_blocks->count;
        if (script_block->collision.bsp.allocator.release)
            bh_collision_model_dispose(&script_block->collision);
        memset(script_block, 0, sizeof(*script_block));
        memcpy(script_block->definition.name, line, length + 1);
        script_block->definition.grid = 10;
        if (!bh_script_parse_file(script_blocks->parser, &script_block->definition))
        {
            script_blocks->error = 2;
            break;
        }
        script_block->resource = bh_render_resources_load_model(script_blocks->resources, line);
        if (!bh_collision_model_init(&script_block->collision, 0))
        {
            script_blocks->error = 2;
            break;
        }
        script_block->model.minimum.x = script_block->model.minimum.y = script_block->model.minimum.z = 9e37f;
        script_block->model.maximum.x = script_block->model.maximum.y = script_block->model.maximum.z = -9e37f;
        if (bh_collision_model_load(&script_block->collision, line, script_blocks->collision_prefix, script_blocks->collision_search))
        {
            if (!bh_geometry_model_init(&script_block->model, &script_block->collision.geometry, script_block->definition.grid, script_block->definition.flags))
            {
                script_blocks->error = 2;
                break;
            }
            script_block->definition.flags = script_block->model.flags;
        }
        if (script_block->resource)
            ++script_blocks->count;
    }
    bh_vfile_close(file);
    script_blocks->parser->vm->script_loading = 0;
    if (script_blocks->error)
        return 0;
    script_blocks->columns = (long)(__int64)(sqrt((double)script_blocks->count) * 1.15);
    if (script_blocks->columns < 5)
        script_blocks->columns = 5;
    script_blocks->rows = ((long)script_blocks->count + script_blocks->columns - 1) / script_blocks->columns;
    textures = script_blocks->resources->models->textures;
    script_blocks->water = bh_texture_image_load(textures, "water.gif");
    if (script_blocks->water)
        bh_texture_image_set_flags(script_blocks->water, 2);
    return 1;
}
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
int bh_script_block_preview(BHScriptBrowser *script_browser, long index, long x, long y)
{
    BHScriptBlock *script_block;
    BHVec3 angles, center, transformed, position;
    BHMat34 matrix;
    char label[32];
    unsigned long i, length;
    float sx, sy, radius, xf, yf;
    double dx, dy, dz, screen_x, screen_y, retained_radius;
    if (!script_browser || !script_browser->library || !script_browser->queue)
    {
        if (script_browser)
            script_browser->error = 1;
        return 0;
    }
    if (index < 0 || (unsigned long)index >= script_browser->library->count)
        return 1;
    script_block = script_browser->library->blocks + index;
    sx = stored((double)x + script_browser->offset_x);
    sy = stored((double)y + script_browser->offset_y);
    screen_x = (double)sx * script_browser->height * (double).2f;
    screen_y = (double)sy * script_browser->height * (double).2f;
    if (!_finite(screen_x) || !_finite(screen_y) || fabs(screen_x) >= 2147483648.0 || fabs(screen_y) >= 2147483648.0)
    {
        script_browser->error = 1;
        return 0;
    }
    strcpy(label, script_block->definition.name);
    length = (unsigned long)strlen(label);
    if (length > 4 && label[length - 4] == '.')
        label[length - 4] = 0;
    if (bh_gdi_text_queue(script_browser->queue, (short)(__int64)screen_x, (short)(__int64)screen_y, label,
                          script_block->definition.script ? 0x806040 : 0x406080) != 1)
    {
        script_browser->error = 2;
        return 0;
    }
    dx = (double)script_block->model.maximum.x - script_block->model.minimum.x;
    dy = (double)script_block->model.maximum.y - script_block->model.minimum.y;
    dz = (double)script_block->model.maximum.z - script_block->model.minimum.z;
    retained_radius = sqrt((dx * stored(dx) + dy * stored(dy)) + dz * stored(dz)) * 5;
    radius = stored(retained_radius);
    if (!_finite(radius) || !_finite(script_block->preview_yaw) || !_finite(script_block->preview_phase))
    {
        script_browser->error = 1;
        return 0;
    }
    angles.x = script_block->preview_yaw;
    angles.y = stored(sin((double)script_block->preview_phase) * .5);
    angles.z = 0;
    for (i = 0; i < 3; ++i)
        ((float *)&center)[i] =
            stored(((double)((float *)&script_block->model.minimum)[i] + ((float *)&script_block->model.maximum)[i]) * .5);
    memset(&matrix, 0, sizeof(matrix));
    matrix.m[0][0] = matrix.m[1][1] = matrix.m[2][2] = 1;
    bh_mat34_rotate_y(angles.x, &matrix);
    bh_mat34_rotate_x(angles.y, &matrix);
    bh_mat34_transform_point(&matrix, &center, &transformed);
    xf = stored((((double)sx - (-.5)) - 2.5) * (2.0 / 9.0));
    yf = stored((((double)sy - (-.5)) - 1.875) * (4.0 / 17.0));
    position.x = stored(retained_radius * xf);
    position.y = stored((double)radius * yf);
    position.z = radius;
    for (i = 0; i < 3; ++i)
        ((float *)&position)[i] = stored((double)((float *)&position)[i] - ((float *)&transformed)[i]);
    if (bh_draw_submit_resource(script_browser->queue, 0, script_block->resource, &position, &angles, 1) != 1)
    {
        script_browser->error = 2;
        return 0;
    }
    return 1;
}
static long signed_word(unsigned long x)
{
    long n;
    memcpy(&n, &x, 4);
    return n;
}
static int quad(BHScriptBrowser *script_browser, unsigned long bucket, unsigned long image, float x, float y, float width,
                float height, float alpha)
{
    if (bh_screen_quad_queue(script_browser->queue, bucket, image, x, y, width, height, 0, 0, alpha) != 1)
    {
        script_browser->error = 2;
        return 0;
    }
    return 1;
}
int bh_script_block_browser(BHScriptBrowser *script_browser)
{
    BHScriptBlocks *library;
    BHScriptBlock *script_block;
    BHVec3 zero = {0, 0, 0};
    unsigned long r = 2401, q, i;
    long columns, rows, step, x, y, index, first_x, last_x, first_y, last_y, j;
    double value, change;
    float px, py;
    if (!script_browser || !script_browser->library || !script_browser->queue || !script_browser->sound || script_browser->height < 1 || script_browser->height > 16384 || script_browser->library->count > 600 ||
        script_browser->library->columns < 5 || script_browser->library->columns > 600 || script_browser->library->rows < 0 || script_browser->library->rows > 600 ||
        !_finite(script_browser->cursor_x) || !_finite(script_browser->cursor_y))
    {
        if (script_browser)
            script_browser->error = 1;
        return 0;
    }
    script_browser->error = 0;
    library = script_browser->library;
    columns = library->columns;
    rows = library->rows;
    step = script_browser->height / 5;
    if (bh_camera_queue_pose(script_browser->queue, &zero, &zero) != 1)
    {
        script_browser->error = 2;
        return 0;
    }
    for (i = 0; i < library->count; ++i)
    {
        script_block = library->blocks + i;
        q = (unsigned long)(signed_word(r * 7UL) / 64);
        change = .05 - (q & 15) * (-1.0 / 300.0);
        if (q & 16)
            change = -change;
        value = (double)script_block->preview_yaw + change;
        script_block->preview_yaw = stored(value);
        if (value < -3.141592653589793)
            script_block->preview_yaw = stored(value - (-6.283185307179586));
        else if (value > 3.141592653589793)
            script_block->preview_yaw = stored(value - 6.283185307179586);
        r *= 49UL;
        q = (unsigned long)(signed_word(r) / 64);
        script_block->preview_phase = stored(((double)script_block->preview_phase - (q & 15) * (-1.0 / 1500.0)) - (-.01));
        value = script_block->preview_phase;
        if (value < -3.141592653589793)
            script_block->preview_phase = stored(value - (-6.283185307179586));
        else if (value > 3.141592653589793)
            script_block->preview_phase = stored(value - 6.283185307179586);
    }
    if (script_browser->cursor_x < 0)
        script_browser->cursor_x = 0;
    else if (script_browser->cursor_x > columns)
        script_browser->cursor_x = (float)columns;
    if (script_browser->cursor_y < 0)
        script_browser->cursor_y = 0;
    else if (script_browser->cursor_y > rows)
        script_browser->cursor_y = (float)rows;
    x = (long)(__int64)script_browser->cursor_x;
    if (x >= columns)
        x = columns - 1;
    y = (long)(__int64)script_browser->cursor_y;
    if (y >= rows)
        y = rows - 1;
    index = y * columns + x;
    if (index >= 0 && (unsigned long)index < library->count && index != script_browser->selected)
    {
        script_browser->selected = index;
        bh_sound_request(script_browser->sound, script_browser->select_sound, 1, 1, 0, 0);
    }
    script_browser->offset_x = stored(-((double)script_browser->cursor_x - 2.5));
    script_browser->offset_y = stored(-((double)script_browser->cursor_y - 1.875));
    if (script_browser->offset_x > 0)
        script_browser->offset_x = 0;
    if (script_browser->offset_y > 0)
        script_browser->offset_y = 0;
    if (script_browser->offset_x < 5 - columns)
        script_browser->offset_x = (float)(5 - columns);
    if (script_browser->offset_y < -((double)rows - 3.75))
        script_browser->offset_y = stored(-((double)rows - 3.75));
    for (x = 0; x <= columns; ++x)
        if (!quad(script_browser, 0, script_browser->grid_image, stored(((double)x + script_browser->offset_x) * step),
                  stored(((double)script_browser->offset_y - rows * (-.5)) * step), 1, (float)(step * rows), .5f))
            return 0;
    for (y = 0; y <= rows; ++y)
        if (!quad(script_browser, 0, script_browser->grid_image, stored(((double)script_browser->offset_x - columns * (-.5)) * step),
                  stored(((double)y + script_browser->offset_y) * step), (float)(step * columns), 1, .5f))
            return 0;
    px = (float)(long)(__int64)(((double)script_browser->offset_x + script_browser->cursor_x) * step);
    py = (float)(long)(__int64)(((double)script_browser->offset_y + script_browser->cursor_y) * step);
    if (!quad(script_browser, 3, script_browser->selection_image, px, py, 2, (float)(step / 4), 1) ||
        !quad(script_browser, 3, script_browser->selection_image, px, py, (float)(step / 4), 2, 1))
        return 0;
    x = script_browser->selected % columns;
    y = script_browser->selected / columns;
    px = (float)(long)(__int64)(((double)x + script_browser->offset_x - (-.5)) * step);
    py = (float)(long)(__int64)(((double)y + script_browser->offset_y - (-.5)) * step);
    if (!quad(script_browser, 3, script_browser->selection_image, px, py, (float)step, (float)step, .25f))
        return 0;
    x = script_browser->active % columns;
    y = script_browser->active / columns;
    px = (float)(long)(__int64)(((double)x + script_browser->offset_x - (-.5)) * step);
    py = (float)(long)(__int64)(((double)y + script_browser->offset_y - (-.5)) * step);
    if (!quad(script_browser, 3, script_browser->grid_image, px, py, (float)step, (float)step, .25f))
        return 0;
    first_x = (long)(__int64)(-(double)script_browser->offset_x);
    last_x = (long)(__int64)(6.0 - script_browser->offset_x);
    if (last_x > columns)
        last_x = columns;
    first_y = (long)(__int64)(-(double)script_browser->offset_y);
    last_y = (long)(__int64)(4.75 - script_browser->offset_y);
    if (last_y > rows)
        last_y = rows;
    for (j = first_y; j < last_y; ++j)
        for (x = first_x; x < last_x; ++x)
            if (!bh_script_block_preview(script_browser, j * columns + x, x, j))
                return 0;
    return 1;
}
