#include "bh_game_bootstrap.h"
#include "bh_game_settings.h"
#include "bh_atlas_text.h"
#include "bh_atlas_glyph.h"
#include "bh_texture_controls.h"
#include "bh_texture_policy.h"
#include "bh_collision_resource.h"
#include "bh_model_images.h"
#include "bh_model_scale.h"
#include "bh_vfile.h"
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
typedef char mission_action_alias[sizeof(BHMenuMissionRow) == sizeof(BHMenuMissionAction) ? 1 : -1];
static int fail(BHGameBootstrap *c, int error)
{
    c->error = error;
    return 0;
}
unsigned long bh_bootstrap_value(const BHGameBootstrap *c, unsigned long identity)
{
    unsigned long i;
    if (c)
        for (i = 0; i < c->value_count; ++i)
            if (c->values[i].identity == identity)
                return c->values[i].value;
    return 0;
}
static int store(BHGameBootstrap *c, unsigned long identity, unsigned long value, unsigned long kind)
{
    unsigned long i;
    for (i = 0; i < c->value_count; ++i)
        if (c->values[i].identity == identity)
            break;
    if (i == N(c->values))
        return fail(c, 2);
    if (i == c->value_count)
        ++c->value_count;
    c->values[i].identity = identity;
    c->values[i].value = value;
    c->values[i].kind = kind;
    for (i = 0; i < c->binding_count; ++i)
        if (c->bindings[i].identity == identity)
        {
            if (!c->bindings[i].destination)
                return fail(c, 1);
            *c->bindings[i].destination = value;
        }
    return 1;
}
static int scale_captain(BHGameBootstrap *);
static int resources(BHGameBootstrap *c, const BHBootstrapResource *ops, unsigned long count)
{
    unsigned long i, v;
    const BHBootstrapResource *r;
    for (i = 0; i < count; ++i)
    {
        r = ops + i;
        v = 0;
        switch (r->kind)
        {
        case 1:
            if (!c->textures)
                return fail(c, 1);
            v = (unsigned long)bh_texture_image_load(c->textures, r->name);
            if (!v)
                return fail(c, 3);
            break;
        case 2:
            if (!c->sounds)
                return fail(c, 1);
            v = bh_sound_load(c->sounds, r->name, r->range, r->level, (unsigned short)r->streaming, (float)r->argument);
            if (v == BH_SOUND_INVALID)
                return fail(c, 4);
            break;
        case 3:
            if (!c->resources)
                return fail(c, 1);
            v = bh_render_resources_load_model(c->resources, r->name);
            if (!v)
                return fail(c, 5);
            break;
        case 4:
            v = bh_bootstrap_value(c, (unsigned long)r->argument);
            ++c->aliases;
            break;
        case 5:
            v = 0;
            break;
        case 6:
            if (!c->sounds || !c->sounds->queue)
                return fail(c, 1);
            bh_sound_set_voice_limit(c->sounds->queue, r->identity);
            continue;
        case 9:
            if (!scale_captain(c))
                return 0;
            continue;
        case 7:
            if (!bh_texture_policy_set(c->textures, r->identity, (unsigned long)r->argument))
                return fail(c, 3);
            continue;
        default:
            return fail(c, 1);
        }
        if (!store(c, r->identity, v, r->kind))
            return 0;
        ++c->resources_loaded;
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
static BHBootstrapMenu *menu(BHGameBootstrap *c, unsigned long id)
{
    unsigned long i;
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
        if (c->menus[i].identity == id)
            return c->menus + i;
    return 0;
}
static BHBootstrapMenu *action_menu(BHMenuActions *a)
{
    return (BHBootstrapMenu *)((char *)a - offsetof(BHBootstrapMenu, actions));
}
static void submenu_first(BHMenuActions *a, long index)
{
    (void)index;
    bh_menu_submenu_first(a->settings);
}
static void network_ship_picker(BHMenuActions *a, long index)
{
    BHMenuRuntime *picker = action_menu(a)->owner->runtime->network_ship_menu;
    (void)index;
    if (a->network_ship)
    {
        picker->actions->menu->selection = *a->network_ship;
        picker->actions->menu->display_selection = (float)*a->network_ship;
    }
}
static void network_ship_select(BHMenuActions *a, long index)
{
    if (a->network_ship)
        *a->network_ship = index;
}
static void save(BHMenuActions *a, long index)
{
    (void)index;
    if (!bh_config_save(a->config, a->save_path))
        a->error = 1;
}
static void quit(BHMenuActions *a, long index)
{
    BHBootstrapMenu *m = action_menu(a);
    (void)index;
    if (!m->owner->transition || !bh_menu_quit_begin(m->owner->transition))
        a->error = 1;
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
static void row_refresh(BHMenuSettingsMenu *s, long row)
{
    BHBootstrapMenu *m = (BHBootstrapMenu *)s->user;
    BHMenuMissionAction fn = 0;
    if (!m || row < 0 || (unsigned long)row >= s->row_count)
        return;
    m->edits.menu = s;
    m->edits.rows = &m->object.rows;
    switch (m->refresh[row])
    {
    case 0x42d920:
        bh_menu_network_address(&m->actions, row);
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
        fn(&m->edits, row);
}
static void initial_refresh(void *s, long index)
{
    row_refresh((BHMenuSettingsMenu *)s, index);
}
static int add_row(BHGameBootstrap *c, const BHBootstrapRowSpec *r)
{
    BHBootstrapMenu *m = menu(c, r->menu), *destination;
    unsigned long row;
    unsigned char body[512], *p = 0;
    BHMenuRow *made;
    if (!m)
        return fail(c, 1);
    row = m->object.rows.count;
    if (row >= BH_BOOTSTRAP_ROWS)
        return fail(c, 2);
    if (r->body)
    {
        const char *text = r->body;
        if (strlen(text) >= sizeof(body))
            return fail(c, 2);
        strcpy((char *)body, text);
        p = body;
    }
    destination = r->destination ? menu(c, r->destination) : 0;
    if (r->destination && !destination)
        return fail(c, 1);
    m->action[row] = action(r->action);
    m->edit_action[row] = edit_action(r->action);
    if (r->action && !m->action[row] && !m->edit_action[row])
        return fail(c, 6);
    made = bh_menu_object_add_row(&m->object, bh_bootstrap_value(c, r->button), (const unsigned char *)r->title, p,
                                  r->destination, r->action, r->refresh ? initial_refresh : 0);
    if (!made)
        return fail(c, 7);
    m->destinations[row] = destination ? &destination->runtime : 0;
    m->refresh[row] = r->refresh;
    m->settings_rows[row].refresh = r->refresh ? row_refresh : 0;
    m->row_pointers[row] = &m->settings_rows[row];
    m->settings.row_count = m->object.rows.count;
    m->present_rows[row].row = made;
    m->present_rows[row].image_handle = (unsigned long)made->image;
    m->present.row_count = m->object.rows.count;
    ++c->rows_created;
    return 1;
}
int bh_bootstrap_menus(BHGameBootstrap *c)
{
    unsigned long i, j;
    BHBootstrapMenu *m;
    if (!c || !c->pool || !c->inputs || !c->atlas || !c->text || !c->runtime || !c->actions || !c->edits)
        return c ? fail(c, 1) : 0;
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
    {
        m = c->menus + i;
        if (m->object.entity)
            return fail(c, 1);
        m->identity = menu_specs[i].identity;
        m->owner = c;
        m->actions = *c->actions;
        m->edits = *c->edits;
        if (!bh_menu_object_create(&m->object, c->pool, c->inputs, menu_specs[i].tag, "") ||
            !bh_menu_object_rows_init(&m->object, c->atlas, c->text, m->rows, BH_BOOTSTRAP_ROWS))
            return fail(c, 7);
        m->root.menu = &m->settings;
        m->root.row_actions = m->action;
        m->root.action_count = BH_BOOTSTRAP_ROWS;
        m->settings.rows = m->row_pointers;
        m->settings.user = m;
        m->actions.menu = &m->settings;
        m->actions.rows = &m->object.rows;
        m->edits.menu = &m->settings;
        m->edits.rows = &m->object.rows;
        if (c->present)
            m->present = *c->present;
        m->present.rows = m->present_rows;
        m->runtime.context = c->runtime;
        m->runtime.entity = m->object.entity;
        m->runtime.actions = &m->root;
        m->runtime.action_context = &m->actions;
        m->runtime.edit_context = &m->edits;
        m->runtime.edit_actions = m->edit_action;
        m->runtime.destinations = m->destinations;
        m->runtime.input = m->object.input;
        m->runtime.present = &m->present;
        m->runtime.pending_selection = -1;
        m->runtime.back_selection = menu_backs[i];
        m->runtime.slide_voice = BH_SOUND_INVALID;
        m->runtime.waypoint = (long)menu_specs[i].tag;
        ++c->menus_created;
    }
    for (i = 0; i < N(row_specs); ++i)
        if (!add_row(c, row_specs + i))
            return 0;

    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
    {
        for (j = 0; j < 9; ++j)
            c->menus[i].edits.mission_rows[j] = (BHMenuMissionRow *)(menu(c, 0x4ce450)->edit_action + j);
        c->menus[i].edits.mission_menu = &menu(c, 0x4ce450)->settings;
    }
    if (c->actions->settings)
        c->actions->settings->ship_menu = &menu(c, 0x4cdc9c)->settings;
    c->runtime->root = &menu(c, 0x495e40)->runtime;
    c->runtime->network_menu = &menu(c, 0x4d1488)->runtime;
    c->runtime->network_ship_menu = &menu(c, 0x4cdca0)->runtime;
    c->runtime->ship_menu = &menu(c, 0x4cdc9c)->runtime;
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
    {
        m = c->menus + i;
        if (!bh_menu_runtime_bind(&m->runtime))
            return fail(c, 8);
        m->object.entity->step = 0;
        m->object.entity->refresh = 0;
    }
    m = menu(c, 0x495e40);
    m->runtime.entity->step = bh_menu_runtime_step_entity;
    m->runtime.entity->refresh = bh_menu_runtime_refresh_entity;
    m->runtime.distance = 100;
    m->runtime.exiting = 0;
    if (m->runtime.waypoint >= 0)
        *c->runtime->waypoint = m->runtime.waypoint;
    if (!bh_input_set_enabled(m->runtime.input, c->snapshot, 1))
        return fail(c, 8);
    for (i = 0; i < m->settings.row_count; ++i)
        if (m->settings_rows[i].refresh)
            m->settings_rows[i].refresh(&m->settings, (long)i);
    for (i = 0; i < m->settings.row_count; ++i)
        if (m->destinations[m->settings.selection] || m->action[m->settings.selection] ||
            m->edit_action[m->settings.selection])
            break;
        else
            m->settings.selection = (m->settings.selection + 1) % m->settings.row_count;
    if (i == m->settings.row_count)
        return fail(c, 8);
    return 1;
}
static int font(BHGameBootstrap *c, BHAtlasPage **page, unsigned long identity, unsigned long size)
{
    *page = bh_atlas_page_create(c->atlas, "textures\\tvfont.pcx", size);
    return *page && store(c, identity, (unsigned long)bh_atlas_page_image(*page), 1);
}
static int scale_captain(BHGameBootstrap *c)
{
    const BHDrawResource *r = bh_render_resources_lookup(bh_bootstrap_value(c, 0x4d047c), c->resources);
    BHModelDrawCache *cache;
    if (!r || r->tag != BH_RESOURCE_TAG_MODEL)
        return fail(c, 5);
    cache = r->data.model.instance->cache;
    return bh_model_scale(r->data.model.instance->model, 3, 3, 3, &cache, 1) ? 1 : fail(c, 5);
}
static void fade_refresh(BHEntity *e, void *packet)
{
    (void)packet;
    bh_menu_music_fade(e);
}
int bh_game_bootstrap(BHGameBootstrap *c)
{
    BHVFile *file;
    BHBootstrapResource r;
    BHVec3 start = {15, -14.699999809265137f, -40};
    BHWorldGeometry *first;
    BHBootstrapMenu *m;
    const BHDrawResource *resource;
    BHModel *model;
    BHEntity *e;
    unsigned long i;
    long width, index;
    static const char *confirm[3] = {"Abandon", "Game ?", "[Y/N]"};
    static const long ys[3] = {13, 26, 40};
    if (!c || !c->textures || !c->resources || !c->sounds || !c->atlas || !c->text || !c->blocks || !c->world ||
        !c->player || !c->pool || !c->inputs || !c->story || !c->availability || !c->panel || !c->transition)
        return c ? fail(c, 1) : 0;
    c->error = 0;
    if (!bh_bootstrap_resources(c, 0) || !bh_texture_set_path(c->textures, "textures", 0) ||
        !bh_collision_resource_path(&c->collision_path, "bsp", 0))
        return fail(c, c->error ? c->error : 3);
    c->blocks->collision_prefix = c->collision_path.prefix;
    c->blocks->collision_search = c->collision_path.search;
    c->player->prefix = c->collision_path.prefix;
    c->player->search = c->collision_path.search;
    if (bh_script_blocks_load(c->blocks, "bsp\\blocks.txt"))
    {
        if (!bh_world_load(c->world, c->custom_level ? "level.lev" : "test.lev") ||
            (c->custom_level && (!c->world->editor->world->count || c->world->skipped)))
            return fail(c, 9);
        first = (c->world->editor->world->count ? &c->world->editor->world->nodes[0]->placement.geometry : 0);
        if (first)
        {
            if (!c->editor || !c->navigation || !bh_editor_input_setup(c->editor))
                return fail(c, 1);
            file = c->custom_level ? bh_vfile_open_level("level.pth") : bh_vfile_open("test.pth", "rb");
            index = bh_navigation_build(c->navigation, first, c->world->editor->world->count, file);
            if (file)
                bh_vfile_close(file);
            if (!index)
                return fail(c, 9);
        }
    }
    else if (c->custom_level)
        return fail(c, 9);
    if (c->compose && !c->compose(c, 1, c->compose_user))
        return fail(c, 11);
    bh_vfile_preload("bsp\\hun_lod1.BSP", "rb");
    bh_vfile_preload("bsp\\hun_lod2.BSP", "rb");
    if (!bh_texture_set_path(c->textures, "textures", 0) || !bh_bootstrap_resources(c, 1))
        return 0;
    r.kind = 1;
    r.identity = 0x4d1aa0;
    r.name = "water.gif";
    r.range = r.level = 0;
    r.streaming = 0;
    r.argument = 0;
    if (!resources(c, &r, 1) || !font(c, &c->confirmation, 0x4962f4, 64))
        return fail(c, 7);
    if (!bh_atlas_page_clear(c->atlas, c->confirmation, 0))
        return fail(c, 7);
    for (i = 0; i < 3; ++i)
    {
        width = bh_atlas_page_text_width(c->confirmation, (const unsigned char *)confirm[i]);
        bh_atlas_page_cursor(c->confirmation, (long)(32 - (double)width * .5), ys[i]);
        bh_atlas_page_text(c->confirmation, (const unsigned char *)confirm[i]);
    }
    if (!bh_player_create(c->player, 0, &start, 1, bh_bootstrap_value(c, 0x4d1964)))
        return fail(c, 10);
    if (c->compose && !c->compose(c, 2, c->compose_user))
        return fail(c, 11);
    e = bh_entity_acquire(c->pool);
    if (e)
    {
        c->story->player = *c->player->current;
        if (!bh_mission_story_bind(c->story, e))
            return fail(c, 10);
        memset(c->availability, 0, 4 * 32 * 4);
    }
    if (!font(c, &c->menu_font, 0x4cdb9c, 128) || !bh_bootstrap_resources(c, 2))
        return fail(c, c->error ? c->error : 7);
    c->graffiti = bh_atlas_page_create(c->atlas, "textures\\graffiti.pcx", 128);
    if (!c->graffiti || !store(c, 0x4d1910, (unsigned long)bh_atlas_page_image(c->graffiti), 1))
        return fail(c, 7);
    for (i = 0; i < c->blocks->count; ++i)
        if (!_stricmp(c->blocks->blocks[i].definition.name, "graffiti.bsp"))
        {
            resource = bh_render_resources_lookup(c->blocks->blocks[i].resource, c->resources);
            if (!resource || resource->tag != BH_RESOURCE_TAG_MODEL)
                return fail(c, 5);
            model = resource->data.model.instance->model;
            index = bh_model_image_find(model, "win.pcx");
            bh_model_image_replace(model, index, bh_atlas_page_image(c->graffiti));
        }
    bh_text_queue_clear(c->text, c->graffiti, 0);
    {
        const char *name = bh_game_settings_player_name();
        if (!*name)
        {
            name = getenv("COMPUTERNAME");
            if (!name || !*name)
                name = getenv("HOSTNAME");
            if (!name || !*name)
                name = "BHunter";
            strncpy(c->computer_name, name, sizeof(c->computer_name) - 1);
            c->computer_name[sizeof(c->computer_name) - 1] = 0;
            name = c->computer_name;
        }
        strncpy(c->player_name, name, sizeof(c->player_name) - 1);
        c->player_name[sizeof(c->player_name) - 1] = 0;
    }
    c->all_keys = bh_player_input_create(c->inputs, 0);
    if (!c->all_keys)
        return fail(c, 10);
    for (i = 0; i < 255; ++i)
        bh_player_input_bind_key(c->all_keys, (long)i, "", 0, 0, (unsigned short)i);
    if (c->compose && !c->compose(c, 5, c->compose_user))
        return fail(c, 11);
    if ((!c->flyby || !bh_flyby_construct(c->flyby)) || !bh_bootstrap_menus(c))
        return fail(c, c->error ? c->error : 10);
    if (c->compose && !c->compose(c, 3, c->compose_user))
        return fail(c, 11);
    if (!c->transition->cd_ready)
    {
        e = bh_entity_acquire(c->pool);
        if (!e || !bh_menu_transition_bind(c->transition, e, 2, 0))
            return fail(c, 10);
        c->transition->music = e;
        e->step = bh_menu_music_tick;
    }
    r.identity = 0x4d4424;
    r.name = "win.bmp";
    if (!resources(c, &r, 1))
        return 0;
    e = bh_entity_acquire(c->pool);
    if (e)
    {
        if (!bh_menu_transition_bind(c->transition, e, 2, 0))
            return fail(c, 10);
        e->step = bh_menu_fade_expiry;
        e->refresh = fade_refresh;
    }
    if (!bh_bootstrap_resources(c, 3) || !bh_texture_set_path(c->textures, "textures", 0) ||
        !bh_bootstrap_resources(c, 4))
        return fail(c, c->error ? c->error : 3);
    c->panel->page = bh_atlas_page_create(c->atlas, "textures\\tvfont.pcx", 128);
    if (!c->panel->page)
        return fail(c, 7);
    store(c, 0x497228, (unsigned long)bh_atlas_page_image(c->panel->page), 1);
    for (i = 0; i < 12; ++i)
        c->panel->icon_images[i] = bh_bootstrap_value(c, 0x4df1d8 + i * 4);
    c->panel->tv_on_sound = bh_bootstrap_value(c, 0x4d1988);
    c->panel->tv_off_sound = bh_bootstrap_value(c, 0x496324);
    c->panel->tv_char_sound = bh_bootstrap_value(c, 0x496570);
    if (!bh_message_panel_create(c->panel))
        return fail(c, 10);
    r.kind = 3;
    r.identity = 0x4d0478;
    r.name = "RCWAGON.BSP";
    if (!resources(c, &r, 1))
        return 0;
    if (c->compose && !c->compose(c, 4, c->compose_user))
        return fail(c, 11);
    return 1;
}
