#include "bh_game_bootstrap.h"
#include "../Config/bh_game_settings.h"
#include "../Graphics/Textures/bh_atlas_text.h"
#include "../Graphics/Textures/bh_atlas_glyph.h"
#include "../Graphics/Textures/bh_texture_controls.h"
#include "../Graphics/Textures/bh_texture_policy.h"
#include "../Physics/bh_collision_resource.h"
#include "../Graphics/Models/bh_model_images.h"
#include "../Graphics/Models/bh_model_scale.h"
#include "../Assets/bh_vfile.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
typedef struct BHBootstrapResource
{
    unsigned long kind, identity;
    const char *name;
    float range, level;
    unsigned long streaming;
    double argument;
} BHBootstrapResource;
typedef struct BHBootstrapMenuSpec
{
    unsigned long identity, tag;
} BHBootstrapMenuSpec;
typedef struct BHBootstrapRowSpec
{
    unsigned long menu, button;
    const char *title, *body;
    unsigned long destination, action, refresh;
} BHBootstrapRowSpec;
#include "bh_bootstrap_data.h"
#define N(a) (sizeof(a) / sizeof((a)[0]))
static_assert(sizeof(BHMenuMissionRow) == sizeof(BHMenuMissionAction), "mission action alias");
static int fail(BHGameBootstrap *game_bootstrap, int error)
{
    game_bootstrap->error = error;
    return 0;
}
unsigned long bh_bootstrap_value(const BHGameBootstrap *game_bootstrap, unsigned long identity)
{
    unsigned long i;
    if (game_bootstrap)
        for (i = 0; i < game_bootstrap->value_count; ++i)
            if (game_bootstrap->values[i].identity == identity)
                return game_bootstrap->values[i].value;
    return 0;
}
static int store(BHGameBootstrap *game_bootstrap, unsigned long identity, unsigned long value, unsigned long kind)
{
    unsigned long i;
    for (i = 0; i < game_bootstrap->value_count; ++i)
        if (game_bootstrap->values[i].identity == identity)
            break;
    if (i == N(game_bootstrap->values))
        return fail(game_bootstrap, 2);
    if (i == game_bootstrap->value_count)
        ++game_bootstrap->value_count;
    game_bootstrap->values[i].identity = identity;
    game_bootstrap->values[i].value = value;
    game_bootstrap->values[i].kind = kind;
    for (i = 0; i < game_bootstrap->binding_count; ++i)
        if (game_bootstrap->bindings[i].identity == identity)
        {
            if (!game_bootstrap->bindings[i].destination)
                return fail(game_bootstrap, 1);
            *game_bootstrap->bindings[i].destination = value;
        }
    return 1;
}
static int scale_captain(BHGameBootstrap *);
static int resources(BHGameBootstrap *game_bootstrap, const BHBootstrapResource *ops, unsigned long count)
{
    unsigned long i, v;
    const BHBootstrapResource *bootstrap_resource;
    for (i = 0; i < count; ++i)
    {
        bootstrap_resource = ops + i;
        v = 0;
        switch (bootstrap_resource->kind)
        {
        case 1:
            if (!game_bootstrap->textures)
                return fail(game_bootstrap, 1);
            v = (unsigned long)bh_texture_image_load(game_bootstrap->textures, bootstrap_resource->name);
            if (!v)
                return fail(game_bootstrap, 3);
            break;
        case 2:
            if (!game_bootstrap->sounds)
                return fail(game_bootstrap, 1);
            v = bh_sound_load(game_bootstrap->sounds, bootstrap_resource->name, bootstrap_resource->range, bootstrap_resource->level, (unsigned short)bootstrap_resource->streaming, (float)bootstrap_resource->argument);
            if (v == BH_SOUND_INVALID)
                return fail(game_bootstrap, 4);
            break;
        case 3:
            if (!game_bootstrap->resources)
                return fail(game_bootstrap, 1);
            v = bh_render_resources_load_model(game_bootstrap->resources, bootstrap_resource->name);
            if (!v)
                return fail(game_bootstrap, 5);
            break;
        case 4:
            v = bh_bootstrap_value(game_bootstrap, (unsigned long)bootstrap_resource->argument);
            ++game_bootstrap->aliases;
            break;
        case 5:
            v = 0;
            break;
        case 6:
            if (!game_bootstrap->sounds || !game_bootstrap->sounds->queue)
                return fail(game_bootstrap, 1);
            bh_sound_set_voice_limit(game_bootstrap->sounds->queue, bootstrap_resource->identity);
            continue;
        case 9:
            if (!scale_captain(game_bootstrap))
                return 0;
            continue;
        case 7:
            if (!bh_texture_policy_set(game_bootstrap->textures, bootstrap_resource->identity, (unsigned long)bootstrap_resource->argument))
                return fail(game_bootstrap, 3);
            continue;
        default:
            return fail(game_bootstrap, 1);
        }
        if (!store(game_bootstrap, bootstrap_resource->identity, v, bootstrap_resource->kind))
            return 0;
        ++game_bootstrap->resources_loaded;
    }
    return 1;
}
int bh_bootstrap_resources(BHGameBootstrap *c, unsigned long stage)
{
    if (!c)
        return 0;
    switch (stage)
    {
    case 0:
        return resources(c, prelude, N(prelude));
    case 1:
        return resources(c, models, N(models));
    case 2:
        return resources(c, menu_resources, N(menu_resources));
    case 3:
        return resources(c, tail, N(tail));
    case 4:
        return resources(c, tv, N(tv));
    case 5:
        return resources(c, engine_sounds, N(engine_sounds));
    case 6:
        return resources(c, engine_images, N(engine_images));
    default:
        return fail(c, 1);
    }
}
static BHBootstrapMenu *menu(BHGameBootstrap *game_bootstrap, unsigned long id)
{
    unsigned long i;
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
        if (game_bootstrap->menus[i].identity == id)
            return game_bootstrap->menus + i;
    return 0;
}
static BHBootstrapMenu *action_menu(BHMenuActions *a)
{
    return (BHBootstrapMenu *)((char *)a - offsetof(BHBootstrapMenu, actions));
}
static void submenu_first(BHMenuActions *menu_actions, long index)
{
    (void)index;
    bh_menu_submenu_first(menu_actions->settings);
}
static void network_ship_picker(BHMenuActions *menu_actions, long index)
{
    BHMenuRuntime *picker = action_menu(menu_actions)->owner->runtime->network_ship_menu;
    (void)index;
    if (menu_actions->network_ship)
    {
        picker->actions->menu->selection = *menu_actions->network_ship;
        picker->actions->menu->display_selection = (float)*menu_actions->network_ship;
    }
}
static void network_ship_select(BHMenuActions *menu_actions, long index)
{
    if (menu_actions->network_ship)
        *menu_actions->network_ship = index;
}
static void save(BHMenuActions *menu_actions, long index)
{
    (void)index;
    if (!bh_config_save(menu_actions->config, menu_actions->save_path))
        menu_actions->error = 1;
}
static void quit(BHMenuActions *menu_actions, long index)
{
    BHBootstrapMenu *bootstrap_menu = action_menu(menu_actions);
    (void)index;
    if (!bootstrap_menu->owner->transition || !bh_menu_quit_begin(bootstrap_menu->owner->transition))
        menu_actions->error = 1;
}
static BHMenuAction action(unsigned long id)
{
    switch (id)
    {
    case 0:
        return 0;
    case 0x42cae0:
        return submenu_first;
    case 0x42cb10:
        return network_ship_picker;
    case 0x42caa0:
        return network_ship_select;
    case 0x42f350:
        return save;
    case 0x42d070:
        return quit;
    case 0x42d0d0:
        return bh_menu_action_video;
    case 0x42db60:
        return bh_menu_action_audio;
    case 0x42df90:
        return bh_menu_action_profile;
    case 0x42e150:
        return bh_menu_action_profile_promote;
    case 0x42f2b0:
        return bh_menu_action_graphics_notice;
    case 0x42f360:
        return bh_menu_action_network_host;
    case 0x42f390:
        return bh_menu_action_network_join;
    default:
        return 0;
    }
}
static BHMenuMissionAction edit_action(unsigned long id)
{
    switch (id)
    {
    case 0x42ca10:
        return bh_menu_action_mission;
    case 0x42ca30:
        return bh_menu_action_ship;
    case 0x42e410:
        return bh_menu_action_motion;
    case 0x42e640:
        return bh_menu_action_primary;
    case 0x42e790:
        return bh_menu_action_secondary;
    case 0x42e8f0:
        return bh_menu_action_camera;
    default:
        return 0;
    }
}
static void row_refresh(BHMenuSettingsMenu *menu_settings_menu, long row)
{
    BHBootstrapMenu *bootstrap_menu = (BHBootstrapMenu *)menu_settings_menu->user;
    BHMenuMissionAction fn = 0;
    if (!bootstrap_menu || row < 0 || (unsigned long)row >= menu_settings_menu->row_count)
        return;
    bootstrap_menu->edits.menu = menu_settings_menu;
    bootstrap_menu->edits.rows = &bootstrap_menu->object.rows;
    switch (bootstrap_menu->refresh[row])
    {
    case 0x42d920:
        bh_menu_network_address(&bootstrap_menu->actions, row);
        return;
    case 0x42d1b0:
        fn = bh_menu_edit_video;
        break;
    case 0x42d290:
        fn = bh_menu_edit_audio;
        break;
    case 0x42e300:
        fn = bh_menu_edit_profile;
        break;
    case 0x42d380:
        fn = bh_menu_edit_motion;
        break;
    case 0x42d550:
        fn = bh_menu_edit_primary;
        break;
    case 0x42d620:
        fn = bh_menu_edit_secondary;
        break;
    case 0x42d6f0:
        fn = bh_menu_edit_camera;
        break;
    }
    if (fn)
        fn(&bootstrap_menu->edits, row);
}
static void initial_refresh(void *s, long index)
{
    row_refresh((BHMenuSettingsMenu *)s, index);
}
static int add_row(BHGameBootstrap *game_bootstrap, const BHBootstrapRowSpec *bootstrap_row_spec)
{
    BHBootstrapMenu *bootstrap_menu = menu(game_bootstrap, bootstrap_row_spec->menu), *destination;
    unsigned long row;
    unsigned char body[512], *p = 0;
    BHMenuRow *made;
    if (!bootstrap_menu)
        return fail(game_bootstrap, 1);
    row = bootstrap_menu->object.rows.count;
    if (row >= BH_BOOTSTRAP_ROWS)
        return fail(game_bootstrap, 2);
    if (bootstrap_row_spec->body)
    {
        const char *text = bootstrap_row_spec->body;
        if (strlen(text) >= sizeof(body))
            return fail(game_bootstrap, 2);
        strcpy((char *)body, text);
        p = body;
    }
    destination = bootstrap_row_spec->destination ? menu(game_bootstrap, bootstrap_row_spec->destination) : 0;
    if (bootstrap_row_spec->destination && !destination)
        return fail(game_bootstrap, 1);
    bootstrap_menu->action[row] = action(bootstrap_row_spec->action);
    bootstrap_menu->edit_action[row] = edit_action(bootstrap_row_spec->action);
    if (bootstrap_row_spec->action && !bootstrap_menu->action[row] && !bootstrap_menu->edit_action[row])
        return fail(game_bootstrap, 6);
    made = bh_menu_object_add_row(&bootstrap_menu->object, bh_bootstrap_value(game_bootstrap, bootstrap_row_spec->button), (const unsigned char *)bootstrap_row_spec->title, p,
                                  bootstrap_row_spec->destination, bootstrap_row_spec->action, bootstrap_row_spec->refresh ? initial_refresh : 0);
    if (!made)
        return fail(game_bootstrap, 7);
    bootstrap_menu->destinations[row] = destination ? &destination->runtime : 0;
    bootstrap_menu->refresh[row] = bootstrap_row_spec->refresh;
    bootstrap_menu->settings_rows[row].refresh = bootstrap_row_spec->refresh ? row_refresh : 0;
    bootstrap_menu->row_pointers[row] = &bootstrap_menu->settings_rows[row];
    bootstrap_menu->settings.row_count = bootstrap_menu->object.rows.count;
    bootstrap_menu->present_rows[row].row = made;
    bootstrap_menu->present_rows[row].image_handle = (unsigned long)made->image;
    bootstrap_menu->present.row_count = bootstrap_menu->object.rows.count;
    ++game_bootstrap->rows_created;
    return 1;
}
int bh_bootstrap_menus(BHGameBootstrap *game_bootstrap)
{
    unsigned long i, j;
    BHBootstrapMenu *bootstrap_menu;
    if (!game_bootstrap || !game_bootstrap->pool || !game_bootstrap->inputs || !game_bootstrap->atlas || !game_bootstrap->text || !game_bootstrap->runtime || !game_bootstrap->actions || !game_bootstrap->edits)
        return game_bootstrap ? fail(game_bootstrap, 1) : 0;
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
    {
        bootstrap_menu = game_bootstrap->menus + i;
        if (bootstrap_menu->object.entity)
            return fail(game_bootstrap, 1);
        bootstrap_menu->identity = menu_specs[i].identity;
        bootstrap_menu->owner = game_bootstrap;
        bootstrap_menu->actions = *game_bootstrap->actions;
        bootstrap_menu->edits = *game_bootstrap->edits;
        if (!bh_menu_object_create(&bootstrap_menu->object, game_bootstrap->pool, game_bootstrap->inputs, menu_specs[i].tag, "") ||
            !bh_menu_object_rows_init(&bootstrap_menu->object, game_bootstrap->atlas, game_bootstrap->text, bootstrap_menu->rows, BH_BOOTSTRAP_ROWS))
            return fail(game_bootstrap, 7);
        bootstrap_menu->root.menu = &bootstrap_menu->settings;
        bootstrap_menu->root.row_actions = bootstrap_menu->action;
        bootstrap_menu->root.action_count = BH_BOOTSTRAP_ROWS;
        bootstrap_menu->settings.rows = bootstrap_menu->row_pointers;
        bootstrap_menu->settings.user = bootstrap_menu;
        bootstrap_menu->actions.menu = &bootstrap_menu->settings;
        bootstrap_menu->actions.rows = &bootstrap_menu->object.rows;
        bootstrap_menu->edits.menu = &bootstrap_menu->settings;
        bootstrap_menu->edits.rows = &bootstrap_menu->object.rows;
        if (game_bootstrap->present)
            bootstrap_menu->present = *game_bootstrap->present;
        bootstrap_menu->present.rows = bootstrap_menu->present_rows;
        bootstrap_menu->runtime.context = game_bootstrap->runtime;
        bootstrap_menu->runtime.entity = bootstrap_menu->object.entity;
        bootstrap_menu->runtime.actions = &bootstrap_menu->root;
        bootstrap_menu->runtime.action_context = &bootstrap_menu->actions;
        bootstrap_menu->runtime.edit_context = &bootstrap_menu->edits;
        bootstrap_menu->runtime.edit_actions = bootstrap_menu->edit_action;
        bootstrap_menu->runtime.destinations = bootstrap_menu->destinations;
        bootstrap_menu->runtime.input = bootstrap_menu->object.input;
        bootstrap_menu->runtime.present = &bootstrap_menu->present;
        bootstrap_menu->runtime.pending_selection = -1;
        bootstrap_menu->runtime.back_selection = menu_backs[i];
        bootstrap_menu->runtime.slide_voice = BH_SOUND_INVALID;
        bootstrap_menu->runtime.waypoint = (long)menu_specs[i].tag;
        ++game_bootstrap->menus_created;
    }
    for (i = 0; i < N(row_specs); ++i)
        if (!add_row(game_bootstrap, row_specs + i))
            return 0;

    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
    {
        for (j = 0; j < 9; ++j)
            game_bootstrap->menus[i].edits.mission_rows[j] = (BHMenuMissionRow *)(menu(game_bootstrap, 0x4ce450)->edit_action + j);
        game_bootstrap->menus[i].edits.mission_menu = &menu(game_bootstrap, 0x4ce450)->settings;
    }
    if (game_bootstrap->actions->settings)
        game_bootstrap->actions->settings->ship_menu = &menu(game_bootstrap, 0x4cdc9c)->settings;
    game_bootstrap->runtime->root = &menu(game_bootstrap, 0x495e40)->runtime;
    game_bootstrap->runtime->network_menu = &menu(game_bootstrap, 0x4d1488)->runtime;
    game_bootstrap->runtime->network_ship_menu = &menu(game_bootstrap, 0x4cdca0)->runtime;
    game_bootstrap->runtime->ship_menu = &menu(game_bootstrap, 0x4cdc9c)->runtime;
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
    {
        bootstrap_menu = game_bootstrap->menus + i;
        if (!bh_menu_runtime_bind(&bootstrap_menu->runtime))
            return fail(game_bootstrap, 8);
        bootstrap_menu->object.entity->step = 0;
        bootstrap_menu->object.entity->refresh = 0;
    }
    bootstrap_menu = menu(game_bootstrap, 0x495e40);
    bootstrap_menu->runtime.entity->step = bh_menu_runtime_step_entity;
    bootstrap_menu->runtime.entity->refresh = bh_menu_runtime_refresh_entity;
    bootstrap_menu->runtime.distance = 100;
    bootstrap_menu->runtime.exiting = 0;
    if (bootstrap_menu->runtime.waypoint >= 0)
        *game_bootstrap->runtime->waypoint = bootstrap_menu->runtime.waypoint;
    if (!bh_input_set_enabled(bootstrap_menu->runtime.input, game_bootstrap->snapshot, 1))
        return fail(game_bootstrap, 8);
    for (i = 0; i < bootstrap_menu->settings.row_count; ++i)
        if (bootstrap_menu->settings_rows[i].refresh)
            bootstrap_menu->settings_rows[i].refresh(&bootstrap_menu->settings, (long)i);
    for (i = 0; i < bootstrap_menu->settings.row_count; ++i)
        if (bootstrap_menu->destinations[bootstrap_menu->settings.selection] || bootstrap_menu->action[bootstrap_menu->settings.selection] ||
            bootstrap_menu->edit_action[bootstrap_menu->settings.selection])
            break;
        else
            bootstrap_menu->settings.selection = (bootstrap_menu->settings.selection + 1) % bootstrap_menu->settings.row_count;
    if (i == bootstrap_menu->settings.row_count)
        return fail(game_bootstrap, 8);
    return 1;
}
static int font(BHGameBootstrap *game_bootstrap, BHAtlasPage **page, unsigned long identity, unsigned long size)
{
    *page = bh_atlas_page_create(game_bootstrap->atlas, "textures\\tvfont.pcx", size);
    return *page && store(game_bootstrap, identity, (unsigned long)bh_atlas_page_image(*page), 1);
}
static int scale_captain(BHGameBootstrap *game_bootstrap)
{
    const BHDrawResource *draw_resource = bh_render_resources_lookup(bh_bootstrap_value(game_bootstrap, 0x4d047c), game_bootstrap->resources);
    BHModelDrawCache *cache;
    if (!draw_resource || draw_resource->tag != BH_RESOURCE_TAG_MODEL)
        return fail(game_bootstrap, 5);
    cache = draw_resource->data.model.instance->cache;
    return bh_model_scale(draw_resource->data.model.instance->model, 3, 3, 3, &cache, 1) ? 1 : fail(game_bootstrap, 5);
}
static void fade_refresh(BHEntity *e, void *packet)
{
    (void)packet;
    bh_menu_music_fade(e);
}
int bh_game_bootstrap(BHGameBootstrap *game_bootstrap)
{
    BHVFile *file;
    BHBootstrapResource r;
    BHVec3 start = {15, -14.699999809265137f, -40};
    BHWorldGeometry *first;
    BHBootstrapMenu *m;
    const BHDrawResource *resource;
    BHModel *model;
    BHEntity *entity;
    unsigned long i;
    long width, index;
    static const char *confirm[3] = {"Abandon", "Game ?", "[Y/N]"};
    static const long ys[3] = {13, 26, 40};
    if (!game_bootstrap || !game_bootstrap->textures || !game_bootstrap->resources || !game_bootstrap->sounds || !game_bootstrap->atlas || !game_bootstrap->text || !game_bootstrap->blocks || !game_bootstrap->world ||
        !game_bootstrap->player || !game_bootstrap->pool || !game_bootstrap->inputs || !game_bootstrap->story || !game_bootstrap->availability || !game_bootstrap->panel || !game_bootstrap->transition)
        return game_bootstrap ? fail(game_bootstrap, 1) : 0;
    game_bootstrap->error = 0;
    if (!bh_bootstrap_resources(game_bootstrap, 0) || !bh_texture_set_path(game_bootstrap->textures, "textures", 0) ||
        !bh_collision_resource_path(&game_bootstrap->collision_path, "bsp", 0))
        return fail(game_bootstrap, game_bootstrap->error ? game_bootstrap->error : 3);
    game_bootstrap->blocks->collision_prefix = game_bootstrap->collision_path.prefix;
    game_bootstrap->blocks->collision_search = game_bootstrap->collision_path.search;
    game_bootstrap->player->prefix = game_bootstrap->collision_path.prefix;
    game_bootstrap->player->search = game_bootstrap->collision_path.search;
    if (bh_script_blocks_load(game_bootstrap->blocks, "bsp\\blocks.txt"))
    {
        if (!bh_world_load(game_bootstrap->world, game_bootstrap->custom_level ? "level.lev" : "test.lev") ||
            (game_bootstrap->custom_level && (!game_bootstrap->world->editor->world->count || game_bootstrap->world->skipped)))
            return fail(game_bootstrap, 9);
        first = (game_bootstrap->world->editor->world->count ? &game_bootstrap->world->editor->world->nodes[0]->placement.geometry : 0);
        if (first)
        {
            if (!game_bootstrap->editor || !game_bootstrap->navigation || !bh_editor_input_setup(game_bootstrap->editor))
                return fail(game_bootstrap, 1);
            file = game_bootstrap->custom_level ? bh_vfile_open_level("level.pth") : bh_vfile_open("test.pth", "rb");
            index = bh_navigation_build(game_bootstrap->navigation, first, game_bootstrap->world->editor->world->count, file);
            if (file)
                bh_vfile_close(file);
            if (!index)
                return fail(game_bootstrap, 9);
        }
    }
    else if (game_bootstrap->custom_level)
        return fail(game_bootstrap, 9);
    if (game_bootstrap->compose && !game_bootstrap->compose(game_bootstrap, 1, game_bootstrap->compose_user))
        return fail(game_bootstrap, 11);
    bh_vfile_preload("bsp\\hun_lod1.BSP", "rb");
    bh_vfile_preload("bsp\\hun_lod2.BSP", "rb");
    if (!bh_texture_set_path(game_bootstrap->textures, "textures", 0) || !bh_bootstrap_resources(game_bootstrap, 1))
        return 0;
    r.kind = 1;
    r.identity = 0x4d1aa0;
    r.name = "water.gif";
    r.range = r.level = 0;
    r.streaming = 0;
    r.argument = 0;
    if (!resources(game_bootstrap, &r, 1) || !font(game_bootstrap, &game_bootstrap->confirmation, 0x4962f4, 64))
        return fail(game_bootstrap, 7);
    if (!bh_atlas_page_clear(game_bootstrap->atlas, game_bootstrap->confirmation, 0))
        return fail(game_bootstrap, 7);
    for (i = 0; i < 3; ++i)
    {
        width = bh_atlas_page_text_width(game_bootstrap->confirmation, (const unsigned char *)confirm[i]);
        bh_atlas_page_cursor(game_bootstrap->confirmation, (long)(32 - (double)width * .5), ys[i]);
        bh_atlas_page_text(game_bootstrap->confirmation, (const unsigned char *)confirm[i]);
    }
    if (!bh_player_create(game_bootstrap->player, 0, &start, 1, bh_bootstrap_value(game_bootstrap, 0x4d1964)))
        return fail(game_bootstrap, 10);
    if (game_bootstrap->compose && !game_bootstrap->compose(game_bootstrap, 2, game_bootstrap->compose_user))
        return fail(game_bootstrap, 11);
    entity = bh_entity_acquire(game_bootstrap->pool);
    if (entity)
    {
        game_bootstrap->story->player = *game_bootstrap->player->current;
        if (!bh_mission_story_bind(game_bootstrap->story, entity))
            return fail(game_bootstrap, 10);
        memset(game_bootstrap->availability, 0, 4 * 32 * 4);
    }
    if (!font(game_bootstrap, &game_bootstrap->menu_font, 0x4cdb9c, 128) || !bh_bootstrap_resources(game_bootstrap, 2))
        return fail(game_bootstrap, game_bootstrap->error ? game_bootstrap->error : 7);
    game_bootstrap->graffiti = bh_atlas_page_create(game_bootstrap->atlas, "textures\\graffiti.pcx", 128);
    if (!game_bootstrap->graffiti || !store(game_bootstrap, 0x4d1910, (unsigned long)bh_atlas_page_image(game_bootstrap->graffiti), 1))
        return fail(game_bootstrap, 7);
    for (i = 0; i < game_bootstrap->blocks->count; ++i)
        if (!_stricmp(game_bootstrap->blocks->blocks[i].definition.name, "graffiti.bsp"))
        {
            resource = bh_render_resources_lookup(game_bootstrap->blocks->blocks[i].resource, game_bootstrap->resources);
            if (!resource || resource->tag != BH_RESOURCE_TAG_MODEL)
                return fail(game_bootstrap, 5);
            model = resource->data.model.instance->model;
            index = bh_model_image_find(model, "win.pcx");
            bh_model_image_replace(model, index, bh_atlas_page_image(game_bootstrap->graffiti));
        }
    bh_text_queue_clear(game_bootstrap->text, game_bootstrap->graffiti, 0);
    {
        const char *name = bh_game_settings_player_name();
        if (!*name)
        {
            name = getenv("COMPUTERNAME");
            if (!name || !*name)
                name = getenv("HOSTNAME");
            if (!name || !*name)
                name = "BHunter";
            strncpy(game_bootstrap->computer_name, name, sizeof(game_bootstrap->computer_name) - 1);
            game_bootstrap->computer_name[sizeof(game_bootstrap->computer_name) - 1] = 0;
            name = game_bootstrap->computer_name;
        }
        strncpy(game_bootstrap->player_name, name, sizeof(game_bootstrap->player_name) - 1);
        game_bootstrap->player_name[sizeof(game_bootstrap->player_name) - 1] = 0;
    }
    game_bootstrap->all_keys = bh_player_input_create(game_bootstrap->inputs, 0);
    if (!game_bootstrap->all_keys)
        return fail(game_bootstrap, 10);
    for (i = 0; i < 255; ++i)
        bh_player_input_bind_key(game_bootstrap->all_keys, (long)i, "", 0, 0, (unsigned short)i);
    if (game_bootstrap->compose && !game_bootstrap->compose(game_bootstrap, 5, game_bootstrap->compose_user))
        return fail(game_bootstrap, 11);
    if ((!game_bootstrap->flyby || !bh_flyby_construct(game_bootstrap->flyby)) || !bh_bootstrap_menus(game_bootstrap))
        return fail(game_bootstrap, game_bootstrap->error ? game_bootstrap->error : 10);
    if (game_bootstrap->compose && !game_bootstrap->compose(game_bootstrap, 3, game_bootstrap->compose_user))
        return fail(game_bootstrap, 11);
    if (!game_bootstrap->transition->cd_ready)
    {
        entity = bh_entity_acquire(game_bootstrap->pool);
        if (!entity || !bh_menu_transition_bind(game_bootstrap->transition, entity, 2, 0))
            return fail(game_bootstrap, 10);
        game_bootstrap->transition->music = entity;
        entity->step = bh_menu_music_tick;
    }
    r.identity = 0x4d4424;
    r.name = "win.bmp";
    if (!resources(game_bootstrap, &r, 1))
        return 0;
    entity = bh_entity_acquire(game_bootstrap->pool);
    if (entity)
    {
        if (!bh_menu_transition_bind(game_bootstrap->transition, entity, 2, 0))
            return fail(game_bootstrap, 10);
        entity->step = bh_menu_fade_expiry;
        entity->refresh = fade_refresh;
    }
    if (!bh_bootstrap_resources(game_bootstrap, 3) || !bh_texture_set_path(game_bootstrap->textures, "textures", 0) ||
        !bh_bootstrap_resources(game_bootstrap, 4))
        return fail(game_bootstrap, game_bootstrap->error ? game_bootstrap->error : 3);
    game_bootstrap->panel->page = bh_atlas_page_create(game_bootstrap->atlas, "textures\\tvfont.pcx", 128);
    if (!game_bootstrap->panel->page)
        return fail(game_bootstrap, 7);
    store(game_bootstrap, 0x497228, (unsigned long)bh_atlas_page_image(game_bootstrap->panel->page), 1);
    for (i = 0; i < 12; ++i)
        game_bootstrap->panel->icon_images[i] = bh_bootstrap_value(game_bootstrap, 0x4df1d8 + i * 4);
    game_bootstrap->panel->tv_on_sound = bh_bootstrap_value(game_bootstrap, 0x4d1988);
    game_bootstrap->panel->tv_off_sound = bh_bootstrap_value(game_bootstrap, 0x496324);
    game_bootstrap->panel->tv_char_sound = bh_bootstrap_value(game_bootstrap, 0x496570);
    if (!bh_message_panel_create(game_bootstrap->panel))
        return fail(game_bootstrap, 10);
    r.kind = 3;
    r.identity = 0x4d0478;
    r.name = "RCWAGON.BSP";
    if (!resources(game_bootstrap, &r, 1))
        return 0;
    if (game_bootstrap->compose && !game_bootstrap->compose(game_bootstrap, 4, game_bootstrap->compose_user))
        return fail(game_bootstrap, 11);
    return 1;
}
