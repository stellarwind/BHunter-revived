#include "bh_game_app.h"
#include "bh_render_interpolation.h"
#include "bh_mission_profile.h"
#include "bh_application_bootstrap.h"
#include "bh_primitive_dispatch.h"
#include "bh_sprite_quad.h"
#include "bh_horizon_color.h"
#include "bh_model_path.h"
#include "bh_texture_policy.h"
#include "bh_loading_image.h"
#include "bh_light_config.h"
#include "bh_gdi_text.h"
#include "bh_attachment.h"
#include "bh_input_text.h"
#include "bh_menu_profile.h"
#include "bh_force_feedback_update.h"
#include "bh_target_construct.h"
#include "bh_actor_response.h"
#include "bh_effect_orbit_tick.h"
#include "bh_effect_accelerated.h"
#include "bh_effect_split_tick.h"
#include "bh_actor_construct.h"
#include "bh_actor_launch.h"
#include "bh_script_actor_construct.h"
#include "bh_network_shutdown.h"
#include "bh_display_shutdown.h"
#include "bh_vfile.h"
#include "bh_fp.h"
#include "bh_sound_startup.h"
#include "bh_effect_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define GAME_WORLD_CAPACITY 6000
#define GAME_DYNAMIC_CAPACITY 256
#define GAME_STARTS 32
#define N(a) (sizeof(a) / sizeof((a)[0]))
#ifdef DEBUGGING
typedef struct BHGamePerfSample
{
    double time_ms, interval_ms, simulation_ms, render_ms, present_ms;
    unsigned long frame, produced, mode, paused, commands, entities, projectiles, contacts, rebuilds;
} BHGamePerfSample;
#define PERF_BLOCK_SAMPLES 4096
typedef struct BHGamePerfBlock
{
    struct BHGamePerfBlock *next;
    unsigned long count;
    BHGamePerfSample samples[PERF_BLOCK_SAMPLES];
} BHGamePerfBlock;
typedef struct BHGamePerfCapture
{
    BHGamePerfBlock *head, *tail;
    unsigned long count, dropped, previous[7];
} BHGamePerfCapture;
static BHGamePerfSample *perf_append(BHGamePerfCapture *c)
{
    BHGamePerfBlock *b;
    if (!c->tail || c->tail->count == PERF_BLOCK_SAMPLES)
    {
        b = (BHGamePerfBlock *)calloc(1, sizeof(*b));
        if (!b)
        {
            ++c->dropped;
            return NULL;
        }
        if (c->tail)
            c->tail->next = b;
        else
            c->head = b;
        c->tail = b;
    }
    ++c->count;
    return c->tail->samples + c->tail->count++;
}
static unsigned long perf_contact_delta(BHGamePerfCapture *c, const unsigned long *current)
{
    unsigned long i, total = 0;
    for (i = 0; i < 7; ++i)
    {
        total += current[i] >= c->previous[i] ? current[i] - c->previous[i] : current[i];
        c->previous[i] = current[i];
    }
    return total;
}
static void perf_dispose(BHGamePerfCapture *c)
{
    BHGamePerfBlock *b = c->head, *next;
    while (b)
    {
        next = b->next;
        free(b);
        b = next;
    }
    memset(c, 0, sizeof(*c));
}

#endif
typedef struct BHGameApp
{
    HWND window;
    HINSTANCE instance;
    int box_level, box_auto;
    unsigned long box_ticks;
    FILE *box_trace;
    BHEntity *box_ai, *box_pickup, *box_drop;
    unsigned long box_pickup_generation, box_target_generation, box_drop_generation;
    int box_objective, box_drop_seen, box_drop_taken;
    long box_drop_ammo;
    BHEntity *box_weapon_pickup;
    unsigned long box_weapon_generation, box_collected_mask, box_fired_mask, box_spawned_mask, box_emit_before;
    long box_ammo_before, box_fire_ammo;
    int sound_diagnostic_logged;
#ifdef DEBUGGING
    double menu_smoke_seconds, menu_smoke_origin;
#endif
    int startup_check, hardware, fullscreen, quit, initialized, error, focus_sample;
    FILE *log;
    char archive[512], config_path[512];
    long width, height, selected_waypoint, mission, ship, slot, input_changed, stay, network_failed, fine_warning,
        pending_profile;
    unsigned long frame, sync_identity, actor_count, availability[4][32], event_value, mission_identity, instability;
    float yaw, latency[8];
    BHVec3 zero, eye, angles;
    BHEntity *player, *camera_entity, *damage_source, *actor_head, *actors[GAME_DYNAMIC_CAPACITY + 2], *remote[8];
    BHMathEnvironment math;
    BHConfig config;
    BHInputProfile *profile;
    BHPlayerInputOwner inputs;
    BHPlayerInputState decoded, global_state;
    BHInputSnapshot snapshot;
    BHInputCollection collection;
    BHInputDevices devices;
    BHLoadingImages loading_images;
    BHLoadingDisplay loading_display;
    BHLoadingProgress loading_progress;
    BHFrameBuffers frames;
    BHDrawQueue queue, slots[3];
    BHRenderInterpolation interpolation;
    BHTextQueue text;
    BHFrameService service;
    BHFrameCallback callback;
    BHFrameWorker worker;
    LPDIRECTDRAW draw;
    LPDIRECTDRAW2 draw2;
    LPDIRECTDRAWSURFACE primary, target;
    LPDIRECTDRAWCLIPPER window_clipper;
    BHDisplayCatalog display_catalog;
    BHDeviceName device_names[8];
    char text_buffer[160];
    BHRenderMode render_mode;
    BHApplicationBootstrap application;
    BHRenderMaterial material;
    BHViewConfig view, scene_view;
    BHMainCamera main_camera;
    BHClipContext clip;
    BHScreenClipContext screen_clip;
    BHAttributeClipContext attribute_clip;
    BHLightConfig light;
    BHLightContext lights;
    BHModelContext models;
    BHRenderResources resources;
    BHModelInstanceContext prototype;
    BHAtlasContext atlas;
    BHRenderState render_state;
    BHPrimitiveDispatch primitive;
    BHHorizonColor horizon_colors;
    BHSpriteBillboardContext billboard;
    BHSpriteOrientedContext oriented;
    BHSpriteOrientedVertex path[4096];
    BHSpriteQuadContext sprite_quad;
    BHScreenQuadContext screen_quad;
    BHPickupDrawContext pickup_draw;
    BHDrawResourceContext resource_draw;
    BHDrawSubmitContext model_submit;
    BHDrawBillboardSubmitContext billboard_submit;
    BHScreenQuadSubmit screen_submit;
    BHQuadQueueContext quad_submit;
    BHPathQueueContext path_submit;
    BHSegmentQueueContext segment_submit;
    BHDrawControlContext controls;
    BHPortalClipCache portal_cache;
    BHHorizonContext horizon;
    BHHorizonQueueContext horizon_submit;
    BHFlareSubmit flare_submit;
    BHFrameDisplay display;
    BHFadeContext fade;
    BHPresentContext present;
    BHPresentBegin begin;
    BHRenderExecute render;
    BHGdiText gdi;
    BHSoundQueue sound;
    BHSoundStartupStats sound_stats;
    BHSoundLoad sounds;
    BHProviderCatalog providers;
    BHProviderStartup provider_startup;
    BHModemNames modems;
    BHNetworkSessions sessions;
    BHNetworkTransport transport;
    BHNetworkGame network_game;
    BHNetworkShutdown network_shutdown;
    BHForceFeedback force;
    BHForceFeedbackStart force_start;
    BHForceFeedbackInit force_init;
    BHForceFeedbackUpdate force_update;
    BHCdClose cd;
    BHCdNotify cd_notify;
    BHCdAudio cd_audio;
    BHMediaControl media;
    BHDisplayShutdown display_shutdown;
    BHGameBootstrap bootstrap;
    BHBootstrapBinding bindings[256];
    unsigned long binding_count;
    BHScriptVMContext vm;
    BHScriptParse parser;
    BHScriptBlocks blocks;
    BHScriptLifetimeContext lifetime;
    BHScriptMotionContext motion;
    BHScriptMotionBinding *motion_bindings;
    BHScriptSensorContext sensors;
    BHScriptEmitterContext emitters;
    BHScriptTimerActor timer_actors[GAME_WORLD_CAPACITY];
    BHScriptVMActor vm_actors[GAME_WORLD_CAPACITY];
    BHScriptVMWaypoint vm_starts[GAME_STARTS];
    BHPlayerCameraStart starts[GAME_STARTS];
    BHVec3 route_starts[GAME_STARTS];
    BHEntityPool pool;
    BHEditorWorld editor_world;
    BHEditorControl editor;
    BHEditorNode *nodes[GAME_WORLD_CAPACITY];
    BHWorldLoad world_load;
    BHEditorInput editor_input;
    BHPlayerInputState editor_state;
    BHScriptBrowser browser;
    BHEditorNumericContext numeric;
    BHRailPaths rails;
    BHRailWagonContext wagon;
    BHNavigation navigation;
    BHWorldSegmentContext segment;
    BHWorldSegmentRoom *segment_rooms;
    BHSceneBindings scene_bindings;
    BHSceneBindingInput *scene_inputs;
    BHPickupRoom *rooms;
    BHEntityScheduleRoom *scheduled;
    BHEntityScheduleContext schedule;
    BHSceneOwner scene;
    BHWorldReset world_reset;
    BHWorldResetNode *reset_nodes;
    BHPhysicsState player_motion;
    BHMat34 player_inverse;
    BHPhysicsStepContext physics;
    BHPhysicsStepWorld *physics_worlds;
    BHGeometry *physics_models[GAME_DYNAMIC_CAPACITY + 1];
    BHPhysicsStepBody physics_bodies[GAME_DYNAMIC_CAPACITY + 1];
    BHPhysicsEffects physics_effects;
    BHActorBasisContext basis;
    BHActorBasisSample samples[15];
    BHActorBasisGroup basis_a[7], basis_b[7];
    BHActorBasisIndices indices_a[7], indices_b[7];
    BHActorBasisScale scales_a[7], scales_b[7];
    BHPlayerForceControl player_force;
    BHPlayerTickState player_state;
    BHPlayerTick player_tick;
    BHPlayerCreate player_create;
    BHPlayerQueryContext player_query;
    BHPlayerReset player_reset;
    BHPlayerCameraContext player_camera;
    BHCameraStateContext camera_state;
    BHCameraCorrectionOwner correction;
    BHCollisionModel player_collision;
    BHPlayerScene player_scene;
    BHSceneBackground background;
    BHHudContext hud;
    BHHudMarkers markers;
    BHRadarFrame radar_frame;
    BHRadarContext radar;
    BHAttachmentContext attachment;
    BHHudWorldContext world_hud;
    BHHudFlatContext flat_hud;
    BHHudScores scores;
    BHHudScoreDraw score_draw;
    BHHudScoreText score_text;
    BHFlybyScene flyby;
    BHFlybyConstruct flyby_construct;
    BHFlybyCameraContext flyby_camera;
    BHFlybyCameraState flyby_state;
    BHCameraRouteContext camera_route;
    BHFlybyCredits credits;
    BHMenuRuntimeContext menu;
    BHMenuActions menu_actions;
    BHMenuActionsRoot *root_menu;
    BHMenuEdits menu_edits;
    BHMenuSettings menu_settings;
    BHMenuProfileContext menu_profiles;
    BHMenuPresent menu_present;
    BHMenuTransition transition;
    BHInputTextCapture text_capture;
    BHInputKeyCapture key_capture;
    BHMessageState messages;
    BHMessagePanel panel;
    BHMissionProgressContext mission_progress;
    BHMissionStory story;
    BHWeaponFactory weapons;
    BHWeaponDispatch weapon_dispatch;
    BHWeaponReadyContext weapon_ready;
    BHWeaponTargetContext weapon_target;
    BHWeaponArcLifecycleContext weapon_arc;
    BHWeaponEmptyContext weapon_empty;
    BHWeaponFireContext weapon_fire;
    BHWeaponPulseContext weapon_pulse;
    BHWeaponEffectFireContext weapon_effect;
    BHWeaponPulseLaunchContext weapon_launch;
    BHWeaponArcFireContext weapon_arc_fire;
    BHArcContext arc_hud;
    BHWeaponHudTargetContext target_hud;
    BHProjectileCreateContext projectiles;
    BHProjectileLifetime projectile_lifetime;
    BHProjectileDrawContext projectile_draw;
    BHBallisticProjectileContext ballistic;
    BHHomingProjectileContext homing, projectile_effects;
    BHAreaProjectileContext area;
    BHEffectProjectileContext effect;
    BHEffectOrbitTickContext orbit;
    BHEffectAcceleratedContext accelerated;
    BHEffectSplitContext split;
    BHEffectNetworkContext effect_network;
    BHPulseProjectileContext pulse;
    BHScriptParticleContext particles;
    BHContactEffects contacts;
    BHFlashFlareContext flash;
    BHScriptDebrisContext debris;
    BHScriptExplosionContext explosions;
    BHHitMarkerContext hit_markers;
    BHPickupContext pickups;
    BHPickupRenderBinding pickup_render;
    BHEntityResourceDrawContext special_draw;
    unsigned long pickup_resources[22];
    BHDebrisContext debris_response;
    BHTargetConstructContext targets;
    BHTargetConstructSlot target_slots[GAME_DYNAMIC_CAPACITY];
    BHTargetDrawContext target_draw;
    BHActorRouteContext actor_route;
    BHActorDamageContext actor_damage;
    BHActorAttackContext actor_attack;
    BHActorVehicleLifetimeContext vehicle_lifetime;
    BHActorResponseContext actor_response;
    BHActorConstructContext actor_construct;
    BHActorConstructRoom *actor_rooms;
    BHActorVehicleDrawContext vehicle_draw;
    BHActorLaunchContext actor_launch;
    unsigned long actor_models[25], actor_sounds[25], cycle_models[4];
    BHScriptActorConstructorContext script_actors;
    BHScriptActorDrawContext script_draw;
    BHRadarQueryContext radar_query;
    BHActorPursuitRouteContext pursuit;
    BHSegmentQuery block_queries[600];
    BHHomingWorldResponse *world_responses;
    BHPulseWorldIndex *pulse_indices;
    long *city_ids;
    BHEffectDrawContext effect_draw;
    BHTrailContext trails;
    BHEffectPuffContext puffs;
    BHPulseVisualContext pulse_draw;
    BHGeometry *base_models[1];
    BHWorldSegmentActor segment_actors[GAME_DYNAMIC_CAPACITY + 1];
    BHSegmentQuery actor_queries[GAME_DYNAMIC_CAPACITY + 1];
    unsigned long actor_query_generations[GAME_DYNAMIC_CAPACITY + 1];
    BHEntity *actor_query_entities[GAME_DYNAMIC_CAPACITY + 1];
    unsigned char key_result[2];
    unsigned long empty_warning;
    long weapon_warning;
    BHWorldGeometry *query_world;
    unsigned short query_world_index;
    const BHProviderRecord *selected_provider;
#ifdef DEBUGGING
    int curve_diagnostic_written;
    int draw_warning_logged;
    int perf_enabled;
    BHGamePerfCapture perf_capture;
    unsigned long perf_projectiles;
    LARGE_INTEGER perf_frequency, perf_origin, perf_previous;
    char perf_path[128];
#endif
} BHGameApp;
static HWND input_window;
/* Original city-name pointer table at0048fb58, indexed by placement city ID. */
static const char *const city_names[16] = {"Downtown",
                                           "Old Bronx",
                                           "Central Stadium",
                                           "The Docks",
                                           "Industrial Complex",
                                           "Space Port",
                                           "Underground City",
                                           "Scraptown",
                                           "Grand Vegas",
                                           "Financial District",
                                           "Mall Town",
                                           "Chinatown",
                                           "the Aqua Tunnels\nnear The Docks",
                                           "Mining Tunnels\nat Industrial\nComplex",
                                           "City Ring",
                                           "Bronx/Downtown\nMetro Station"};
#ifdef DEBUGGING
static void stage(BHGameApp *a, const char *text)
{
    if (a->log)
    {
        fprintf(a->log, "%s\n", text);
        fflush(a->log);
    }
    printf("%s\n", text);
    fflush(stdout);
}
static void curve_words(FILE *f, const void *values, unsigned long count)
{
    unsigned long i, bits;
    const unsigned char *p = (const unsigned char *)values;
    fputc('[', f);
    for (i = 0; i < count; ++i)
    {
        memcpy(&bits, p + i * 4, 4);
        fprintf(f, "%s\"%08lx\"", i ? "," : "", bits);
    }
    fputc(']', f);
}
static void __cdecl curve_diagnostic(void *user, unsigned long owner, const BHEntity *e, const BHActorRouteState *s,
                                     unsigned short now, const BHVec3 *before, const BHVec3 *at, const BHVec3 *after,
                                     const BHVec3 *angles)
{
    BHGameApp *a = (BHGameApp *)user;
    FILE *f;
    char path[96], message[192];
    unsigned long model;
    int written = 0;
    if (a->curve_diagnostic_written)
        return;
    a->curve_diagnostic_written = 1;
    memcpy(&model, (const char *)e + 0x114, 4);
    sprintf(path, "nonfinite-curve-%08lx-%lu.json", GetTickCount(), GetCurrentProcessId());
    f = fopen(path, "w");
    if (f)
    {
        fprintf(
            f,
            "{\"owner\":\"%08lx\",\"entity\":\"%08lx\",\"generation\":%lu,\"model\":%lu,\"clock32\":%lu,\"now16\":%u,\"stamp16\":%u,\"duration16\":%u,\"controls_bits\":",
            owner, (unsigned long)e, e->generation, model, a->player_state.tick, (unsigned int)now,
            (unsigned int)s->tick, (unsigned int)s->duration_ticks);
        curve_words(f, s->controls, 12);
        fprintf(f, ",\"before_bits\":");
        curve_words(f, before, 3);
        fprintf(f, ",\"at_bits\":");
        curve_words(f, at, 3);
        fprintf(f, ",\"after_bits\":");
        curve_words(f, after, 3);
        fprintf(f, ",\"angles_bits\":");
        curve_words(f, angles, 3);
        fprintf(f, "}\n");
        written = !ferror(f);
        if (fclose(f))
            written = 0;
    }

#ifdef DEBUGGING
    sprintf(message, "Nonfinite curve producer: owner=%08lx model=%lu evidence=%s written=%d", owner, model, path,
            written);
#endif
    stage(a, message);
}
#else
#define stage(a, text) ((void)0)
#endif
static int failure(BHGameApp *a, int code, const char *text)
{
    a->error = code;
    stage(a, text);
    return 0;
}
#ifdef DEBUGGING
static double perf_ms(BHGameApp *a, LARGE_INTEGER end, LARGE_INTEGER start)
{
    return (double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)a->perf_frequency.QuadPart;
}
static int perf_start(BHGameApp *a)
{
    if (!a->perf_enabled)
        return 1;
    if (!QueryPerformanceFrequency(&a->perf_frequency) || a->perf_frequency.QuadPart <= 0)
        return failure(a, 12, "Frame timer unavailable");

    QueryPerformanceCounter(&a->perf_origin);
    sprintf(a->perf_path, "frame-times-%08lx-%lu.csv", GetTickCount(), GetCurrentProcessId());
    stage(
        a,
        "Frame timing enabled: samples buffered in memory until exit, growing4096-sample blocks; no fixed presentation limit");
    return 1;
}
static void perf_record(BHGameApp *a, int produced, LARGE_INTEGER simulation_start, LARGE_INTEGER render_start,
                        LARGE_INTEGER present_start, LARGE_INTEGER end)
{
    BHGamePerfSample *s;
    BHEntity *e;
    unsigned long contacts[7], count = 0;
    s = perf_append(&a->perf_capture);
    if (!s)
        return;
    s->time_ms = perf_ms(a, end, a->perf_origin);
    s->interval_ms = a->perf_previous.QuadPart ? perf_ms(a, end, a->perf_previous) : 0;
    a->perf_previous = end;
    s->simulation_ms = produced ? perf_ms(a, render_start, simulation_start) : 0;
    s->render_ms = perf_ms(a, present_start, render_start);
    s->present_ms = perf_ms(a, end, present_start);
    s->frame = a->frame;
    s->produced = produced;
    s->mode = a->player_state.mode;
    s->paused = a->player_state.paused;
    s->commands = a->slots[a->frames.latest].count;
    s->entities = a->pool.used;
    s->rebuilds = a->targets.rebuilds;
    if (produced)
    {
        a->perf_projectiles = 0;
        for (e = a->pool.active; e && count++ < a->pool.capacity; e = e->next)
            if (e->query == bh_projectile_query)
                ++a->perf_projectiles;
    }
    s->projectiles = a->perf_projectiles;
    contacts[0] = a->physics.contacts;
    contacts[1] = a->ballistic.bindings.world_hits;
    contacts[2] = a->ballistic.bindings.proximity;
    contacts[3] = a->homing.world_hits;
    contacts[4] = a->homing.proximity;
    contacts[5] = a->projectile_effects.world_hits;
    contacts[6] = a->projectile_effects.proximity;
    s->contacts = perf_contact_delta(&a->perf_capture, contacts);
}
static void perf_save(BHGameApp *a)
{
    FILE *f;
    unsigned long i;
    BHGamePerfSample *s;
    BHGamePerfBlock *block;
    char text[256];
    if (!a->perf_capture.head)
        return;
    f = fopen(a->perf_path, "w");
    if (f)
    {
        fprintf(
            f,
            "time_ms,interval_ms,simulation_ms,render_ms,present_ms,frame,produced,mode,paused,commands,entities,projectiles,contacts,rebuilds\n");
        for (block = a->perf_capture.head; block; block = block->next)
            for (i = 0; i < block->count; ++i)
            {
                s = block->samples + i;
                fprintf(f, "%.6f,%.6f,%.6f,%.6f,%.6f,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", s->time_ms, s->interval_ms,
                        s->simulation_ms, s->render_ms, s->present_ms, s->frame, s->produced, s->mode, s->paused,
                        s->commands, s->entities, s->projectiles, s->contacts, s->rebuilds);
            }
        fclose(f);

#ifdef DEBUGGING
        sprintf(text, "Frame timing saved: %s (%lu samples, %lu dropped)", a->perf_path, a->perf_capture.count,
                a->perf_capture.dropped);
#endif
        stage(a, text);
    }
    else
        stage(a, "Frame timing file could not be written");
    perf_dispose(&a->perf_capture);
}
#endif
static BHEntity **__cdecl marker_room_head(BHEntity *owner, void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    unsigned long i;
    for (i = 0; i < a->homing.response_count; ++i)
        if (a->world_responses[i].receiver == owner)
            return &a->rooms[i].pickups;
    return 0;
}
static int __cdecl refresh_segment_actors(BHWorldSegmentContext *q, void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    BHEntity *e;
    BHGeometry *geometry;
    BHSegmentQuery *query;
    unsigned long i, j, index;
    /* Follow the same authoritative ordered registry used by player/debris
      constructors, resolving meshes through their actual owning contexts. */
    for (i = 0; i < N(a->actors) && a->actors[i]; ++i)
    {
        if (i >= N(a->segment_actors))
            return 0;
        e = a->actors[i];
        index = 0;
        geometry = 0;
        if (e == a->player)
            geometry = &a->player_collision.geometry;
        else
            for (j = 0; j < N(a->target_slots); ++j)
                if (a->target_slots[j].body.entity == e && a->target_slots[j].body.generation == e->generation)
                {
                    index = j + 1;
                    geometry = &a->target_slots[j].collision.geometry;
                    break;
                }
        if (!geometry || !geometry->bsp)
            return 0;
        query = a->actor_queries + index;
        if (query->geometry != geometry || a->actor_query_entities[index] != e ||
            a->actor_query_generations[index] != e->generation)
        {
            if (query->frames && !bh_segment_query_dispose(query))
                return 0;
            if (!bh_segment_query_init(query, geometry))
                return 0;
            a->actor_query_generations[index] = e->generation;
            a->actor_query_entities[index] = e;
        }
        a->segment_actors[i].entity = e;
        a->segment_actors[i].query = query;
    }
    if (i == N(a->actors))
        return 0;
    q->actors = a->segment_actors;
    q->actor_count = i;
    return 1;
}
static int restore_display(BHGameApp *a)
{
    LPDIRECTDRAWSURFACE surfaces[2];
    HRESULT hr;
    unsigned long i;
    char text[128];
    surfaces[0] = a->primary;
    surfaces[1] = a->target;
    for (i = 0; i < 2; ++i)
        if (surfaces[i])
        {
            hr = IDirectDrawSurface_IsLost(surfaces[i]);
            if (hr == DDERR_SURFACELOST)
                hr = IDirectDrawSurface_Restore(surfaces[i]);
            if (FAILED(hr))
            {

#ifdef DEBUGGING
                sprintf(text, "Display surface recovery failed: surface=%lu HRESULT=%08lx", i, (unsigned long)hr);
#endif
                return failure(a, 13, text);
            }
        }
    return 1;
}
int __cdecl bh_game_assets_verify(const char *path, unsigned long *bytes, unsigned long *crc)
{
    FILE *file;
    unsigned char block[32768];
    unsigned long sum = 0xffffffffUL, total = 0, n, i, j;
    if (!path || (file = fopen(path, "rb")) == 0)
        return 0;
    while ((n = (unsigned long)fread(block, 1, sizeof(block), file)) != 0)
    {
        total += n;
        for (i = 0; i < n; ++i)
        {
            sum ^= block[i];
            for (j = 0; j < 8; ++j)
                sum = (sum >> 1) ^ ((0UL - (sum & 1)) & 0xedb88320UL);
        }
    }
    i = ferror(file);
    fclose(file);
    sum ^= 0xffffffffUL;
    if (bytes)
        *bytes = total;
    if (crc)
        *crc = sum;
    return !i && total == 13154037UL && sum == 0xe53a3d3fUL;
}
static BOOL WINAPI game_cursor(LPPOINT point)
{
    RECT client;
    if (!GetCursorPos(point) || !ScreenToClient(input_window, point) || !GetClientRect(input_window, &client)) return FALSE;
    point->x -= (client.right - client.left) / 2 - 160;
    point->y -= (client.bottom - client.top) / 2 - 100;
    return TRUE;
}
static BOOL WINAPI game_recenter(int x, int y)
{
    POINT p;
    RECT client;
    if (!GetClientRect(input_window, &client)) return FALSE;
    p.x = x + (client.right - client.left) / 2 - 160;
    p.y = y + (client.bottom - client.top) / 2 - 100;
    return ClientToScreen(input_window, &p) && SetCursorPos(p.x, p.y);
}
static HRESULT __cdecl loading_present(LPDIRECTDRAWSURFACE color, void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    RECT r;
    POINT p = {0, 0};
    GetClientRect(a->window, &r);
    ClientToScreen(a->window, &p);
    OffsetRect(&r, p.x, p.y);
    return IDirectDrawSurface_Blt(a->primary, &r, color, 0, DDBLT_WAIT, 0);
}
static int loading_begin(BHGameApp *a)
{
    if (!bh_loading_images_bind(&a->loading_images, a->width, 2, a->render_mode.engine.textures.display_right,
                                a->render_mode.engine.textures.display_left, 0) ||
        !bh_loading_images_init(&a->loading_images))
        return failure(a, 6, "Original loading images failed");
    a->loading_display.images = &a->loading_images;
    a->loading_display.color = a->target;
    a->loading_display.primary = a->primary;
    a->loading_display.back = a->render_mode.render;
    a->loading_display.width = a->width;
    a->loading_display.height = a->height;
    a->loading_display.scale = 1;
    a->loading_display.previous[0] = a->loading_display.previous[1] = -1;
    /* Fullscreen retains0044aac0's native Flip and alternating histories. */
    if (!a->fullscreen)
    {
        a->loading_display.present = loading_present;
        a->loading_display.present_user = a;
    }
    a->loading_progress.display = &a->loading_display;
    if (a->startup_check)
        return 1;
    if (!bh_loading_progress_install(&a->loading_progress))
        return failure(a, 6, "Loading presentation initialization failed");
    return 1;
}
static void __cdecl sdl_message(HWND window, UINT message, WPARAM key, LPARAM bits, void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    if (message == WM_ACTIVATEAPP && !key)
        a->focus_sample = 0;
    if (message == WM_SETCURSOR && LOWORD(bits) == HTCLIENT && !a->startup_check)
        SetCursor(NULL);
    if (message == WM_ACTIVATEAPP && !key)
        memset(a->snapshot.current, 0, sizeof(a->snapshot.current));
    if (message == MM_MCINOTIFY) {
        bh_cd_notify(&a->cd_notify, (short)key, (MCIDEVICEID)bits);
        return;
    }
    switch (message) {
    case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
    case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_RBUTTONDOWN: case WM_RBUTTONUP:
    case WM_MBUTTONDOWN: case WM_MBUTTONUP:
        bh_input_message(&a->snapshot, window, message, key, bits);
        break;
    }
}
static BHTextureImage *__cdecl image(unsigned long handle, void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    BHTextureImage *p;
    for (p = a->render_mode.engine.textures.images; p; p = p->next)
        if ((unsigned long)p == handle)
            return p;
    return 0;
}
#ifdef DEBUGGING
static int emission(BHGameApp *a, int result, BHClipVertex *ring, BHTextureImage *image)
{
    BHClipVertex *v = ring;
    unsigned long n = 0;
    if (!result && a->log)
    {
        fprintf(a->log, "Emission failure: primitive=%d raster=%d split=%d kernel=%lu image=%s\n", a->primitive.error,
                a->primitive.raster.error, a->primitive.split.error, a->primitive.selected_index,
                image ? image->name : "NULL");
        do
        {
            fprintf(a->log, "vertex %.9g %.9g z=%.9g uv=%.9g,%.9g\n", v->sx, v->sy, v->point.z, v->u, v->v);
            v = v->next;
        } while (v && v != ring && ++n < 12);
        fflush(a->log);
    }
    return result;
}
#else
#define emission(a, result, ring, image) (result)
#endif
static int __cdecl polygon(BHClipVertex *ring, BHTextureImage *texture, unsigned long color, void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    int result;
    a->primitive.raster.pixels = a->display.color_pixels;
    a->primitive.raster.pitch = a->display.color_pitch;
    a->primitive.raster.depth = (unsigned short *)a->display.auxiliary_pixels;
    a->primitive.raster.depth_pitch = a->display.auxiliary_pitch;
    result = bh_primitive_emit(&a->primitive, texture, ring, color);
    return emission(a, result, ring, texture);
}
/* 00455f70 dispatches ribbon vertices to 0046e430, whose shade comes from
   each vertex's scalar. The plain polygon callback would discard it. */
static int __cdecl scalar_polygon(BHClipVertex *ring, BHTextureImage *texture, unsigned long color, void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    int result;
    (void)color;
    a->primitive.raster.pixels = a->display.color_pixels;
    a->primitive.raster.pitch = a->display.color_pitch;
    a->primitive.raster.depth = (unsigned short *)a->display.auxiliary_pixels;
    a->primitive.raster.depth_pitch = a->display.auxiliary_pitch;
    result = bh_primitive_emit_scalar(&a->primitive, texture, ring);
    return emission(a, result, ring, texture);
}
static int __cdecl horizon_polygon(BHClipVertex *ring, BHTextureImage *texture, int gradient, const float rgb[3],
                                   void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    int result;
    unsigned long color = (unsigned long)rgb[0] * 0x10000UL + (unsigned long)rgb[1] * 0x100UL + (unsigned long)rgb[2];
    if (!gradient)
        return polygon(ring, texture, 0x80ffffffUL, user);
    a->primitive.raster.pixels = a->display.color_pixels;
    a->primitive.raster.pitch = a->display.color_pitch;
    a->primitive.raster.depth = (unsigned short *)a->display.auxiliary_pixels;
    a->primitive.raster.depth_pitch = a->display.auxiliary_pitch;
    result = bh_primitive_emit_gradient(&a->primitive, texture, ring, color);
    return emission(a, result, ring, texture);
}
static int platform(BHGameApp *a, int show)
{
    DDSURFACEDESC surface;
    BHSoftwareRaster raster;
    a->window = bh_sdl_open(a->startup_check, sdl_message, a);
    if (!a->window)
        return failure(a, 3, bh_sdl_error());
    input_window = a->window;
    strcpy(a->device_names[0].name, "SDL3 GPU");
    if (!bh_config_init(&a->config, 0) ||
        !bh_config_bootstrap(&a->config, a->config_path, a->device_names, 1))
        return failure(a, 6, "Config ownership initialization failed");
    a->application.config = &a->config;
    a->application.window = a->window;
    a->hardware = 1;
    a->fullscreen = 1; /* Internal flip path; SDL independently owns window mode. */
    if (DirectDrawCreate(0, &a->draw, 0) != DD_OK)
        return failure(a, 4, "SDL renderer adapter creation failed");
    memset(&surface, 0, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface.dwWidth = a->width;
    surface.dwHeight = a->height;
    surface.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (IDirectDraw_CreateSurface(a->draw, &surface, &a->primary, 0) != DD_OK)
        return failure(a, 4, "SDL presentation target creation failed");
    surface.ddsCaps.dwCaps = DDSCAPS_3DDEVICE;
    if (IDirectDraw_CreateSurface(a->draw, &surface, &a->target, 0) != DD_OK ||
        !bh_render_startup_hardware(&a->render_mode.engine, a->draw, a->target, a->primary,
                                    a->width, a->height, 16))
        return failure(a, 4, "SDL GPU renderer initialization failed");
    a->render_mode.screen = a->primary;
    a->render_mode.render = a->target;
    IDirectDrawSurface_AddRef(a->primary);
    IDirectDrawSurface_AddRef(a->target);
    if (a->hardware && !bh_texture_bind_display(&a->render_mode.engine.textures, &a->render_mode.engine.pixel))
        return failure(a, 4, "Hardware splash display format binding failed");
    if (!bh_view_config_init(&a->view, a->width, a->height) || !bh_main_camera_settings(&a->view, 60, .2, 25000) ||
        !bh_main_camera_pose(&a->view, &a->main_camera, 0, 0, 0, 0, 0, 0) ||
        !bh_clip_init(&a->clip, a->view.near_z, a->view.far_z, 0, (float)a->width, 0, (float)a->height) ||
        !bh_screen_clip_init(&a->screen_clip, a->view.near_z, a->view.far_z, 0, (float)a->width, 0, (float)a->height) ||
        !bh_attribute_clip_init(&a->attribute_clip, a->view.near_z, a->view.far_z, 0, (float)a->width, 0,
                                (float)a->height))
        return failure(a, 5, "Projection/clipping initialization failed");
    if (!bh_render_state_init(&a->render_state, &a->render_mode.engine.textures) ||
        !bh_horizon_color_init(&a->horizon_colors, &a->render_mode.engine.textures))
        return failure(a, 5, "Render-state ownership initialization failed");
    memset(&raster, 0, sizeof(raster));
    raster.width = a->width;
    raster.height = a->height;
    raster.near_z = a->view.near_z;
    raster.tables = &a->render_mode.engine.tables;
    raster.gradient = &a->horizon_colors;
    if (!bh_primitive_dispatch_init(&a->primitive, &a->render_state, &raster, 0, (float)a->width, 0, (float)a->height))
        return failure(a, 5, "Primitive dispatch initialization failed");
    a->prototype.camera = &a->main_camera;
    a->prototype.config = &a->view;
    a->prototype.clip = &a->clip;
    a->prototype.emit = polygon;
    a->prototype.emit_user = a;
    a->prototype.light_config = &a->light;
    a->prototype.lights = &a->lights;
    a->prototype.light_reference = &a->zero;
    a->prototype.texture_mask = 0xffffffffUL;
    a->prototype.base_color = 0xff000000UL;
    if (!bh_model_context_init(&a->models, &a->render_mode.engine.textures, 0) ||
        !bh_model_path_set(&a->models, "bsp", 0) ||
        !bh_render_resources_init(&a->resources, &a->models, &a->prototype, 0) ||
        !bh_atlas_context_init(&a->atlas, &a->render_mode.engine.textures, 320))
        return failure(a, 5, "Model/atlas initialization failed");
    a->billboard.camera = &a->main_camera;
    a->billboard.config = &a->view;
    a->billboard.clip = &a->screen_clip;
    a->billboard.emit = polygon;
    a->billboard.emit_user = a;
    a->oriented.path = a->path;
    a->oriented.capacity = N(a->path);
    a->oriented.clip = &a->attribute_clip;
    a->oriented.emit = scalar_polygon;
    a->oriented.emit_user = a;
    a->oriented.near_z = a->view.near_z;
    a->oriented.projection_scale = a->main_camera.focal;
    a->sprite_quad.camera = &a->main_camera;
    a->sprite_quad.clip = &a->clip;
    a->sprite_quad.directional = &a->light;
    a->sprite_quad.world_lights = &a->lights;
    a->sprite_quad.emit = polygon;
    a->sprite_quad.emit_user = a;
    a->screen_quad.view = &a->view;
    a->screen_quad.clip = &a->screen_clip;
    a->screen_quad.emit = polygon;
    a->screen_quad.emit_user = a;
    a->pickup_draw.queue = &a->queue;
    a->pickup_draw.camera = &a->main_camera;
    a->pickup_draw.view = &a->view;
    a->pickup_draw.clip = &a->screen_clip;
    a->pickup_draw.emit = polygon;
    a->pickup_draw.emit_user = a;
    a->pickup_draw.display_depth = a->width; /* Legacy field name: original004e2998 is viewport width, not bit depth. */
    a->resource_draw.camera = &a->main_camera;
    a->resource_draw.config = &a->view;
    a->resource_draw.billboard = &a->billboard;
    a->resource_draw.oriented = &a->oriented;
    a->model_submit.draw = &a->resource_draw;
    a->model_submit.lookup = bh_render_resources_lookup;
    a->model_submit.lookup_user = &a->resources;
    a->billboard_submit.draw = &a->billboard;
    a->billboard_submit.lookup = image;
    a->billboard_submit.lookup_user = a;
    a->screen_submit.draw = &a->screen_quad;
    a->screen_submit.lookup = image;
    a->screen_submit.lookup_user = a;
    a->quad_submit.draw = &a->sprite_quad;
    a->quad_submit.lookup = image;
    a->quad_submit.lookup_user = a;
    a->path_submit.view = &a->view;
    a->path_submit.camera = &a->main_camera;
    a->path_submit.draw = &a->oriented;
    a->path_submit.lookup = image;
    a->path_submit.lookup_user = a;
    a->segment_submit.camera = &a->main_camera;
    a->segment_submit.draw = &a->oriented;
    a->segment_submit.lookup = image;
    a->segment_submit.lookup_user = a;
    a->controls.view = &a->view;
    a->controls.camera = &a->main_camera;
    a->controls.portal_cache = &a->portal_cache;
    a->controls.resources = &a->resources;
    a->controls.model_clip = &a->clip;
    a->controls.screen_clip = &a->screen_clip;
    a->controls.attribute_clip = &a->attribute_clip;
    a->horizon.color_mode = a->render_mode.engine.bytes_per_pixel;
    a->horizon.camera = &a->main_camera;
    a->horizon.clip = &a->attribute_clip;
    a->horizon.state = &a->render_state;
    a->horizon.colors = &a->horizon_colors;
    a->horizon.emit = horizon_polygon;
    a->horizon.user = a;
    a->horizon_submit.draw = &a->horizon;
    a->horizon_submit.lookup = image;
    a->horizon_submit.lookup_user = a;
    a->flare_submit.camera = &a->main_camera;
    a->flare_submit.view = &a->view;
    a->flare_submit.width = a->width;
    a->flare_submit.height = a->height;
    a->flare_submit.screen = &a->screen_submit;
    a->display.textures = &a->render_mode.engine.textures;
    a->display.atlas = &a->atlas;
    a->display.color = a->target;
    a->display.auxiliary = a->render_mode.engine.depth;
    a->display.auxiliary_bytes = a->width * a->height * 2;
    a->fade.render = &a->render_state;
    a->fade.width = a->width;
    a->fade.height = a->height;
    a->fade.pixel_bytes = 2;
    a->fade.near_plane = a->view.near_z;
    a->fade.masks[0] = 0xf800;
    a->fade.masks[1] = 0x7e0;
    a->fade.masks[2] = 31;
    a->fade.extract[0] = 11;
    a->fade.extract[1] = 5;
    a->fade.expand[0] = a->fade.expand[2] = 3;
    a->fade.expand[1] = 2;
    a->fade.cached_amount = -1;
    a->present.fade = &a->fade;
    a->present.color = a->target;
    a->present.auxiliary = a->render_mode.engine.depth;
    a->begin.present = &a->present;
    a->gdi.display = &a->display;
    a->render.display = &a->display;
    a->render.text = &a->text;
    a->render.atlas = &a->atlas;
    a->render.draw = &a->queue;
    a->render.view = &a->view;
    a->render.camera = &a->main_camera;
    a->render.state = &a->render_state;
    a->render.horizon = &a->horizon_submit;
    a->render.image_lookup = image;
    a->render.image_user = a;
    a->render.model = &a->model_submit;
    a->render.billboard = &a->billboard_submit;
    a->render.controls = &a->controls;
    a->render.screen = &a->screen_submit;
    a->render.rect = &a->pickup_draw;
    a->render.plane = &a->quad_submit;
    a->render.path = &a->path_submit;
    a->render.segment = &a->segment_submit;
    a->render.fade = &a->fade;
    a->render.flare = &a->flare_submit;
    a->render.light = &a->light;
    a->render.gdi = &a->gdi;
    a->special_draw.queue = &a->queue;
    if (!a->startup_check)
    {
        (void)show;
        UpdateWindow(a->window);
    }
    stage(a, a->hardware     ? "Fullscreen Direct3D2 HAL renderer ready"
             : a->fullscreen ? "Fullscreen restored software renderer ready"
                             : "Native window and restored software renderer ready");
    return 1;
}
static void binding(BHGameApp *a, unsigned long identity, unsigned long *destination)
{
    BHBootstrapBinding *b;
    if (a->binding_count >= N(a->bindings))
    {
        a->error = 1;
        return;
    }
    b = a->bindings + a->binding_count++;
    b->identity = identity;
    b->destination = destination;
}
#define B(id, field) binding(a, id, (unsigned long *)&a->field)
static void resource_bindings(BHGameApp *a)
{
    B(0x4cdb88, player_tick.resources.menu_cue);
    B(0x496570, player_tick.resources.ui_cue);
    B(0x4cda14, player_tick.resources.no_weapon);
    B(0x4d1ab0, player_tick.resources.damage_warning);
    B(0x4d0474, player_tick.resources.hull_warning);
    B(0x4f554c, player_tick.resources.collision_side);
    B(0x4f550c, player_tick.resources.electro);
    B(0x4cda20, player_tick.resources.target_cue);
    B(0x4cda10, player_tick.resources.lock_cue);
    B(0x4f555c, player_tick.resources.ground_loop);
    B(0x4f5514, player_tick.resources.death_loop);
    B(0x4f5540, player_tick.resources.death_cue);
    B(0x4d195c, player_tick.resources.death_model);
    B(0x4cda18, player_tick.resources.cheat_model);
    B(0x4cdc98, player_tick.resources.boost_start);
    B(0x4d1904, player_tick.resources.boost_loop);
    B(0x4cdc74, player_tick.resources.boost_end);
    B(0x4f0028, player_tick.resources.smoke_image);
    B(0x4f0090, player_tick.resources.second_smoke);
    B(0x4f00a8, player_tick.resources.water_cloud);
    B(0x4f0058, player_tick.resources.water_ring);
    B(0x4f5544, player_create.engine_loop);
    B(0x4d1a6c, menu.move_sound);
    B(0x4d1a6c, menu.open_sound);
    B(0x4d1a70, menu.slide_sound);
    B(0x4d1a68, menu.press_sound);
    B(0x4d1a6c, menu.release_sound);
    B(0x4f00a0, flash.textures[2]);
    B(0x4f0098, flash.textures[0]);
    B(0x4f009c, flash.textures[1]);
    B(0x4f00a4, flash.textures[3]);
    B(0x4f0030, explosions.image);
    B(0x4f54f8, explosions.sound_resource);
    B(0x4df23c, debris.image);
    B(0x4f00bc, contacts.spark_texture);
    B(0x4f0088, contacts.quad_texture);
    B(0x4f00a8, contacts.line_texture);
    B(0x4f0058, contacts.rays_texture);
    B(0x4f0084, hit_markers.image);
    B(0x4d1aa0, player_scene.material_image);
    B(0x4f005c, player_scene.horizon_image);
    B(0x4f0040, player_scene.aim_image);
    B(0x4f00d0, player_scene.aim_overlay_image);
    B(0x4f54d8, player_scene.particle_image);
    B(0x4962f4, player_scene.confirmation_image);
    B(0x4f00cc, player_scene.trail_image);
    B(0x4f002c, player_scene.overlay_image);
    B(0x496338, player_scene.cockpit_resource);
    B(0x4f005c, flyby.horizon_image);
    B(0x4d4424, flyby.full_image);
    B(0x4f0068, flyby.corner_left_image);
    B(0x4f0080, flyby.corner_right_image);
    B(0x4d1960, flyby.scenario_resources[0]);
    B(0x4d1974, flyby.scenario_resources[1]);
    B(0x4d1978, flyby.scenario_resources[2]);
    B(0x4d1a98, background.first_model);
    B(0x4df238, background.second_model);
    B(0x4f0078, background.first_flare);
    B(0x4f007c, background.second_flare);
    B(0x4f0028, emitters.fountain_texture);
    B(0x4f00a8, emitters.steam_texture);
    B(0x4f00c0, hud.background_image);
    B(0x4f54e4, world_hud.atlas_image);
    B(0x4f0054, world_hud.weapon_icons_image);
    B(0x4f54e4, flat_hud.atlas_image);
    B(0x4f0054, flat_hud.weapon_icons_image);
    B(0x4f00b4, markers.brackets_image);
    B(0x4f54e4, pickup_draw.atlas_image);
    B(0x4f00b0, pickup_draw.glow_image);
    B(0x4cdb9c, markers.status_image);
    B(0x4f00b8, attachment.default_image);
    B(0x4f006c, attachment.flag200_image);
    B(0x4f0070, attachment.flag400_image);
    B(0x4f0074, attachment.flag800_image);
    B(0x4f54dc, radar.marker_image);
    B(0x4f00b4, radar.bar_image);
    B(0x4f00ac, projectile_draw.ballistic_flare);
    B(0x4f007c, projectile_draw.ballistic_trail);
    B(0x4f0094, projectile_draw.homing_flare);
    B(0x4f003c, projectile_draw.homing_trail);
    B(0x4f0034, projectile_draw.pulse_image);
    B(0x4f5548, physics_effects.impact_sound);
    B(0x4f54fc, physics_effects.quad_sound);
    B(0x4f54fc, physics_effects.rays_sound);
    B(0x4f5560, pickups.take_sound);
    B(0x4d0478, wagon.model);
    B(0x4d1a64, wagon.sound_resource);
    a->bootstrap.bindings = a->bindings;
    a->bootstrap.binding_count = a->binding_count;
}
#undef B
static int world_ready(BHGameApp *a);
static int player_ready(BHGameApp *a);
static int resources_ready(BHGameApp *a);
static int runtime_owners(BHGameApp *a);
static int __cdecl compose(BHGameBootstrap *bootstrap, unsigned long phase, void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    (void)bootstrap;
    switch (phase)
    {
    case 1:
        stage(a, "Building scene/query registries");
        return world_ready(a);
    case 2:
        stage(a, "Binding live player/physics/scene");
        return player_ready(a);
    case 3:
        stage(a, "Binding constructed menus");
        return 1;
    case 5:
        a->menu_present.frame_resource = bh_bootstrap_value(&a->bootstrap, 0x496304);
        a->flyby_camera.credit_font = a->bootstrap.menu_font;
        return a->flyby_camera.credit_font != 0;
    case 4:
        stage(a, "Binding loaded resource consumers");
        return resources_ready(a);
    default:
        return failure(a, 1, "Unknown composition phase");
    }
}
static int source_owners(BHGameApp *a)
{
    unsigned long i;
    a->actor_route.live_tick = &a->player_state.tick;
#ifdef DEBUGGING
    a->actor_route.nonfinite_curve = curve_diagnostic;
    a->actor_route.nonfinite_user = a;
#endif
    a->vehicle_draw.clock = &a->actor_route;
    a->script_draw.clock = &a->actor_route;
    if (!bh_player_input_owner_init(&a->inputs, 0) ||
        !bh_input_collection_init(&a->collection, &a->snapshot, &a->inputs) ||
        !bh_input_devices_bind(&a->devices, &a->collection, 0))
        return failure(a, 6, "Input ownership initialization failed");
    a->profile = a->config.profiles;
    a->collection.disable_mouse_motion = 1;
    a->collection.api.cursor = game_cursor;
    a->collection.api.recenter = game_recenter;
    if (!a->startup_check && !bh_input_devices_start(&a->devices, a->window))
        return failure(a, 6, "Native input device initialization failed");
    if (!bh_entities_init(&a->pool, 0x2580, 0) || !bh_sound_queue_init(&a->sound, 0, 0, 0) ||
        !bh_network_transport_init(&a->transport, 0, 0) || !bh_network_game_init(&a->network_game, &a->sessions, 0) ||
        !bh_provider_catalog_init(&a->providers, 0) || !bh_rail_paths_init(&a->rails, 0) ||
        !bh_navigation_init(&a->navigation, 0))
        return failure(a, 6, "Entity/audio/navigation ownership initialization failed");
    bh_text_queue_init(&a->text);
    if (!bh_draw_queue_init(&a->queue, 3500, 0) || !bh_frame_buffers_bind(&a->frames, 0) ||
        !bh_script_particles_init(&a->particles, &a->pool, &a->queue) ||
        !bh_contact_effects_init(&a->contacts, &a->pool, &a->queue, 0, 0, 0, 0) ||
        !bh_collision_model_init(&a->player_collision, 0) || !bh_weapon_factory_open(&a->weapons, &a->pool, &a->sound))
        return failure(a, 6, "Draw/physics/player ownership initialization failed");
    a->sounds.queue = &a->sound;
    a->provider_startup.catalog = &a->providers;
    a->provider_startup.modems = &a->modems;
    a->force_start.force = &a->force;
    a->force_init.start = &a->force_start;
    a->force_update.force = &a->force;
    a->cd.last_track = -1;
    a->cd_notify.cd = &a->cd;
    a->cd_audio.notify = &a->cd_notify;
    a->cd_audio.wave_device = (HWAVEOUT)-1;
    a->cd_audio.music_path = "MUSIC_DATA\\track02.wav";
    a->media.audio = &a->cd_audio;
    bh_message_state_init(&a->messages);
    a->messages.panel_entity = 0;
    a->panel.pool = &a->pool;
    a->panel.messages = &a->messages;
    a->panel.text = &a->text;
    a->panel.sound = &a->sound;
    a->panel.draw = &a->queue;
    a->panel.screen_width = a->width;
    a->panel.near_depth = a->view.near_z;
    a->lifetime.timers.pool = &a->pool;
    a->lifetime.timers.actors = a->timer_actors;
    a->lifetime.sounds = &a->sound;
    a->motion.lifetime = &a->lifetime;
    a->sensors.lifetime = &a->lifetime;
    a->sensors.radar = a->actors;
    a->sensors.radar_count = N(a->actors);
    a->emitters.lifetime = &a->lifetime;
    a->emitters.particles = &a->particles;
    a->flash.pool = &a->pool;
    a->flash.queue = &a->queue;
    a->debris.pool = &a->pool;
    a->debris.queue = &a->queue;
    a->explosions.pool = &a->pool;
    a->explosions.queue = &a->queue;
    a->explosions.sound = &a->sound;
    a->explosions.debris = &a->debris;
    a->explosions.flash = &a->flash;
    a->hit_markers.pool = &a->pool;
    a->hit_markers.queue = &a->queue;
    a->pickups.pool = &a->pool;
    a->pickups.resources = a->pickup_resources;
    a->pickups.resource_count = N(a->pickup_resources);
    a->pickups.outside_actors = a->actors;
    a->pickups.sound = &a->sound;
    a->pickup_render.default_draw = &a->pickup_draw;
    a->pickup_render.special_draw = &a->special_draw;
    if (!bh_pickup_render_bind(&a->pickups, &a->pickup_render))
        return failure(a, 6, "Pickup renderer binding failed");
    a->vm.lifetime = &a->lifetime;
    a->vm.motion = &a->motion;
    a->vm.sensors = &a->sensors;
    a->vm.emitters = &a->emitters;
    a->vm.explosions = &a->explosions;
    a->vm.contacts = &a->contacts;
    a->vm.flash = &a->flash;
    a->vm.script_actors = &a->script_actors;
    a->vm.targets = &a->targets;
    a->vm.pickups = &a->pickups;
    a->vm.launch = &a->actor_launch;
    a->vm.rails = &a->wagon;
    a->vm.radar = &a->radar_query;
    a->vm.actors = a->vm_actors;
    a->vm.waypoints = a->vm_starts;
    a->vm.waypoint_count = GAME_STARTS;
    a->parser.vm = &a->vm;
    a->parser.sounds = &a->sounds;
    a->parser.script_prefix = "bsp\\";
    a->parser.sound_prefix = "sound\\";
    if (!bh_script_blocks_init(&a->blocks, &a->parser, &a->resources, "bsp\\", BH_BSP_PREFIX_FIRST))
        return failure(a, 6, "Block library initialization failed");
    a->editor_world.pool = &a->pool;
    a->editor_world.library = &a->blocks;
    a->editor_world.queue = &a->queue;
    a->editor_world.sound = &a->sound;
    a->editor_world.nodes = a->nodes;
    a->editor_world.capacity = GAME_WORLD_CAPACITY;
    a->editor_world.input = &a->editor_state;
    a->editor_world.numeric = &a->numeric;
    a->editor_world.browser = &a->browser;
    a->editor_world.mode = 1;
    a->editor.world = &a->editor_world;
    a->editor.step_limit = 65535;
    a->numeric.selected = -1;
    a->browser.library = &a->blocks;
    a->browser.queue = &a->queue;
    a->browser.sound = &a->sound;
    a->browser.height = a->height;
    a->browser.selected = -1;
    a->world_load.editor = &a->editor;
    a->world_load.vm = &a->vm;
    a->world_load.rails = &a->rails;
    a->world_load.states = a->timer_actors;
    a->world_load.actor_capacity = GAME_WORLD_CAPACITY;
    a->editor_input.owner = &a->inputs;
    a->editor_input.pool = &a->pool;
    a->editor_input.state = &a->editor_state;
    a->editor_input.name = "";
    a->basis.samples = a->samples;
    a->basis.sample_capacity = N(a->samples);
    a->basis.groups_a = a->basis_a;
    a->basis.groups_b = a->basis_b;
    a->basis.indices_a = a->indices_a;
    a->basis.indices_b = a->indices_b;
    a->basis.scales_a = a->scales_a;
    a->basis.scales_b = a->scales_b;
    a->basis.a_capacity = a->basis.b_capacity = 7;
    a->player_force.basis = &a->basis;
    a->player_force.body = &a->physics_bodies[0];
    a->player_force.instability_counter = &a->instability;
    a->player_state.force = bh_player_force_control_b;
    a->physics_bodies[0].physics = &a->player_motion;
    a->physics_bodies[0].target_world = &a->player_motion.pose;
    a->physics_bodies[0].target_local = &a->player_inverse;
    a->player_inverse.m[0][0] = a->player_inverse.m[1][1] = a->player_inverse.m[2][2] = 1;
    a->weapon_dispatch.factory = &a->weapons;
    a->weapon_dispatch.ready = &a->weapon_ready;
    a->weapon_dispatch.target = &a->weapon_target;
    a->weapon_dispatch.arc = &a->weapon_arc;
    a->weapon_dispatch.empty = &a->weapon_empty;
    a->weapon_dispatch.fire = &a->weapon_fire;
    a->weapon_dispatch.pulse = &a->weapon_pulse;
    a->weapon_dispatch.effects = &a->weapon_effect;
    a->weapon_dispatch.pulse_launch = &a->weapon_launch;
    a->weapon_dispatch.arc_fire = &a->weapon_arc_fire;
    a->weapon_dispatch.arc_hud = &a->arc_hud;
    a->weapon_dispatch.target_hud = &a->target_hud;
    a->weapon_ready.sound = &a->sound;
    a->weapon_empty.sound = &a->sound;
    a->weapon_empty.messages = &a->messages;
    a->weapon_empty.warning_seen = &a->weapon_warning;
    a->player_camera.starts = a->starts;
    a->player_camera.start_count = GAME_STARTS;
    a->player_camera.camera = &a->camera_state;
    a->correction.query = &a->segment;
    a->player_reset.current_player = &a->player;
    a->player_reset.weapons = &a->weapons.registry;
    a->player_reset.sound = &a->sound;
    a->player_reset.frame_count = &a->player_state.death_frames;
    a->player_tick.pool = &a->pool;
    a->player_tick.state = &a->player_state;
    a->player_tick.physics = &a->physics;
    a->player_tick.body = &a->physics_bodies[0];
    a->player_tick.force = &a->player_force;
    a->player_tick.decoded = &a->decoded;
    a->player_tick.snapshot = &a->snapshot;
    a->player_tick.weapons = &a->weapon_dispatch;
    a->player_tick.camera = &a->player_camera;
    a->player_tick.reset = &a->player_reset;
    a->player_tick.scene = &a->player_scene;
    a->player_tick.panel = &a->panel;
    a->player_tick.world_reset = &a->world_reset;
    a->player_tick.menu = &a->menu;
    a->player_tick.media = &a->media;
    a->player_tick.sound = &a->sound;
    a->player_tick.world = &a->segment;
    a->player_tick.lifetime = &a->lifetime;
    a->player_tick.particles = &a->particles;
    a->player_tick.contacts = &a->contacts;
    a->player_tick.flash = &a->flash;
    a->player_tick.explosions = &a->explosions;
    a->player_tick.feedback = &a->force_update;
    a->player_tick.game = &a->network_game;
    a->player_tick.navigation = &a->navigation;
    a->player_tick.latency = a->latency;
    a->player_tick.camera_yaw = &a->yaw;
    a->player_tick.damage_source = &a->damage_source;
    a->player_create.tick = &a->player_tick;
    a->player_create.query = &a->player_query;
    a->player_create.current = &a->player;
    a->player_create.input_owner = &a->inputs;
    a->player_create.profile = a->profile;
    a->player_create.collision = &a->player_collision;
    a->player_create.correction = &a->correction;
    a->player_create.actors = a->actors;
    a->player_create.actor_capacity = N(a->actors);
    a->player_query.weapons = &a->weapons.registry;
    a->player_query.sound = &a->sound;
    a->player_query.messages = &a->messages;
    a->player_query.ready = &a->weapon_ready;
    a->player_query.target = &a->weapon_target;
    a->player_query.arc = &a->weapon_arc;
    a->player_query.lifetime = &a->lifetime;
    a->player_query.transport = &a->transport;
    a->player_query.actors = &a->vehicle_lifetime.head;
    a->player_query.fine_warning = &a->fine_warning;
    a->player_query.damage_source = &a->damage_source;
    a->player_query.mode_count = &a->player_state.mode;
    a->player_query.simulation_tick = &a->player_state.tick;
    a->player_query.world = &a->query_world;
    a->player_query.world_index = &a->query_world_index;
    a->player_query.sound_velocity = &a->zero;
    a->world_reset.vm = &a->vm;
    a->world_reset.loot_rows = a->availability;
    a->physics_effects.effects = &a->contacts;
    a->physics_effects.sound = &a->sound;
    a->physics_effects.transport = &a->transport;
    a->menu.pool = &a->pool;
    a->menu.snapshot = &a->snapshot;
    a->menu.sound = &a->sound;
    a->menu.sessions = &a->sessions;
    a->menu.provider = &a->selected_provider;
    a->menu.transition = &a->transition;
    a->menu.flyby = &a->flyby;
    a->menu.mode = &a->player_state.mode;
    a->menu.paused = &a->player_state.paused;
    a->menu.waypoint = &a->selected_waypoint;
    a->menu.input_changed = &a->input_changed;
    a->menu.stay = &a->stay;
    a->menu_settings.input_profile = a->profile;
    a->menu_settings.network_records = (BHMenuNetworkRecord *)a->providers.records;
    a->menu_settings.network_record_count = a->providers.count;
    a->menu_actions.devices = a->device_names;
    a->menu_actions.device_count = (unsigned long)a->display_catalog.secondary_count + 1;
    a->menu_actions.config = &a->config;
    a->menu_actions.save_path = a->config_path;
    a->menu_actions.settings = &a->menu_settings;
    a->menu_actions.profiles = &a->menu_profiles;
    a->menu_actions.text = &a->text_capture;
    a->menu_actions.media = &a->media;
    a->menu_actions.modems = &a->modems;
    a->menu_actions.providers = &a->providers;
    a->menu_actions.sessions = &a->sessions;
    a->menu_actions.game = &a->network_game;
    a->menu_actions.root = &a->root_menu;
    a->menu_actions.messages = &a->messages;
    a->menu_actions.sound = &a->sound;
    a->menu_actions.player_name = a->bootstrap.player_name;
    a->menu_actions.mode = &a->player_state.mode;
    a->menu_actions.network_failed = &a->network_failed;
    a->menu_actions.frame_count = &a->player_state.death_frames;
    a->menu_profiles.config = &a->config;
    a->menu_profiles.current = &a->profile;
    a->menu_profiles.pending_selection = &a->pending_profile;
    a->menu_edits.config = &a->config;
    a->menu_edits.settings = &a->menu_settings;
    a->menu_edits.audio = &a->cd_audio;
    a->menu_edits.sessions = &a->sessions;
    a->menu_edits.capture = &a->key_capture;
    a->menu_edits.mode = &a->player_state.mode;
    a->menu_edits.mission = &a->mission;
    a->menu_edits.ship = &a->ship;
    a->transition.pool = &a->pool;
    a->transition.draw = &a->queue;
    a->transition.media = &a->media;
    a->transition.messages = &a->messages;
    a->transition.config = &a->config;
    a->transition.config_path = a->config_path;
    a->transition.hardware_flags = 1;
    a->transition.camera_index = &a->selected_waypoint;
    a->transition.quit = &a->quit;
    a->transition.window = (DWORD)a->window;
    a->key_capture.pool = &a->pool;
    a->key_capture.snapshot = &a->snapshot;
    a->key_capture.state = &a->global_state;
    a->key_capture.settings = &a->menu_settings;
    a->key_capture.sound = &a->sound;
    a->key_capture.result = a->key_result;
    a->text_capture.buffer = a->text_buffer;
    a->text_capture.capacity = sizeof(a->text_buffer);
    a->text_capture.pool = &a->pool;
    a->text_capture.snapshot = &a->snapshot;
    a->text_capture.state = &a->global_state;
    a->text_capture.settings = &a->menu_settings;
    a->text_capture.profile = &a->menu_profiles;
    a->text_capture.sound = &a->sound;
    a->flyby.entities = &a->pool;
    a->flyby.current_player = &a->player;
    a->flyby.queue = &a->queue;
    a->flyby.sound = &a->sound;
    a->flyby.background = &a->background;
    a->flyby.scene = &a->scene;
    a->flyby.input_owner = &a->inputs;
    a->flyby.input_profile = a->profile;
    a->flyby.player_camera = &a->player_camera;
    a->flyby.flyby_camera = &a->flyby_camera;
    a->flyby.flyby_state = &a->flyby_state;
    a->flyby.starts = a->starts;
    a->flyby.start_count = GAME_STARTS;
    a->flyby.mode_count = &a->player_state.mode;
    a->flyby.selected_waypoint = &a->selected_waypoint;
    a->flyby.scenario = &a->ship;
    a->flyby.player_slot = &a->mission;
    a->flyby.display_width = &a->width;
    a->flyby.display_height = &a->height;
    a->flyby.near_z = &a->view.near_z;
    a->flyby.event_value = &a->event_value;
    a->flyby.mission_identity = &a->mission_identity;
    a->flyby_camera.route = &a->camera_route;
    a->flyby_camera.camera = &a->camera_state;
    a->flyby_camera.starts = a->starts;
    a->flyby_camera.start_count = GAME_STARTS;
    a->flyby_camera.text = &a->text;
    a->flyby_camera.credits = &a->credits;
    a->flyby_construct.scene = &a->flyby;
    a->flyby_construct.current_camera = &a->camera_entity;
    a->camera_route.navigation = &a->navigation;
    a->camera_route.waypoints = a->route_starts;
    a->camera_route.waypoint_count = GAME_STARTS;
    a->selected_waypoint = 10;
    a->mission_progress.config = &a->config;
    a->mission_progress.path = a->config_path;
    a->mission_progress.lifetime = &a->lifetime;
    a->story.progress = &a->mission_progress;
    a->story.messages = &a->messages;
    a->story.weapons = &a->weapons.registry;
    a->story.actors = &a->vehicle_lifetime.head;
    a->story.actor_limit = 9600;
    a->story.availability = a->availability;
    a->story.sound = &a->sound;
    a->story.force_control = &a->player_state.force;
    a->story.force_gate = &a->stay;
    a->story.ending = &a->player_state.ending;
    a->bootstrap.diagnostic_box = a->box_level;
    a->bootstrap.textures = &a->render_mode.engine.textures;
    a->bootstrap.resources = &a->resources;
    a->bootstrap.sounds = &a->sounds;
    a->bootstrap.atlas = &a->atlas;
    a->bootstrap.text = &a->text;
    a->bootstrap.blocks = &a->blocks;
    a->bootstrap.world = &a->world_load;
    a->bootstrap.editor = &a->editor_input;
    a->bootstrap.navigation = &a->navigation;
    a->bootstrap.player = &a->player_create;
    a->bootstrap.story = &a->story;
    a->bootstrap.availability = a->availability;
    a->bootstrap.inputs = &a->inputs;
    a->bootstrap.pool = &a->pool;
    a->bootstrap.snapshot = &a->snapshot;
    a->bootstrap.config = &a->config;
    a->bootstrap.providers = &a->providers;
    a->bootstrap.modems = &a->modems;
    a->bootstrap.flyby = &a->flyby_construct;
    a->bootstrap.runtime = &a->menu;
    a->bootstrap.actions = &a->menu_actions;
    a->bootstrap.edits = &a->menu_edits;
    a->bootstrap.panel = &a->panel;
    a->bootstrap.transition = &a->transition;
    a->bootstrap.present = &a->menu_present;
    a->bootstrap.compose = compose;
    a->bootstrap.compose_user = a;
    a->menu_present.queue = &a->queue;
    a->menu_present.eye = &a->flyby_state.camera.position;
    a->menu_present.angles = &a->flyby_state.camera.angles;
    resource_bindings(a);
    for (i = 0; i < 8; ++i)
        a->background.slots[i].elapsed = -1;
    a->background.queue = &a->queue;
    return 1;
}
/* Placement metadata starts with BHWorldGeometry, but it is never a
   BHEntity. Resolve each relationship through the owning block/node table. */
static int world_ready(BHGameApp *a)
{
    unsigned long i, n = a->editor_world.count;
    BHEditorNode *node;
    BHScriptBlock *block;
    if (!n || n > GAME_WORLD_CAPACITY || a->blocks.count > N(a->block_queries))
        return failure(a, 7, "Invalid constructed world/block counts");
    a->motion_bindings = (BHScriptMotionBinding *)calloc(n, sizeof(*a->motion_bindings));
    a->motion.bindings = a->motion_bindings;
    a->motion.binding_count = n;
    a->rooms = (BHPickupRoom *)calloc(n, sizeof(*a->rooms));
    a->scheduled = (BHEntityScheduleRoom *)calloc(n, sizeof(*a->scheduled));
    a->scene_inputs = (BHSceneBindingInput *)calloc(n, sizeof(*a->scene_inputs));
    a->segment_rooms = (BHWorldSegmentRoom *)calloc(n, sizeof(*a->segment_rooms));
    a->physics_worlds = (BHPhysicsStepWorld *)calloc(n, sizeof(*a->physics_worlds));
    a->reset_nodes = (BHWorldResetNode *)calloc(n, sizeof(*a->reset_nodes));
    if (!a->motion_bindings || !a->rooms || !a->scheduled || !a->scene_inputs || !a->segment_rooms ||
        !a->physics_worlds || !a->reset_nodes)
        return failure(a, 7, "World registry allocation failed");
    for (i = 0; i < a->blocks.count; ++i)
        if (a->blocks.blocks[i].collision.geometry.bsp &&
            !bh_segment_query_init(a->block_queries + i, &a->blocks.blocks[i].collision.geometry))
            return failure(a, 7, "Block collision query initialization failed");
    for (i = 0; i < n; ++i)
    {
        node = a->nodes[i];
        if (!node || node->model_index >= a->blocks.count)
            return failure(a, 7, "Invalid placement owner");
        block = a->blocks.blocks + node->model_index;
        a->motion_bindings[i].entity = node->entity;
        a->motion_bindings[i].world = &node->placement.geometry;
        a->motion_bindings[i].draw_angles = &a->scheduled[i].angles;
        a->motion_bindings[i].attachments_owner = &a->rooms[i].pickups;
        a->scene_inputs[i].world = &node->placement.geometry;
        a->scene_inputs[i].resource = block->resource;
        a->scene_inputs[i].resource_flags = block->definition.flags;
        a->scene_inputs[i].angles = node->placement.angles;
        a->segment_rooms[i].world = &node->placement.geometry;
        a->segment_rooms[i].actors_owner = &a->rooms[i].actors;
        a->segment_rooms[i].query = block->collision.geometry.bsp ? a->block_queries + node->model_index : 0;
        a->segment_rooms[i].kind = node->entity->kind;
        a->physics_worlds[i].world = &node->placement.geometry;
        memcpy(&a->physics_worlds[i].mass, (char *)node->entity + 0xb4, 4);
        a->physics_worlds[i].response = node->entity;
        a->physics_worlds[i].index = (unsigned short)i;
        a->reset_nodes[i].entity = node->entity;
        a->reset_nodes[i].placement = &node->placement;
        a->reset_nodes[i].definition = &block->definition;
        a->reset_nodes[i].draw_angles = &a->scheduled[i].angles;
    }
    a->scene_view = a->view;
    a->schedule.queue = &a->queue;
    a->schedule.viewport = &a->scene_view;
    a->schedule.camera = &a->scene.camera;
    a->schedule.max_rooms = n;
    a->schedule.max_entities = a->pool.capacity;
    a->schedule.max_depth = n;
    if (!bh_scene_bindings_init(&a->scene_bindings, a->scene_inputs, n, a->rooms, a->scheduled, &a->resources,
                                &a->schedule) ||
        !bh_scene_owner_init(&a->scene, &a->scene_bindings, a->width, a->height) ||
        !bh_world_segment_init(&a->segment, &a->nodes[0]->placement.geometry, n, a->segment_rooms, n, 0, 0,
                               a->pool.capacity))
        return failure(a, 7, "Scene/query topology binding failed");
    a->segment.refresh_actors = refresh_segment_actors;
    a->segment.refresh_user = a;
    for (i = 0; i < n; ++i)
    {
        a->timer_actors[i].active = 0;
        a->scheduled[i].activity = &a->timer_actors[i].active;
        a->scheduled[i].activity_entity = a->nodes[i]->entity;
    }
    a->world_reset.nodes = a->reset_nodes;
    a->world_reset.count = n;
    a->camera_route.first = a->segment.first;
    a->camera_route.scene_count = n;
    a->player_tick.rooms = a->rooms;
    a->player_tick.room_count = n;
    a->pickups.rooms = a->rooms;
    a->pickups.room_count = n;
    a->pickups.first_world = a->segment.first;
    a->pickups.world_count = n;
    for (i = 0; i < GAME_STARTS; ++i)
    {
        a->starts[i].position = a->vm_starts[i].position;
        a->starts[i].angles = a->vm_starts[i].angles;
        memcpy(a->starts[i].metadata, &a->vm_starts[i].speed, 8);
        a->route_starts[i] = a->vm_starts[i].position;
    }
    return 1;
}
static int player_ready(BHGameApp *a)
{
    if (a->actor_route.live_tick != &a->player_state.tick || a->vehicle_draw.clock != &a->actor_route ||
        a->script_draw.clock != &a->actor_route)
        return failure(a, 8, "Actor consumers do not share the live simulation clock");
    if (a->player_query.contact_physics != &a->player_motion)
        return failure(a, 8, "Car contact callback is not bound to live physics");
    if (!a->player || a->physics_bodies[0].entity != a->player ||
        a->physics_bodies[0].geometry != &a->player_collision.geometry ||
        bh_player_tick_context(a->player) != &a->player_tick)
        return failure(a, 8, "Live player ownership mismatch");
    a->base_models[0] = &a->player_collision.geometry;
    a->physics_bodies[0].model_index = 0;
    if (!bh_physics_step_init(&a->physics, &a->pool, &a->physics_bodies[0], 1, a->physics_worlds, a->editor_world.count,
                              a->base_models, 1, &a->physics_effects, 0, 0))
        return failure(a, 8, "Live player physics initialization failed");
    a->schedule.overlay_position = (const BHVec3 *)((char *)a->player + 0x160);
    a->player_scene.player = a->player;
    a->player_scene.queue = &a->queue;
    a->player_scene.sound = &a->sound;
    a->player_scene.hud = &a->hud;
    a->player_scene.markers = &a->markers;
    a->player_scene.radar = &a->radar;
    a->player_scene.world_status = &a->world_hud;
    a->player_scene.flat_status = &a->flat_hud;
    a->player_scene.score_draw = &a->score_draw;
    a->player_scene.scores = &a->scores;
    a->player_scene.background = &a->background;
    a->player_scene.scene = &a->scene;
    a->player_scene.weapons = &a->weapons.registry;
    a->player_scene.arc = &a->arc_hud;
    a->player_scene.target = &a->target_hud;
    a->player_scene.actor_head = &a->vehicle_lifetime.head;
    a->player_scene.pickup_head = &a->pickups.head;
    a->player_scene.mode_count = &a->player_state.mode;
    a->player_scene.frame_count = &a->player_state.death_frames;
    a->player_scene.display_width = &a->width;
    a->player_scene.display_height = &a->height;
    a->player_scene.confirmation_gate = &a->player_state.confirmation;
    a->player_scene.pickup_limit = a->pool.capacity;
    a->attachment.queue = &a->queue;
    a->attachment.frame = &a->radar_frame;
    a->attachment.viewport_width = a->width;
    a->hud.attachment = &a->attachment;
    a->hud.frame = &a->radar_frame;
    a->radar.queue = &a->queue;
    a->radar.frame = &a->radar_frame;
    a->radar_frame.near_z = a->view.near_z;
    a->markers.projection = &a->pickup_draw;
    a->markers.radar = &a->radar;
    a->markers.display_width = a->width;
    a->markers.limit = a->pool.capacity;
    a->world_hud.hud = &a->hud;
    return 1;
}
static int resources_ready(BHGameApp *a)
{
    unsigned long i;
    if (!runtime_owners(a))
        return 0;
    /* Independent original call-site keys and shared globals:00441d80,
      00430a30,00433b30;004dfb28/34/38. Check the actual application wiring. */
    if (a->area.hit_sound != bh_bootstrap_value(&a->bootstrap, 0x4f54f8) ||
        a->weapon_fire.homing_sound != bh_bootstrap_value(&a->bootstrap, 0x4d1894) ||
        a->weapon_fire.empty_sound != bh_bootstrap_value(&a->bootstrap, 0x4cda10))
        return failure(a, 9, "Original weapon cue binding mismatch");
    if (a->projectiles.next_identity != &a->debris.origin_tag || a->area.identity != &a->debris.origin_tag ||
        a->pulse.identity != &a->debris.origin_tag || a->effect.debris != &a->debris ||
        a->effect.pending_identity != a->pulse.sync_identity || a->pulse.sync_identity != &a->sync_identity)
        return failure(a, 9, "Original projectile shared identity binding mismatch");
    if (a->weapon_target.actors_owner != &a->vehicle_lifetime.head ||
        a->weapon_arc.head_owner != &a->vehicle_lifetime.head ||
        a->pursuit.actor_head_owner != &a->vehicle_lifetime.head)
        return failure(a, 9, "Original live target list binding mismatch");
    if (!image(a->player_scene.particle_image, a) ||
        a->player_scene.particle_image != bh_bootstrap_value(&a->bootstrap, 0x4f54d8) ||
        a->oriented.emit != scalar_polygon || a->oriented.projection_scale != a->main_camera.focal ||
        a->main_camera.focal <= 1)
        return failure(a, 9, "Player boost/ribbon resource or projection binding missing");
    /* Every destination must receive the value published by the bootstrap;
      identities stay keys and are never interpreted as source pointers. */
    for (i = 0; i < a->binding_count; ++i)
        if (*a->bindings[i].destination != bh_bootstrap_value(&a->bootstrap, a->bindings[i].identity))
            return failure(a, 9, "Resource consumer binding mismatch");
    if (a->box_level)
        a->camera_entity = a->player;
    if (a->bootstrap.menus_created != BH_BOOTSTRAP_MENUS || !a->player || !a->camera_entity ||
        a->physics.pool != &a->pool || !bh_entities_valid(&a->pool))
        return failure(a, 9, "Constructed application ownership check failed");
    return 1;
}
#define V(id) bh_bootstrap_value(&a->bootstrap, id)
static int runtime_owners(BHGameApp *a)
{
    unsigned long i, j, n = a->editor_world.count;
    long city;
    BHHomingProjectileContext *h = &a->projectile_effects;
    a->world_responses = (BHHomingWorldResponse *)calloc(n, sizeof(*a->world_responses));
    a->pulse_indices = (BHPulseWorldIndex *)calloc(n, sizeof(*a->pulse_indices));
    a->actor_rooms = (BHActorConstructRoom *)calloc(n, sizeof(*a->actor_rooms));
    a->city_ids = (long *)calloc(n, sizeof(*a->city_ids));
    if (!a->world_responses || !a->pulse_indices || !a->actor_rooms || !a->city_ids)
        return failure(a, 9, "Actor response registry allocation failed");
    for (i = 0; i < n; ++i)
    {
        a->world_responses[i].world = a->rooms[i].world;
        a->world_responses[i].receiver = a->nodes[i]->entity;
        a->pulse_indices[i].world = a->rooms[i].world;
        a->pulse_indices[i].index = (unsigned short)i;
        a->actor_rooms[i].room = a->rooms + i;
        memcpy(&city, (char *)a->nodes[i]->entity + 0x11c, 4);
        a->actor_rooms[i].bound_resource = (unsigned long)city;
        a->vm_actors[i].damage.world = a->rooms[i].world;
        a->vm_actors[i].damage.player = a->player;
    }
    for (i = 0; i < a->navigation.node_count; ++i)
    {
        for (j = 0; j < n; ++j)
            if (a->rooms[j].world == a->navigation.nodes[i].placement)
                break;
        if (j == n)
            return failure(a, 9, "AI navigation city owner missing");
        memcpy(a->city_ids + i, (char *)a->nodes[j]->entity + 0x11c, 4);
    }
    for (i = 0; i < N(a->actor_models); ++i)
    {
        a->actor_models[i] = V(0x4d1920 + i * 4);
        a->actor_sounds[i] = V(0x4d1a00 + i * 4);
    }
    for (i = 0; i < N(a->pickup_resources); ++i)
        a->pickup_resources[i] = V(0x4d19a0 + i * 4);
    a->sensors.player = a->player;
    a->story.camera = bh_player_camera_state(a->player);
    a->story.pickup_sound = V(0x4f5560);
    a->flat_hud.hud = &a->hud;
    a->flat_hud.viewport_height = a->height;
    a->arc_hud.queue = &a->queue;
    a->arc_hud.image = V(0x49632c);
    a->target_hud.projection = &a->pickup_draw;
    a->target_hud.viewport_width = a->width;
    a->target_hud.atlas_image = V(0x4f00b4);
    a->score_draw.page_image = V(0x4cdb9c);
    a->score_text.queue = &a->text;
    a->score_text.page = a->bootstrap.menu_font;
    a->scores.remote = a->remote;
    a->scores.local_name = a->bootstrap.player_name;
    a->score_draw.queue = &a->queue;
    a->score_draw.display_width = a->width;
    a->score_draw.near_z = a->view.near_z;
    a->projectile_lifetime.pool = &a->pool;
    a->projectile_lifetime.sound = &a->sound;
    a->projectile_draw.queue = &a->queue;
    h->pool = &a->pool;
    h->segment = &a->segment;
    h->sound = &a->sound;
    h->particles = &a->particles;
    h->contacts = &a->contacts;
    h->flash = &a->flash;
    h->markers = &a->hit_markers;
    h->responses = a->world_responses;
    h->response_count = n;
    h->hit_sound = V(0x4f5564);
    h->liquid_sound = V(0x4f54fc);
    h->first_image = V(0x4f0090);
    h->second_image = V(0x4f0030);
    a->hit_markers.owner_head = marker_room_head;
    a->hit_markers.owner_head_user = a;
    a->ballistic.bindings = *h;
    a->ballistic.ricochet_sound = V(0x4f550c);
    a->projectiles.lifetime = &a->projectile_lifetime;
    a->projectiles.ballistic = &a->ballistic;
    a->projectiles.homing = &a->homing;
    a->projectiles.draw = &a->projectile_draw;
    a->projectiles.next_identity = &a->debris.origin_tag;
    a->projectiles.loop_sound = V(0x4f5504);
    a->projectiles.launch_sound = V(0x4f5510);
    a->area.effects = h;
    a->area.draw = &a->projectile_draw;
    a->area.identity = &a->debris.origin_tag;
    a->area.loop_sound = V(0x4962f0);
    a->area.hit_sound = V(0x4f54f8);
    a->weapon_fire.projectiles = &a->projectiles;
    a->weapon_fire.area = &a->area;
    a->weapon_fire.sound = &a->sound;
    a->weapon_fire.transport = &a->transport;
    a->weapon_fire.messages = &a->messages;
    a->weapon_fire.empty_warning = &a->empty_warning;
    a->weapon_fire.local = a->player;
    a->weapon_fire.ballistic_sound = V(0x4d0480);
    a->weapon_fire.homing_sound = V(0x4d1894);
    a->weapon_fire.empty_sound = V(0x4cda10);
    a->weapon_fire.area_sound = V(0x4ce448);
    a->weapon_pulse.effects = *h;
    a->weapon_pulse.debris = &a->debris;
    a->weapon_pulse.transport = &a->transport;
    a->weapon_pulse.shot_sound = V(0x4d0480);
    a->weapon_pulse.smoke_image = V(0x4f0028);
    a->weapon_pulse.flare_image = V(0x4f0030);
    a->weapon_pulse.trail_image = V(0x4f0060);
    a->homing = *h;
    a->homing.hit_sound = V(0x4f550c);
    a->homing.first_image = V(0x4f003c);
    a->homing.second_image = V(0x4f0094);
    a->weapon_ready.sound_resource = V(0x4cda10);
    a->weapon_empty.shift_sound = V(0x4cda10);
    a->weapon_target.sound = &a->sound;
    a->weapon_target.world = &a->segment;
    a->weapon_target.max_actors = a->pool.capacity;
    a->weapon_target.ready_sound = V(0x4cda20);
    a->weapon_target.beep_sound = V(0x4cdb88);
    a->weapon_target.lock_sound = V(0x4cdb8c);
    a->weapon_arc.sound = &a->sound;
    a->weapon_arc.world = &a->segment;
    a->weapon_arc.limit = a->pool.capacity;
    a->weapon_effect.fire = &a->weapon_fire;
    a->weapon_effect.projectile = &a->effect;
    a->weapon_effect.split_model = V(0x4cdc70);
    a->weapon_effect.shot_sound = V(0x4d1894);
    a->weapon_launch.fire = &a->weapon_fire;
    a->weapon_launch.projectile = &a->pulse;
    a->weapon_launch.sound = V(0x4cda20);
    a->weapon_arc_fire.fire = &a->weapon_fire;
    a->weapon_arc_fire.particles = &a->particles;
    a->weapon_arc_fire.flash = &a->flash;
    a->weapon_arc_fire.first_image = V(0x496334);
    a->weapon_arc_fire.second_image = V(0x4f0094);
    a->weapon_arc_fire.sound = V(0x4ce448);
    a->effect.effects = h;
    a->effect.cleanup.pool = &a->pool;
    a->effect.cleanup.sound = &a->sound;
    a->effect.debris = &a->debris;
    a->effect.explosion = &a->explosions;
    a->effect.draw = &a->effect_draw;
    a->effect.trails = &a->trails;
    a->effect.pending_identity = &a->sync_identity;
    a->effect.orbit = &a->orbit;
    a->effect.accelerated = &a->accelerated;
    a->effect.split = &a->split;
    a->effect.network = &a->effect_network;
    a->orbit.core = &a->effect;
    a->accelerated.puffs = &a->puffs;
    a->puffs.queue = &a->queue;
    a->effect_draw.queue = &a->queue;
    a->trails.pool = &a->pool;
    a->trails.queue = &a->queue;
    a->effect_network.transport = &a->transport;
    a->pulse.effects = h;
    a->pulse.lifetime = &a->projectile_lifetime;
    a->pulse.draw = &a->pulse_draw;
    a->pulse.debris = &a->debris;
    a->pulse.explosion = &a->explosions;
    a->pulse.transport = &a->transport;
    a->pulse.world_indices = a->pulse_indices;
    a->pulse.world_count = n;
    a->pulse.identity = &a->debris.origin_tag;
    a->pulse.sync_identity = &a->sync_identity;
    a->pulse_draw.queue = &a->queue;
    if (!bh_debris_context_init(&a->debris_response, &a->pool, &a->sound, V(0x496218), a->actors, N(a->actors)))
        return failure(a, 9, "Dynamic body registry binding failed");
    a->target_draw.queue = &a->queue;
    a->target_draw.camera = &a->main_camera;
    a->target_draw.view = &a->view;
    a->target_draw.player = a->player;
    a->target_draw.screen_width = a->width;
    a->target_draw.atlas_image = V(0x4f54e4);
    a->target_draw.marker_image = V(0x4f00b4);
    a->targets.debris = &a->debris_response;
    a->targets.physics = &a->physics;
    a->targets.effects = &a->physics_effects;
    a->targets.explosions = &a->explosions;
    a->targets.particles = &a->particles;
    a->targets.pickups = &a->pickups;
    a->targets.draw = &a->target_draw;
    a->targets.mode = &a->editor_world.mode;
    a->targets.smoke_image = V(0x4f0028);
    a->targets.slots = a->target_slots;
    a->targets.capacity = GAME_DYNAMIC_CAPACITY;
    a->targets.roster = a->physics_bodies;
    a->targets.models = a->physics_models;
    a->targets.base_bodies = a->physics_bodies;
    a->targets.base_count = 1;
    a->targets.base_models = a->base_models;
    a->targets.base_model_count = 1;
    a->targets.worlds = a->physics_worlds;
    a->targets.world_count = n;
    a->targets.resources = a->pickup_resources;
    a->targets.resource_count = N(a->pickup_resources);
    a->targets.prefix = "bsp\\";
    a->targets.search = BH_BSP_PREFIX_FIRST;
    a->actor_route.rooms = a->rooms;
    a->actor_route.room_count = n;
    a->actor_route.actor_limit = a->pool.capacity;
    a->actor_route.sound = &a->sound;
    a->vehicle_lifetime.pool = &a->pool;
    a->vehicle_lifetime.route = &a->actor_route;
    a->actor_response.damage = &a->actor_damage;
    a->actor_response.attack = &a->actor_attack;
    a->actor_response.vehicles = &a->vehicle_lifetime;
    a->actor_response.mission = &a->lifetime;
    a->actor_response.player = a->player;
    /* Original wreck angles are unwritten stack data. Use an explicit stable
      zero orientation adapter for that death-only undefined input. */
    a->actor_damage.legacy_death_angles = &a->zero;
    a->actor_damage.pool = &a->pool;
    a->actor_damage.route = &a->actor_route;
    a->actor_damage.particles = &a->particles;
    a->actor_damage.fragments = &a->debris;
    a->actor_damage.targets = &a->targets;
    a->actor_damage.loot_rows = a->availability;
    a->actor_damage.loot_row_count = 4;
    a->actor_damage.smoke_resource = V(0x4f0028);
    a->actor_damage.death_sound = V(0x4f5540);
    a->actor_response.mission_resource = V(0x4d1918);
    a->actor_attack.pursuit = &a->pursuit;
    a->actor_attack.effects = &a->effect;
    a->actor_attack.projectiles = &a->projectiles;
    a->actor_attack.area = &a->area;
    a->pursuit.route = &a->actor_route;
    a->pursuit.navigation = &a->navigation;
    a->pursuit.player = a->player;
    a->pursuit.actor_limit = a->pool.capacity;
    a->pursuit.city_ids = a->city_ids;
    a->pursuit.messages = &a->messages;
    a->weapon_target.actors_owner = &a->vehicle_lifetime.head;
    a->weapon_arc.head_owner = &a->vehicle_lifetime.head;
    a->pursuit.actor_head_owner = &a->vehicle_lifetime.head;
    a->pursuit.city_names = city_names;
    a->pursuit.city_count = N(city_names);
    a->vehicle_draw.queue = &a->queue;
    a->vehicle_draw.sound = &a->sound;
    a->vehicle_draw.damage_resource = V(0x4f0028);
    a->actor_construct.response = &a->actor_response;
    a->actor_construct.draw = &a->vehicle_draw;
    a->actor_construct.first = a->segment.first;
    a->actor_construct.world_count = n;
    a->actor_construct.rooms = a->actor_rooms;
    a->actor_construct.room_count = n;
    a->actor_construct.models = a->actor_models;
    a->actor_construct.sounds = a->actor_sounds;
    a->actor_construct.resource_count = N(a->actor_models);
    a->actor_construct.cycle_models = a->cycle_models;
    a->actor_construct.cycle_count = N(a->cycle_models);
    a->actor_launch.actors = &a->actor_construct;
    a->actor_launch.transport = &a->transport;
    a->actor_launch.navigation = &a->navigation;
    a->script_actors.first_world = a->segment.first;
    a->script_actors.world_count = n;
    a->script_actors.callbacks.pool = &a->pool;
    a->script_actors.callbacks.route = &a->actor_route;
    a->script_actors.callbacks.draw = &a->script_draw;
    a->script_draw.queue = &a->queue;
    a->script_draw.particles = &a->particles;
    a->script_actors.model_resources[0] = V(0x4d1890);
    a->script_actors.model_resources[1] = V(0x4d19f8);
    a->script_actors.model_resources[2] = V(0x4d18e4);
    a->script_actors.model_resources[3] = V(0x4961ec);
    a->script_actors.sound_resource = V(0x4d1a64);
    a->radar_query.pool = &a->pool;
    a->radar_query.lifetime = &a->lifetime;
    a->script_actors.callbacks.pulse_resource = V(0x4d441c);
    a->wagon.rails = &a->rails;
    a->wagon.pool = &a->pool;
    a->wagon.first_world = a->segment.first;
    a->wagon.world_count = n;
    a->wagon.rooms = a->rooms;
    a->wagon.room_count = n;
    a->wagon.queue = &a->queue;
    a->wagon.sound = &a->sound;
    a->player_query.contact_side = V(0x4f554c);
    a->player_query.contact_axial = V(0x4f5558);
    a->player_query.primary_cue = V(0x4961e8);
    a->player_query.secondary_cue = V(0x4962fc);
    a->menu_settings.network_records = (BHMenuNetworkRecord *)a->providers.records;
    a->menu_settings.network_record_count = a->providers.count;
    /* These keys come from the consuming owners' retained original call sites,
      not from the load-table order (sound handle zero is a valid resource). */
    a->effect.blast_sound = V(0x4f5548);
    a->effect.impact_sound = V(0x496218);
    a->effect.ricochet_sound = V(0x4f550c);
    a->effect.loop_sound = V(0x4f5504);
    a->effect.second_loop_sound = V(0x49633c);
    a->effect.smoke_image = V(0x4f0028);
    a->effect.trail_image = V(0x4f004c);
    a->effect_draw.directional_model = V(0x496340);
    a->effect_draw.fixed_model = V(0x496300);
    a->effect_draw.timed_texture = V(0x4f0094);
    a->effect_draw.random_texture = V(0x4f008c);
    a->effect_draw.fixed_texture = V(0x4f00b0);
    a->accelerated.ring_image = V(0x4f0050);
    a->accelerated.second_ring_image = V(0x4f0064);
    a->accelerated.random_image = V(0x4f0090);
    a->accelerated.interval_sound = V(0x4df234);
    a->accelerated.expiry_sound = V(0x4ce448);
    a->split.split_sound = V(0x496218);
    a->weapon_arc.activate_sound = V(0x4cda20);
    a->weapon_arc.release_sound = V(0x4cdc74);
    a->weapon_arc.charge16_sound = V(0x4cdc98);
    a->weapon_arc.charge51_sound = V(0x4d1904);
    a->weapon_effect.shot_sound = V(0x4cda20);
    a->projectiles.loop_sound = V(0x4f5510);
    a->projectiles.launch_sound = V(0x4f550c);
    a->projectile_draw.ballistic_flare = V(0x4f00ac);
    a->projectile_draw.ballistic_trail = V(0x4f007c);
    a->projectile_draw.homing_flare = V(0x4f0094);
    a->projectile_draw.homing_trail = V(0x4f003c);
    a->projectile_draw.pulse_image = V(0x4f0034);
    a->pulse.loop_sound = V(0x4df234);
    a->pulse.bounce_sound = V(0x4f5558);
    a->pulse_draw.model = V(0x4d0470);
    a->pulse_draw.image = V(0x4f0034);
    for (i = 0; i < N(a->cycle_models); ++i)
        a->cycle_models[i] = V(0x496578 + i * 4);
    a->actor_construct.special_model = V(0x4d047c);
    a->vehicle_draw.glowing_model = V(0x4d1940);
    a->vehicle_draw.glow_first = V(0x4f0078);
    a->vehicle_draw.glow_second = V(0x4f008c);
    a->script_draw.smoke_resource = V(0x4f0028);
    a->script_draw.glowing_model = V(0x4d18e4);
    a->script_draw.glow_first = V(0x4f008c);
    a->script_draw.glow_second = V(0x4f00b0);
    a->actor_attack.orbit_cue = V(0x496be0);
    a->actor_attack.area_sound = V(0x4ce448);
    a->actor_attack.ballistic_sound = V(0x4d0480);
    a->actor_attack.homing_sound = V(0x4d1894);
    a->actor_attack.impact_sound = V(0x4f5564);
    a->actor_attack.liquid_sound = V(0x4f54fc);
    a->actor_attack.split_model = V(0x4cdc70);
    a->actor_attack.smoke_image = V(0x4f0028);
    a->actor_attack.impact_image = V(0x4f0090);
    a->actor_attack.flash_image = V(0x4f0030);
    a->actor_attack.beam_image = V(0x4f0060);
    a->panel.flash_image = (unsigned long)bh_atlas_page_image(a->panel.page);
    a->panel.alert_image = V(0x4f00b4);
    /* 004225ce: ordinary completion; 004225a3: chapter progression. */
    a->mission_progress.first_sound = V(0x496bdc);
    a->mission_progress.unlocked_sound = V(0x4d18e0);
    return 1;
}
#undef V
static LRESULT __cdecl source_message(void *user, HWND w, UINT m, WPARAM key, LPARAM bits)
{
    BHGameApp *a = (BHGameApp *)user;
    return bh_input_message(&a->snapshot, w, m, key, bits);
}
#ifdef DEBUGGING
#include "bh_game_box.inc"
#endif
static void __cdecl source_produce(void *user)
{
    BHGameApp *a = (BHGameApp *)user;
    unsigned long clock = a->frame + (a->player_state.paused == 0);
    bh_mp_frame(clock);
    if (!bh_entities_focus(&a->pool, a->player_state.paused ? a->player : 0))
    {
        failure(a, 12, "Paused player entity focus binding failed");
        return;
    }
    /* The service increments the original frame clock before entity callbacks. */
    a->pursuit.clock = a->vehicle_draw.animation_clock = (long)clock;
    a->script_draw.animation_time = (float)clock;
    a->service.packet = a->bootstrap.all_keys->state;
    a->service.packet_bytes = sizeof(a->bootstrap.all_keys->state);
    if (!bh_frame_service_tick(&a->service))
    {
        char text[512];
        sprintf(
            text,
            "Source frame producer failure: service=%d sound=%d hresult=%08lx native=%08lx player=%d query=%d weapon=%d method=%08lx target=%d arc=%d pulse=%d fire=%d tick=%lu",
            a->service.error, a->sound.error, (unsigned long)a->sound.last_hresult,
            (unsigned long)a->service.sound_stats.last_error, a->player_tick.error, a->player_query.error,
            a->weapon_dispatch.error, a->weapon_dispatch.identity, a->weapon_target.error, a->weapon_arc.error,
            a->weapon_pulse.error, a->weapon_fire.error, a->box_ticks);
        failure(a, 12, text);
    }
#ifdef DEBUGGING
    if (a->service.sound_stats.last_error && !a->sound_diagnostic_logged)
    {
        char text[256];
        sprintf(
            text,
            "Native sound diagnostic (original frame continues): hresult=%08lx frequency_failures=%lu resource=%lu slot=%lu frequency=%ld tick=%lu",
            (unsigned long)a->service.sound_stats.last_error, a->service.sound_stats.frequency_failures,
            a->service.sound_stats.failed_resource, a->service.sound_stats.failed_slot,
            a->service.sound_stats.failed_frequency, a->box_ticks);
        stage(a, text);
        a->sound_diagnostic_logged = 1;
    }
#endif
#ifdef DEBUGGING
    if (a->service.error == BH_FRAME_SERVICE_ENTITY)
    {
        char text[512];
        sprintf(
            text,
            "Entity traversal failure: frame=%lu step=%d refresh=%d count=%lu phase=%s current=%08lx callback=%08lx next=%08lx generation=%lu->%lu active=%d vm=%d timer=%d",
            a->frame, a->service.step_error, a->service.refresh_error, a->pool.failure_count,
            a->pool.failure_refresh ? "refresh" : "step", (unsigned long)a->pool.failure_current,
            a->pool.failure_callback, (unsigned long)a->pool.failure_next, a->pool.failure_expected_generation,
            a->pool.failure_actual_generation, a->pool.failure_next_active, a->vm.error, a->lifetime.timers.error);
        stage(a, text);
    }
#endif
}
static int render_frame(BHGameApp *a, BHDrawQueue *queue)
{
    int ok;
    BHDrawQueue *saved;
#ifdef DEBUGGING
    DWORD began = GetTickCount();
#endif
    if (!restore_display(a))
        return 0;
    if (!bh_present_begin(&a->begin))
        return failure(a, 13, "Present begin failed");
    a->display.color_pixels = a->fade.pixels;
    a->display.color_pitch = a->fade.pitch;
    a->display.auxiliary_pixels = a->begin.auxiliary_pixels;
    a->display.auxiliary_pitch = a->begin.auxiliary_pitch;
    a->display.locked = a->present.locked;
    saved = a->render.draw;
    a->render.draw = queue;
    ok = bh_render_execute(&a->render);
    if (queue == &a->interpolation.queue) bh_interpolation_rendered(&a->interpolation);
    a->render.draw = saved;
#ifdef DEBUGGING
    if (a->render.failures && a->render.error == BH_RENDER_EXECUTE_CALLBACK && !a->draw_warning_logged)
    {
        char text[384];
        sprintf(
            text,
            "Source draw diagnostic (original callback loop continues): failures=%lu resource=%lu draw=%d pose=%.9g,%.9g,%.9g/%.9g,%.9g,%.9g alpha=%.9g",
            a->render.failures, a->render.failed_resource, a->render.failed_draw_error, a->render.failed_pose[0],
            a->render.failed_pose[1], a->render.failed_pose[2], a->render.failed_pose[3], a->render.failed_pose[4],
            a->render.failed_pose[5], a->render.failed_pose[6]);
        stage(a, text);
        a->draw_warning_logged = 1;
    }
#endif
    a->fade.pixels = a->display.color_pixels;
    a->fade.pitch = a->display.color_pitch;
    a->present.locked = a->display.locked;
    if (!bh_present_end(&a->present))
        ok = 0;
    a->display.locked = a->present.locked;
#ifdef DEBUGGING
    if (a->startup_check || (!a->perf_enabled && a->frame % 30 == 0))
    {
        char profile[256];
        sprintf(
            profile,
            "Render profile: ms=%lu commands=%lu polygons=%lu texture_locks=%lu writes=%lu mouse=%ld,%ld filtered=%.5g,%.5g",
            GetTickCount() - began, queue->count, a->primitive.polygons, a->primitive.locks,
            a->primitive.raster.written, a->collection.delta[0], a->collection.delta[1], a->snapshot.mouse_axes[0],
            a->snapshot.mouse_axes[1]);
        stage(a, profile);
    }
#endif
    if (!ok)
    {
        char text[512];
        sprintf(
            text,
            "Source renderer failed: execute=%d index=%lu kind=%lu resource=%08lx status=%d first_draw=%d first_model=%d first_clip=%d pose=%.9g,%.9g,%.9g/%.9g,%.9g,%.9g alpha=%.9g resource_draw=%d primitive=%d display=%d HRESULT=%08lx",
            a->render.error, a->render.failed_index, a->render.failed_kind, a->render.failed_resource,
            a->render.failed_status, a->render.failed_draw_error, a->render.failed_model_error,
            a->render.failed_clip_error, a->render.failed_pose[0], a->render.failed_pose[1], a->render.failed_pose[2],
            a->render.failed_pose[3], a->render.failed_pose[4], a->render.failed_pose[5], a->render.failed_pose[6],
            a->resource_draw.error, a->primitive.error, a->display.error, (unsigned long)a->render.last_hresult);
        return failure(a, 13, text);
    }
    return 1;
}
#ifdef DEBUGGING
static int startup_perf_checks(BHGameApp *a)
{
    BHGamePerfCapture capture;
    BHGamePerfBlock *b;
    BHGamePerfSample *s;
    unsigned long i, n = 0, current[7] = {100, 100, 2, 3, 4, 5, 6};
    int ok = 1;
    memset(&capture, 0, sizeof(capture));
    for (i = 0; i < 40001; ++i)
    {
        s = perf_append(&capture);
        if (!s)
        {
            ok = 0;
            break;
        }
        s->frame = i;
    }
    for (b = capture.head; b; b = b->next)
        for (i = 0; i < b->count; ++i)
            if (b->samples[i].frame != n++)
                ok = 0;
    if (n != 40001 || capture.count != 40001 || capture.dropped)
        ok = 0;
    if (perf_contact_delta(&capture, current) != 220)
        ok = 0;
    current[0] = 1;
    current[1] = 101;
    if (perf_contact_delta(&capture, current) != 2 || perf_contact_delta(&capture, current) != 0)
        ok = 0;
    perf_dispose(&capture);
    if (capture.head || capture.tail || capture.count)
        ok = 0;
    if (!ok)
        return failure(a, 13, "Static frame capture/reset-counter checks failed");
    stage(a, "Static frame capture: 40001 ordered samples, block cleanup and independent contact resets passed");
    return 1;
}
#endif
static int startup_clock_checks(BHGameApp *a)
{
    BHSimulationClock clock;
    unsigned long rate, i, count;
    static const unsigned long rates[5] = {30, 60, 75, 144, 240};
    if (a->mission_progress.first_sound != bh_bootstrap_value(&a->bootstrap, 0x496bdc) ||
        a->mission_progress.unlocked_sound != bh_bootstrap_value(&a->bootstrap, 0x4d18e0))
        return failure(a, 13, "Mission completion sound bindings reversed");
    for (rate = 0; rate < N(rates); ++rate) {
        memset(&clock, 0, sizeof(clock)); count = 0;
        for (i = 0; i <= rates[rate] * 10; ++i)
            count += bh_simulation_advance(&clock, 100000.0 + i / (double)rates[rate]);
        if (count != 301)
            return failure(a, 13, "Producer clock depends on presentation refresh rate");
    }
    memset(&clock, 0, sizeof(clock));
    if (bh_simulation_advance(&clock, 0) != 1 || bh_simulation_advance(&clock, 10) > 8 ||
        bh_simulation_advance(&clock, 10) != 0)
        return failure(a, 13, "Producer clock accumulated a stall backlog");
    stage(a, "SDL producer clock: exact 30 Hz at 30/60/75/144/240 presentations, bounded stall catch-up passed");
    return 1;
}
static int capture_fixture(BHGameApp *a, const char *name)
{
#ifdef DEBUGGING
    char directory[MAX_PATH], path[MAX_PATH + 80];
    if (GetEnvironmentVariableA("BH_RENDER_CAPTURE_DIR", directory, sizeof(directory))) {
        sprintf(path, "%s\\%s.png", directory, name);
        if (!bh_sdl_capture(path))
            return failure(a, 13, "SDL GPU fixture capture failed");
    }
#else
    (void)a; (void)name;
#endif
    return 1;
}
static BHBootstrapMenu *diagnostic_menu(BHGameApp *a, unsigned long identity)
{
    unsigned long i;
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
        if (a->bootstrap.menus[i].identity == identity) return a->bootstrap.menus + i;
    return 0;
}
static int diagnostic_press(BHBootstrapMenu *m, unsigned long key)
{
    memset(&m->runtime.input_state, 0, sizeof(m->runtime.input_state));
    m->runtime.input->state[1] = 0;
    m->runtime.distance = 4.5f;
    m->runtime.exiting = 0;
    m->runtime.input_state.values[key] = 1;
    return bh_menu_runtime_step(&m->runtime);
}
static int startup_menu_navigation(BHGameApp *a)
{
    BHBootstrapMenu *main = diagnostic_menu(a, 0x495e40), *options = diagnostic_menu(a, 0x4cdb90),
                    *video = diagnostic_menu(a, 0x496220);
    unsigned long i, before, arena = a->messages.arena_used;
    char first[64];
    if (!main || !options || !video) return failure(a, 13, "Menu diagnostic identity missing");
    main->settings.selection = 0;
    for (i = 0; i < main->settings.row_count; ++i) {
        before = main->settings.selection;
        if (!diagnostic_press(main, 1) || main->settings.selection != (long)((before + 1) % main->settings.row_count))
            return failure(a, 13, "Main menu Down/wrap navigation failed");
    }
    main->settings.selection = 2;
    if (!diagnostic_press(main, 4) || main->runtime.destinations[2] != &options->runtime ||
        options->runtime.input->flags != 2 || options->runtime.distance != 100)
        return failure(a, 13, "Main menu Setup activation failed");
    options->settings.selection = 0;
    if (!diagnostic_press(options, 4) || options->runtime.destinations[0] != &video->runtime || video->runtime.input->flags != 2)
        return failure(a, 13, "Options Video activation failed");
    strcpy(first, bh_sdl_video_label(0));
    video->settings.selection = 0;
    if (!diagnostic_press(video, 4) || !strcmp(first, bh_sdl_video_label(0)))
        return failure(a, 13, "Video window-mode action failed");
    if (!diagnostic_press(video, 4) || strcmp(first, bh_sdl_video_label(0)))
        return failure(a, 13, "Video window-mode round trip failed");
    video->settings.selection = 2;
    if (!diagnostic_press(video, 4) || video->actions.error || video->edits.error)
        return failure(a, 13, "Video resolution action/label refresh failed");
    if (!diagnostic_press(video, 8) || video->runtime.destinations[video->settings.selection] != &options->runtime ||
        a->messages.arena_used != arena)
        return failure(a, 13, "Video Back action or obsolete restart notice failed");
    stage(a, "SDL menu navigation: Down/wrap, Setup, Video, mode round trip, resolution and Back passed");
    return 1;
}
static int startup_menu_checks(BHGameApp *a)
{
    unsigned long i, row;
    BHVec3 zero = {0, 0, 0};
    BHBootstrapMenu *m;
    BHMenuPresent saved;
    char name[80];
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i) {
        m = a->bootstrap.menus + i;
        if (m->identity != 0x495e40 && m->identity != 0x4cdb90 && m->identity != 0x496220)
            continue;
        for (row = 0; row < m->settings.row_count; ++row)
            if (m->settings.rows[row] && m->settings.rows[row]->refresh)
                m->settings.rows[row]->refresh(&m->settings, (long)row);
        saved = m->present;
        m->present.eye = m->present.angles = &zero;
        m->present.distance = 4.5f;
        m->present.push = 1;
        m->present.application_mode = 0;
        for (row = 0; row < m->settings.row_count; ++row) {
            m->present.selection = (long)row;
            m->present.display_selection = (float)row;
            if (!bh_frame_clear_color(&a->display, 0, 0, 0) ||
                !bh_draw_queue_begin(&a->queue) || bh_camera_queue_pose(&a->queue, &zero, &zero) != 1 ||
                !bh_menu_present(&m->present) || !render_frame(a, &a->queue))
                return failure(a, 13, "3D menu row rendering failed");
            sprintf(name, "menu-%08lx-row-%lu", m->identity, row);
            if (!capture_fixture(a, name)) return 0;
        }
        m->present = saved;
    }
    stage(a, "SDL menu fixtures: main/options/video selections rendered through production 3D menu owners");
    return startup_menu_navigation(a);
}
/* Source-only render fixtures: no input, projectile creation, entity steps,
   menus or simulation. Check actual production bindings against real pixels. */
static int startup_effect_checks(BHGameApp *a)
{
    unsigned long pass, x, y, pixels, unique, color, hash, trail_hash = 0, fresh_hash = 0;
    unsigned char seen[8192];
    BHVec3 zero = {0, 0, 0}, left = {-2, 0, 20}, right = {2, 0, 20}, center = {0, 0, 20}, up = {0, -1, 0},
           normal = {0, 0, -1};
    DDBLTFX fill;
    DDSURFACEDESC locked;
    char text[160];
    BHHudStatusState status;
    BHHudWeaponAmmo ammo[2];
    const BHHudWeaponAmmo *ammo_ptr[2];
    static const char *names[12] = {"beam",
                                   "boost",
                                   "logo",
                                   "panel",
                                   "radar",
                                   "HUD",
                                   "ballistic trail",
                                   "ballistic fresh flare",
                                   "ballistic repeated flare",
                                   "interpolated fresh flare",
                                   "interpolated repeated flare",
                                   "next tick fresh flare"};
    memset(&status, 0, sizeof(status));
    memset(ammo, 0, sizeof(ammo));
    ammo[0].amount = 216;
    ammo[1].amount = 0;
    ammo_ptr[0] = ammo;
    ammo_ptr[1] = ammo + 1;
    status.weapons = ammo_ptr;
    status.weapon_count = 2;
    status.secondary = 1;
    status.counter_a = 10000;
    if (!startup_clock_checks(a))
        return 0;
#ifdef DEBUGGING
    if (!startup_perf_checks(a))
        return 0;
#endif
    for (pass = 0; pass < 12; ++pass)
    {
        memset(&fill, 0, sizeof(fill));
        fill.dwSize = sizeof(fill);
        if (IDirectDrawSurface_Blt(a->target, 0, 0, 0, DDBLT_COLORFILL | DDBLT_WAIT, &fill) != DD_OK)
            return failure(a, 13, "Effect fixture clear failed");
        /* Pass 8 replays the same mutable queue; pass 10 replays its render
           snapshot through interpolation. Both must retain flare consumption. */
        if (pass != 8 && pass != 10 &&
            (!bh_draw_queue_begin(&a->queue) || bh_camera_queue_pose(&a->queue, &zero, &zero) != 1))
            return failure(a, 13, "Effect fixture initialization failed");
        if (pass == 0)
        {
            if (bh_draw_segment(&a->queue, 3, a->projectile_draw.ballistic_trail, &left, &right, .3f, .3f, 1, 1, 0) !=
                1)
                return failure(a, 13, "Beam fixture queue failed");
        }
        else if (pass == 1)
        {
            if (bh_draw_oriented(&a->queue, 3, a->player_scene.particle_image, &center, &up, &normal, 2, 2, 0, 8,
                                 .5f) != 1)
                return failure(a, 13, "Boost fixture queue failed");
        }
        else if (pass == 2)
        {
            if (bh_pickup_rect_queue(&a->queue, 3, a->flyby.corner_left_image, 100, 100, 128, 128, 0, 0, 1, 0, 1, 0, 1,
                                     a->view.near_z + .01f) != 1)
                return failure(a, 13, "Logo fixture queue failed");
        }
        else if (pass == 3)
        {
            if (bh_screen_quad_queue(&a->queue, 7, a->panel.icon_images[0], 100, 100, 128, 128, 0, 0, 1) != 1)
                return failure(a, 13, "Panel fixture queue failed");
        }
        else if (pass == 4)
        {
            if (bh_screen_quad_queue(&a->queue, 8, a->hud.background_image, 100, 100, 128, 128, 0, 0, 1) != 1)
                return failure(a, 13, "Radar fixture queue failed");
        }
        else if (pass == 5)
        {
            if (!bh_hud_flat_draw(&a->flat_hud, &status))
                return failure(a, 13, "Static HUD fixture queue failed");
        }
        else if (pass == 6 || pass == 7 || pass == 9 || pass == 11)
        {
            if (bh_draw_segment(&a->queue, 3, a->projectile_draw.ballistic_trail, &left, &right, .3f, .3f, 1, 1, 0) !=
                1)
                return failure(a, 13, "Ballistic trail fixture queue failed");
            if ((pass == 7 || pass == 9 || pass == 11) &&
                bh_flare_queue(&a->queue, &center, 2, 50, a->projectile_draw.ballistic_flare, 0, 0, 0, 0) != 1)
                return failure(a, 13, "Ballistic flare fixture queue failed");
        }
        if (pass >= 9) {
            if (pass != 10) bh_interpolation_capture(&a->interpolation, &a->queue, 0);
            if (!render_frame(a, bh_interpolation_sample(&a->interpolation, pass == 10 ? .5f : 0))) return 0;
        } else if (!render_frame(a, &a->queue)) return 0;
        sprintf(text, "effect-%lu", pass);
        if (!capture_fixture(a, text)) return 0;
        memset(&locked, 0, sizeof(locked));
        locked.dwSize = sizeof(locked);
        if (IDirectDrawSurface_Lock(a->target, 0, &locked, DDLOCK_WAIT, 0) != DD_OK)
            return failure(a, 13, "Effect fixture readback failed");
        pixels = unique = 0;
        hash = 2166136261UL;
        memset(seen, 0, sizeof(seen));
        for (y = 0; y < (unsigned long)a->height; ++y)
            for (x = 0; x < (unsigned long)a->width; ++x)
            {
                color = ((unsigned short *)((char *)locked.lpSurface + y * locked.lPitch))[x];
                hash = (hash ^ color) * 16777619UL;
                if (color)
                {
                    ++pixels;
                    if (!(seen[color >> 3] & (1 << (color & 7))))
                    {
                        seen[color >> 3] |= 1 << (color & 7);
                        ++unique;
                    }
                }
            }
        IDirectDrawSurface_Unlock(a->target, 0);

#ifdef DEBUGGING
        sprintf(text, "Static %s source fixture: %lu nonblack pixels, %lu colors, hash=%08lx", names[pass], pixels,
                unique, hash);
#endif
        stage(a, text);
        if (pass == 6)
            trail_hash = hash;
        if (pass == 7)
        {
            fresh_hash = hash;
            if (fresh_hash == trail_hash)
                return failure(a, 13, "Fresh ballistic flare positive control was invisible");
        }
        if (pass == 8 && (hash != trail_hash || hash == fresh_hash))
            return failure(a, 13, "Repeated ballistic queue did not restore orange trail pixels");
        if ((pass == 9 || pass == 11) && hash != fresh_hash)
            return failure(a, 13, "Fresh publication lost its ballistic flare");
        if (pass == 10 && hash != trail_hash)
            return failure(a, 13, "Interpolated render resurrected the consumed ballistic flare");
        if (pixels < 8)
            return failure(a, 13, "Effect fixture produced no visible footprint");
        if ((pass == 2 || pass == 3 || pass == 5) && unique < 16)
            return failure(a, 13, "Texture fixture lost image color content");
        if ((pass == 2 || pass == 4) && pixels >= 128UL * 128UL)
            return failure(a, 13, "Texture fixture lost transparent background");
    }
    return 1;
}
static int startup_callback_continuation_checks(BHGameApp *a, unsigned long resource)
{
    static const unsigned long patterns[4] = {0x7fc12345UL, 0xffc00000UL, 0x7f800000UL, 0xff800000UL};
    float recorded[7] = {645.083496f, 49.1670189f, 10, -.671133101f, 0, 0, 1};
    BHVec3 zero = {0, 0, 0}, position = {0, 0, 10};
    unsigned long field, k, slot;
    if (!a->hardware)
        return 1;
    for (field = 0; field < 7; ++field)
        for (k = 0; k < 4; ++k)
        {
            float bad[7];
            slot = (field + 5) % 7;
            memcpy(bad, recorded, sizeof(bad));
            memcpy(bad + slot, patterns + k, 4);
            if (!bh_draw_queue_begin(&a->queue) || bh_camera_queue_pose(&a->queue, &zero, &zero) != 1 ||
                bh_draw_submit_resource(&a->queue, 0, resource, (BHVec3 *)bad, (BHVec3 *)(bad + 3), bad[6]) != 1 ||
                bh_draw_submit_resource(&a->queue, 0, resource, &position, &zero, 1) != 1)
                return failure(a, 13, "Static invalid/valid command construction failed");
            if (!render_frame(a, &a->queue) || a->render.failures != 1 ||
                a->render.error != BH_RENDER_EXECUTE_CALLBACK || a->render.kinds[BH_DRAW_MODEL_RESOURCE] != 2 ||
                a->render.failed_resource != resource || a->resource_draw.error != BH_DRAW_RESOURCE_OK)
                return failure(a, 13, "Original callback continuation contract failed");
        }
    stage(
        a,
        "Static hardware callback continuation: all seven payload fields x NaN/indefinite/+Inf/-Inf retain diagnostics, consume later valid commands and finish frame");
    return 1;
}
static int startup_player_model_checks(BHGameApp *a)
{
    static const float depths[11] = {0, .001f, .125f, .2f, .5f, 1, 2, 4, 8, 16, 32}, offsets[5] = {-4, -.1f, 0, .1f, 4};
    BHVec3 zero = {0, 0, 0}, position, angles = {0, 0, 0};
    unsigned long i, j, k, resource;
    memcpy(&resource, (char *)a->player + 0x21c, 4);
    for (i = 0; i < N(depths); ++i)
        for (j = 0; j < N(offsets); ++j)
            for (k = 0; k < 4; ++k)
            {
                position.x = offsets[j];
                position.y = 0;
                position.z = depths[i];
                angles.x = k * 1.5707963267948966f;
                if (!bh_draw_queue_begin(&a->queue) || bh_camera_queue_pose(&a->queue, &zero, &zero) != 1 ||
                    bh_draw_submit_resource(&a->queue, 0, resource, &position, &angles, 1) != 1 ||
                    !render_frame(a, &a->queue))
                    return 0;
            }
    stage(a, "Static player model clipping: 220 camera-plane/side/rotation poses passed");
    return startup_callback_continuation_checks(a, resource);
}
#ifdef DEBUGGING
static int startup_portal_seam_checks(BHGameApp *a)
{
    BHPcx pcx;
    BHTextureImage *texture;
    unsigned char texels[32 * 32];
    BHVec3 zero = {0, 0, 0}, eye = {1, 0, 0}, left = {-5, 0, 10}, right = {5, 0, 10},
           up = {0, -1, 0}, normal = {0, 0, -1},
           portal[4] = {{0, -5, 10}, {10, -5, 10}, {10, 5, 10}, {0, 5, 10}};
    BHMainCamera stale;
    const BHMainCamera *saved_camera = a->controls.camera;
    DDSURFACEDESC pixels;
    unsigned long pass, x, color, gaps[2] = {0, 0};
    char option[8], message[160];
    if (!GetEnvironmentVariableA("BH_PLAYER_INTERPOLATION_CHECK", option, sizeof(option))) return 1;
    memset(&pcx, 0, sizeof(pcx)); memset(texels, 1, sizeof(texels));
    memset(pcx.palette, 255, sizeof(pcx.palette));
    pcx.pixels = texels; pcx.width = pcx.height = 32; pcx.pixel_bytes = sizeof(texels);
    texture = bh_texture_stage(&a->render_mode.engine.textures, "@@PORTAL-SEAM@@", &pcx, 0, 0);
    if (!texture || !bh_texture_preallocate(&a->render_mode.engine.textures))
        return failure(a, 13, "Portal seam fixture texture failed");
    texture->runs = (unsigned long *)a->render_mode.engine.textures.allocator.allocate(0x80, a->render_mode.engine.textures.allocator.user);
    texture->vertices = (D3DTLVERTEX *)a->render_mode.engine.textures.allocator.allocate(0xc00, a->render_mode.engine.textures.allocator.user);
    texture->vertex_capacity = 96;
    if (!texture->runs || !texture->vertices) return failure(a, 13, "Portal seam fixture vertex arena failed");
    stale = a->main_camera;
    if (!bh_main_camera_pose(&a->view, &stale, 0, 0, 0, 0, 0, 0)) return 0;
    for (pass = 0; pass < 2; ++pass) {
        if (!bh_draw_queue_begin(&a->queue) || !bh_frame_clear_color(&a->display, 0, 0, 255) ||
            bh_camera_queue_pose(&a->queue, &eye, &zero) != 1 ||
            bh_menu_light_queue(&a->queue, 0, 0, 0, 0, 0, 0, 0, 0, 0) != 1 ||
            bh_draw_controls_clip_rect(&a->queue, 0, 0, 0, 640, 480) != 1 ||
            bh_quad_queue(&a->queue, 0, (unsigned long)texture, &left, &up, &normal, 10, 10, 0, 8, 1, 0, 1, 0, 1) != 1 ||
            bh_draw_controls_portal_clip(&a->queue, 0, portal, BH_DRAW_END, .01f, 25000) != 1 ||
            bh_quad_queue(&a->queue, 0, (unsigned long)texture, &right, &up, &normal, 10, 10, 0, 8, 1, 0, 1, 0, 1) != 1 ||
            bh_draw_controls_clip_rect(&a->queue, 0, 0, 0, 640, 480) != 1)
            return failure(a, 13, "Portal seam fixture queue failed");
        a->controls.camera = pass ? saved_camera : &stale;
        if (!render_frame(a, &a->queue) || !capture_fixture(a, pass ? "portal-seam-fixed" : "portal-seam-before")) return 0;
        memset(&pixels, 0, sizeof(pixels)); pixels.dwSize = sizeof(pixels);
        if (IDirectDrawSurface_Lock(a->target, 0, &pixels, DDLOCK_WAIT, 0) != DD_OK) return 0;
        /* Inspect the interior seam, excluding outer viewport raster edges. */
        for (x = 1; x < 639; ++x) {
            color = ((unsigned short *)((char *)pixels.lpSurface + 240 * pixels.lPitch))[x];
            if ((color & 0xffe0UL) == 0 && (color & 31)) ++gaps[pass];
        }
        IDirectDrawSurface_Unlock(a->target, 0);
    }
    a->controls.camera = saved_camera;
    if (gaps[0] < 10 || gaps[1]) return failure(a, 13, "Portal seam positive/negative pixel controls failed");
    sprintf(message, "SDL portal seam pixels: stale camera=%lu blue gap pixels, render camera=%lu, same geometry and depth", gaps[0], gaps[1]);
    stage(a, message);
    return 1;
}
/* Compare moving target brackets against the original screen-space equations
   evaluated at the render pose, including yaw, dolly and target motion. */
static void __cdecl tracking_query(BHEntity *entity, void *packet)
{
    ((BHEntityPositionPacket *)packet)->position = entity->position;
}
static int startup_marker_tracking_checks(BHGameApp *a)
{
    BHEntity actor;
    BHVec3 eye, angles = {0,0,0}, radar_player = {0,-10000,0};
    BHDrawQueue *sample;
    unsigned char flags = 0;
    unsigned long frame, i, reference, kind = 1;
    float alpha, x, y, z, scale, opacity, size = 16, depth, fx, fy;
    char name[80];
    memset(&actor, 0, sizeof(actor));
    actor.query = tracking_query; actor.generation = 1;
    memcpy(actor.payload + 24, &kind, 4);
    memset(&a->interpolation, 0, sizeof(a->interpolation));
    bh_interpolation_bind(&a->queue);
    for (frame = 0; frame < 2; ++frame) {
        actor.position.x = frame * 8.0f; actor.position.z = 40 - frame * 16.0f;
        eye.x = -2 + frame * 4.0f; eye.y = 0; eye.z = frame * 4.0f;
        angles.x = -.05f + frame * .15f;
        if (!bh_main_camera_pose(&a->view, &a->main_camera, eye.x, eye.y, eye.z, angles.x, 0, 0)) return 0;
        bh_draw_queue_begin(&a->queue); bh_interpolation_reset();
        bh_camera_queue_pose(&a->queue, &eye, &angles);
        bh_interpolation_stamp(9, 0x1234, 1);
        if (!bh_hud_markers_draw(&a->markers, &actor, &radar_player, &flags) || a->markers.queued != 5)
            return failure(a, 13, "Moving target fixture failed to queue five brackets/label");
        bh_interpolation_capture(&a->interpolation, &a->queue, 1);
    }
    for (frame = 0; frame < 3; ++frame) {
        alpha = frame * .5f;
        for (reference = 0; reference < 2; ++reference) {
            if (!reference) sample = bh_interpolation_sample(&a->interpolation, alpha);
            else {
                actor.position.x = alpha * 8; actor.position.z = 40 - alpha * 16;
                eye.x = -2 + alpha * 4; eye.z = alpha * 4; angles.x = -.05f + alpha * .15f;
                bh_main_camera_pose(&a->view, &a->main_camera, eye.x, 0, eye.z, angles.x, 0, 0);
                x = actor.position.x; y = 0; z = actor.position.z;
                if (!bh_pickup_project(&a->pickup_draw, &x, &y, &z, &scale)) return 0;
                opacity = scale * 100 / 640; if (opacity > 1) opacity = 1;
                depth = z - 8; if (depth < a->view.near_z + .01f) depth = a->view.near_z + .01f;
                bh_draw_queue_begin(&a->queue); bh_camera_queue_pose(&a->queue, &eye, &angles);
                for (i = 0; i < 4; ++i) {
                    fx = (i & 1) ? 2.0f : -2.0f; fy = i < 2 ? -2.0f : 2.0f;
                    bh_pickup_rect_queue(&a->queue, 3, a->markers.brackets_image, x + fx * scale, y + fy * scale,
                                         size, size, i == 0 ? 0 : i == 1 ? 1.57079637f : i == 2 ? -1.57079637f : -3.14159274f,
                                         0, opacity, 0, .25f, 0, .25f, depth);
                }
                bh_pickup_rect_queue(&a->queue, 3, a->markers.brackets_image, x - 2 * scale + 24, y + 2 * scale + 20,
                                     64, size, 0, 0, opacity, 0, .5f, .25f, .125f, depth);
                sample = &a->queue;
            }
            if (!bh_frame_clear_color(&a->display, 0, 0, 0) || !render_frame(a, sample)) return 0;
            sprintf(name, "target-tracking-%lu-%s", frame, reference ? "reference" : "interpolated");
            if (!capture_fixture(a, name)) return 0;
        }
    }
    bh_interpolation_bind(0);
    memset(&a->interpolation, 0, sizeof(a->interpolation));
    stage(a, "SDL target tracking: moving target, camera yaw/dolly, three fractions versus render-pose reference");
    return 1;
}
static int startup_player_interpolation_checks(BHGameApp *a)
{
    BHEntity saved_player;
    BHCameraState *camera;
    BHMainCamera tick_camera;
    const BHMainCamera *saved_clip_camera;
    BHVec3 zero = {0, 0, 0}, hidden = {0, 0, -100000};
    BHDrawQueue *sample;
    unsigned long frame, j, k, car, camera_command, resource, attachments = 0, mismatches = 0;
    long saved_mode;
    float expected, observed, previous_x = 0, previous_camera_x = 0, alpha;
    char option[8], path[512], directory[MAX_PATH];
    FILE *trace = 0;
    if (!GetEnvironmentVariableA("BH_PLAYER_INTERPOLATION_CHECK", option, sizeof(option))) return 1;
    saved_player = *a->player;
    saved_mode = a->player_state.mode;
    camera = bh_player_camera_state(a->player);
    memcpy(&resource, (char *)a->player + 0x21c, 4);
    if (GetEnvironmentVariableA("BH_RENDER_CAPTURE_DIR", directory, sizeof(directory))) {
        sprintf(path, "%s\\player-interpolation.csv", directory);
        trace = fopen(path, "w");
        if (trace) fprintf(trace, "tick,commands,alpha,car_expected,car_rendered,camera_expected,camera_rendered\n");
    }
    a->player_state.mode = 1;
    *((unsigned char *)a->player + 0x2bb) = 1;
    memset(&a->interpolation, 0, sizeof(a->interpolation));
    bh_interpolation_bind(&a->queue);
    for (frame = 0; frame < 16; ++frame) {
        a->player->position.x = saved_player.position.x + frame * 2;
        camera->position.x = a->player->position.x;
        camera->position.y = a->player->position.y - 2;
        camera->position.z = a->player->position.z - 12;
        camera->angles = zero;
        camera->current.mode = frame % 3 ? 1 : 0;
        bh_draw_queue_begin(&a->queue);
        bh_interpolation_reset();
        /* Visibility/effect churn ahead of the real player scene must not
           change the identity of its camera, car, NPCs or cockpit. */
        for (j = 0; j < frame % 4; ++j)
            bh_draw_submit_resource(&a->queue, 0, resource, &hidden, &zero, 1);
        if (!bh_player_scene_draw(&a->player_scene)) { mismatches = 1; break; }
        bh_interpolation_stamp(9, (unsigned long)a->player, a->player->generation * 2 + 1);
        car = camera_command = BH_DRAW_END;
        for (j = 9; j < a->queue.count; ++j) {
            BHDrawCommand *c = a->queue.commands + j;
            if (c->kind == BH_DRAW_CAMERA) camera_command = j;
            if (c->kind == BH_DRAW_MODEL_RESOURCE && c->resource == resource &&
                !memcmp(c->words, &a->player->position, sizeof(BHVec3))) car = j;
        }
        if (car == BH_DRAW_END || camera_command == BH_DRAW_END) { mismatches = 1; break; }
        bh_interpolation_capture(&a->interpolation, &a->queue, 1);
        for (j = 0; j < 3; ++j) {
            float expected_camera, observed_camera;
            alpha = j * .5f;
            sample = bh_interpolation_sample(&a->interpolation, alpha);
            expected = frame ? previous_x + (a->player->position.x - previous_x) * alpha : a->player->position.x;
            expected_camera = frame ? previous_camera_x + (camera->position.x - previous_camera_x) * alpha : camera->position.x;
            memcpy(&observed, sample->commands[car].words, 4);
            memcpy(&observed_camera, &sample->commands[camera_command].resource, 4);
            if (fabs(observed - expected) > .001 || fabs(observed_camera - expected_camera) > .001) ++mismatches;
            for (k = 9; k < sample->count; ++k) if (a->interpolation.current_ids[k].attachment) {
                float current_x, render_x;
                memcpy(&current_x, a->interpolation.current[k].words, 4);
                memcpy(&render_x, sample->commands[k].words, 4);
                if (fabs((render_x - current_x) - (expected - a->player->position.x)) > .001) ++mismatches;
                ++attachments;
            }
            if (trace) fprintf(trace, "%lu,%lu,%.1f,%.9g,%.9g,%.9g,%.9g\n", frame, a->queue.count, alpha,
                               expected, observed, expected_camera, observed_camera);
            if (frame == 8 && j == 1) {
                /* Same interpolated geometry, deliberately stale portal camera
                   for a before/after capture of the original seam failure. */
                tick_camera = a->main_camera;
                saved_clip_camera = a->controls.camera;
                if (!bh_main_camera_pose(&a->view, &tick_camera, camera->position.x, camera->position.y,
                                         camera->position.z, camera->angles.x, camera->angles.y, camera->angles.z)) ++mismatches;
                a->controls.camera = &tick_camera;
                if (!bh_frame_clear_color(&a->display, 0, 0, 0) || !render_frame(a, sample) || !capture_fixture(a, "portal-stale-camera")) ++mismatches;
                a->controls.camera = saved_clip_camera;
                sample = bh_interpolation_sample(&a->interpolation, alpha);
                if (!bh_frame_clear_color(&a->display, 0, 0, 0) || !render_frame(a, sample) || !capture_fixture(a, "player-interpolated")) ++mismatches;
            }
        }
        previous_x = a->player->position.x;
        previous_camera_x = camera->position.x;
    }
    if (trace) fclose(trace);
    *a->player = saved_player;
    a->player_state.mode = saved_mode;
    bh_interpolation_bind(0);
    memset(&a->interpolation, 0, sizeof(a->interpolation));
    if (mismatches || !attachments) return failure(a, 13, "Player/camera/cockpit interpolation failed under visibility churn");
    stage(a, "SDL player interpolation: 16 production scene poses x 3 render fractions, changing visibility/cockpit HUD, zero pose mismatches");
    return startup_marker_tracking_checks(a);
}
#endif
/* Simulation and audio run at 30 Hz. Render copies interpolate between the
   last two completed ticks; all callback mutations stay in the render copy. */
static int manual_loop(BHGameApp *a)
{
    DWORD began;
    BHSimulationClock clock;
    RECT destination;
    POINT origin;
    unsigned long i, presentations = 0, ticks, tick;
    int produce;
#ifdef DEBUGGING
    double smoke_elapsed;
    MCI_STATUS_PARMS music_status;
    FILE *smoke_summary;
    DWORD rendered, presented;
    float controls[3];
    char profile[512];
    LARGE_INTEGER simulation_start, render_start, present_start, frame_end;
#endif
    if (a->startup_check)
        return failure(a, 12, "Startup check cannot enter application frames");
    memset(&clock, 0, sizeof(clock));
#ifdef DEBUGGING
    if (!perf_start(a))
        return 0;
#endif
    if (a->frames.produce != source_produce || a->frames.produce_user != a ||
        a->frames.bytes != 3500UL * sizeof(BHDrawCommand) || !bh_draw_queue_dispose(&a->queue))
        return failure(a, 12, "Source frame ownership initialization failed");

#ifdef DEBUGGING
    a->service.profile = 1;
#endif
    a->service.input = &a->collection;
    a->service.entities = &a->pool;
    a->service.sound = &a->sound;
    a->service.frames = &a->frames;
    a->service.draw = &a->queue;
    bh_interpolation_bind(&a->queue);
    for (i = 0; i < 3; ++i)
        a->service.slot_draw[i] = a->slots + i;
    a->service.player_input = &a->global_state;
    a->service.paused = (const volatile LONG *)&a->player_state.paused;
    a->service.frame_counter = &a->frame;
    a->service.stop = &a->worker.stop;
    a->service.worker_done = &a->worker.done;
    a->text_capture.input = a->bootstrap.all_keys;
    a->key_capture.input = a->bootstrap.all_keys;
    stage(a, "Manual preview running; keyboard/mouse belong to the active game window");
#ifdef DEBUGGING
    {
        char value[32];
        if (GetEnvironmentVariableA("BH_MENU_SMOKE_SECONDS", value, sizeof(value))) {
            a->menu_smoke_seconds = atof(value);
            if (a->menu_smoke_seconds < 1 || a->menu_smoke_seconds > 30) a->menu_smoke_seconds = 5;
            a->menu_smoke_origin = bh_sdl_seconds();
            a->collection.disable_mouse_motion = 1;
            a->focus_sample = 1;
        }
    }
#endif
    while (!a->quit && !a->error)
    {
        BH_MP_BEGIN(frame_marker);
        began = GetTickCount();
        if (!bh_sdl_poll())
            a->quit = 1;
        if (a->quit)
            break;
        if (!a->box_auto &&
#ifdef DEBUGGING
            !a->menu_smoke_seconds &&
#endif
            (GetForegroundWindow() != a->window || IsIconic(a->window)))
        {
            a->focus_sample = 0;
            clock.ready = 0;
            a->interpolation.ready = 0;
#ifdef DEBUGGING
            a->perf_previous.QuadPart = 0;
#endif
            Sleep(20);
            continue;
        }
        if (!a->focus_sample)
        {
            SetCursor(NULL);
            if (!a->collection.disable_mouse_motion && !game_recenter(160, 100))
                return failure(a, 12, "Windowed input recenter failed");
            memset(a->snapshot.mouse_axes, 0, sizeof(a->snapshot.mouse_axes));
            memset(a->snapshot.pair4, 0, sizeof(a->snapshot.pair4));
            a->collection.delta[0] = a->collection.delta[1] = 0;
            a->focus_sample = 1;
        }
        ticks = a->box_auto ? 1 : bh_simulation_advance(&clock, bh_sdl_seconds());
        produce = ticks != 0;
#ifdef DEBUGGING
        if (a->perf_enabled)
            QueryPerformanceCounter(&simulation_start);
#endif
        for (tick = 0; tick < ticks; ++tick)
        {
            if (a->box_level && a->snapshot.current[VK_ESCAPE])
            {
                a->quit = 1;
                break;
            }
#ifdef DEBUGGING
            box_inputs(a);
#endif
            a->flyby.input_profile = a->profile;
            a->menu_settings.input_profile = a->profile;
            a->player_create.profile = a->profile;
            a->actor_launch.mode = a->actor_damage.mode = a->player_state.mode;
            a->eye = a->flyby_state.camera.position;
            a->angles = a->flyby_state.camera.angles;
            a->contacts.has_camera = 1;
            a->contacts.camera_yaw = a->yaw = a->angles.x;
            {
                BH_MP_BEGIN(marker);
                a->frames.produce(a->frames.produce_user);
                BH_MP_END(marker, "simulation", a->player_state.paused ? "paused" : a->player_state.mode == 0 ? "menu" : a->player_state.mode == 1 ? "singleplayer" : "network", a->frame);
            }
#ifdef DEBUGGING
            box_observe(a);
#endif
            if (a->error)
                break;
            bh_interpolation_capture(&a->interpolation, a->slots + a->frames.latest,
                                     a->player_state.mode * 100 + a->mission);
        }
        if (a->player_tick.error || a->player_query.error || a->vm.error || a->lifetime.timers.error || a->menu.error ||
            a->flyby.error || a->actor_construct.error || a->actor_response.error)
        {
            char text[256];
#ifdef DEBUGGING
            sprintf(
                text,
                "Source dependency failure: player=%d query=%d vm=%d timer=%d menu=%d flyby=%d actor=%d response=%d",
                a->player_tick.error, a->player_query.error, a->vm.error, a->lifetime.timers.error, a->menu.error,
                a->flyby.error, a->actor_construct.error, a->actor_response.error);
#endif
            failure(a, 12, text);
            break;
        }
#ifdef DEBUGGING
        if (a->perf_enabled)
            QueryPerformanceCounter(&render_start);
#endif

#ifdef DEBUGGING
        rendered = GetTickCount();
#endif
        {
            int render_ok;
            BH_MP_BEGIN(marker);
            render_ok = render_frame(a, bh_interpolation_sample(&a->interpolation, (float)(clock.remainder * 30.0)));
            BH_MP_END(marker, "render", "", a->frame);
            if (!render_ok)
                break;
        }
#ifdef DEBUGGING
        rendered = GetTickCount() - rendered;
#endif
#ifdef DEBUGGING
        if (a->perf_enabled)
            QueryPerformanceCounter(&present_start);
#endif
        GetClientRect(a->window, &destination);
        origin.x = origin.y = 0;
        ClientToScreen(a->window, &origin);
        OffsetRect(&destination, origin.x, origin.y);

#ifdef DEBUGGING
        presented = GetTickCount();
#endif
        if (a->fullscreen)
        {
            BH_MP_BEGIN(present_marker);
            if (a->target != a->render_mode.render &&
                IDirectDrawSurface_BltFast(a->render_mode.render, 0, 0, a->target, 0, DDBLTFAST_WAIT) != DD_OK)
            {
                failure(a, 13, "Fullscreen copy failed");
                break;
            }
            if (IDirectDrawSurface_Flip(a->primary, 0, DDFLIP_WAIT) != DD_OK)
            {
                failure(a, 13, "Fullscreen presentation failed");
                break;
            }
            BH_MP_END(present_marker, "present", "", presentations);
        }
        else if (IDirectDrawSurface_Blt(a->primary, &destination, a->target, 0, DDBLT_WAIT, 0) != DD_OK)
        {
            failure(a, 13, "Windowed presentation failed");
            break;
        }
#ifdef DEBUGGING
        presented = GetTickCount() - presented;
#endif
#ifdef DEBUGGING
        if (a->perf_enabled)
        {
            QueryPerformanceCounter(&frame_end);
            perf_record(a, produce, simulation_start, render_start, present_start, frame_end);
        }
#endif
        ++presentations;
        BH_MP_END(frame_marker, "frame", "", presentations);
#ifdef DEBUGGING
        if (!a->perf_enabled && produce && (a->frame <= 5 || a->frame % 30 == 0))
        {
            memcpy(controls, (char *)a->player + 0x18c, 12);
            sprintf(
                profile,
                "Frame profile: frame=%lu mode=%ld total=%lu input=%lu step=%lu refresh=%lu sound=%lu render=%lu blit=%lu commands=%lu mouse=%ld,%ld filtered=%.5g,%.5g controls=%.5g,%.5g,%.5g angular=%.5g,%.5g,%.5g keys_LR=%u,%u panel=%d arena=%lu ring=%ld presentations=%lu",
                a->frame, a->player_state.mode, GetTickCount() - began, a->service.input_ms, a->service.step_ms,
                a->service.refresh_ms, a->service.sound_ms, rendered, presented, a->queue.count, a->collection.delta[0],
                a->collection.delta[1], a->snapshot.mouse_axes[0], a->snapshot.mouse_axes[1], controls[0], controls[1],
                controls[2], a->player_motion.angular_velocity.x, a->player_motion.angular_velocity.y,
                a->player_motion.angular_velocity.z, a->snapshot.current[37], a->snapshot.current[39], a->panel.error,
                a->messages.arena_used, a->messages.ring_count, presentations);
            stage(a, profile);
        }
#endif
        if (!a->box_auto)
            bh_sdl_pace();
#ifdef DEBUGGING
        if (a->menu_smoke_seconds && (smoke_elapsed = bh_sdl_seconds() - a->menu_smoke_origin) >= a->menu_smoke_seconds) {
            memset(&music_status, 0, sizeof(music_status));
            music_status.dwItem = MCI_STATUS_POSITION;
            if (a->cd.device) mciSendCommandA(a->cd.device, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)&music_status);
            smoke_summary = fopen("menu-smoke-summary.json", "w");
            if (smoke_summary) {
                fprintf(smoke_summary, "{\"seconds\":%.6f,\"presentations\":%lu,\"simulation_ticks\":%lu,\"sound_ticks\":%lu,\"music_device\":%u,\"local_music\":%d,\"music_position\":%lu,\"music_plays\":%lu,\"music_failures\":%lu,\"error\":%d}\n",
                        smoke_elapsed, presentations, a->service.calls, a->service.sound_calls,
                        a->cd.device, a->cd.local_file, (unsigned long)music_status.dwReturn, a->cd_notify.plays, a->cd_notify.failures, a->error);
                fclose(smoke_summary);
            }
            capture_fixture(a, "menu-live");
            a->quit = 1;
        }
#endif
    }
    return !a->error;
}
static void cleanup(BHGameApp *a)
{
    unsigned long i;
#ifdef DEBUGGING
    if (a->box_trace)
    {
        fclose(a->box_trace);
        a->box_trace = 0;
    }
    box_observer = 0;
#endif
#ifdef DEBUGGING
    perf_save(a);
#endif
    for (i = 0; i < N(a->actor_queries); ++i)
        if (a->actor_queries[i].frames)
            bh_segment_query_dispose(a->actor_queries + i);
    if (!a->startup_check && a->window && GetForegroundWindow() == a->window)
        SetCursor(LoadCursorA(NULL, IDC_ARROW));
    if (a->hit_markers.pool)
        bh_hit_marker_dispose(&a->hit_markers);
    bh_loading_progress_remove(&a->loading_progress);
    bh_loading_images_dispose(&a->loading_images);
    /* No callback may survive its context. Quiesce all callbacks before releasing
      metadata; shared models/textures are released after their borrowers. */
    if (a->panel.entity)
        bh_message_panel_close(&a->panel);
    bh_flyby_scene_unbind(&a->flyby);
    bh_player_tick_unbind(&a->player_tick);
    bh_player_query_unbind(&a->player_query);
    if (a->physics.pool)
        bh_physics_step_dispose(&a->physics);
    bh_scene_bindings_dispose(&a->scene_bindings);
    for (i = 0; i < N(a->block_queries); ++i)
        if (a->block_queries[i].geometry)
            bh_segment_query_dispose(a->block_queries + i);
    if (a->pool.chunks)
    {
        bh_editor_world_dispose(&a->editor_world);
        while (a->pool.active)
            if (!bh_entity_release(&a->pool, a->pool.active))
                break;
        bh_weapon_factory_close(&a->weapons);
        bh_entities_dispose(&a->pool);
    }
    if (!a->pool.used && a->targets.slots)
    {
        for (i = 0; i < a->targets.capacity; ++i)
            a->targets.slots[i].body.entity = 0;
        bh_target_construct_dispose(&a->targets);
    }
    if (a->player_collision.bsp.allocator.release)
        bh_collision_model_dispose(&a->player_collision);
    bh_script_blocks_dispose(&a->blocks);
    bh_navigation_dispose(&a->navigation);
    bh_rail_paths_dispose(&a->rails);
    bh_text_queue_init(&a->text);
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
        bh_menu_rows_dispose(&a->bootstrap.menus[i].object.rows);
    if (a->bootstrap.confirmation)
        bh_atlas_page_dispose(&a->atlas, a->bootstrap.confirmation);
    if (a->bootstrap.menu_font)
        bh_atlas_page_dispose(&a->atlas, a->bootstrap.menu_font);
    if (a->bootstrap.graffiti)
        bh_atlas_page_dispose(&a->atlas, a->bootstrap.graffiti);
    if (a->panel.page)
        bh_atlas_page_dispose(&a->atlas, a->panel.page);
    bh_atlas_context_dispose(&a->atlas);
    bh_render_resources_dispose(&a->resources);
    bh_horizon_color_dispose(&a->horizon_colors);
    bh_gdi_text_dispose(&a->gdi);
    bh_input_devices_dispose(&a->devices);
    bh_player_input_owner_dispose(&a->inputs);
    bh_config_dispose(&a->config);
    bh_sound_queue_dispose(&a->sound);
    bh_network_game_dispose(&a->network_game);
    if (a->provider_startup.com_initialized)
        CoUninitialize();
    bh_provider_catalog_dispose(&a->providers);
    bh_network_transport_dispose(&a->transport);
    if (!a->queue.allocator.release)
        memset(&a->queue, 0, sizeof(a->queue));
    bh_interpolation_bind(0);
    bh_frame_buffers_dispose(&a->frames);
    bh_draw_queue_dispose(&a->queue);
    bh_fade_dispose(&a->fade);
    bh_render_material_dispose(&a->material);
    bh_render_mode_dispose(&a->render_mode);
    if (a->target)
        IDirectDrawSurface_Release(a->target);
    if (a->primary)
        IDirectDrawSurface_Release(a->primary);
    if (a->window_clipper)
        IDirectDrawClipper_Release(a->window_clipper);
    if (a->draw2)
    {
        IDirectDraw2_RestoreDisplayMode(a->draw2);
        IDirectDraw2_SetCooperativeLevel(a->draw2, a->window, DDSCL_NORMAL);
        IDirectDraw2_Release(a->draw2);
    }
    if (a->draw)
    {
        IDirectDraw_SetCooperativeLevel(a->draw, a->window, DDSCL_NORMAL);
        IDirectDraw_Release(a->draw);
    }
    bh_sdl_close();
    a->window = 0;
    input_window = 0;
    bh_vfile_shutdown();
    free(a->motion_bindings);
    free(a->world_responses);
    free(a->pulse_indices);
    free(a->actor_rooms);
    free(a->city_ids);
    free(a->rooms);
    free(a->scheduled);
    free(a->scene_inputs);
    free(a->segment_rooms);
    free(a->physics_worlds);
    free(a->reset_nodes);
}
int __cdecl bh_game_app_run(HINSTANCE instance, const char *directory, int startup_check, int show)
{
    BHGameApp *a;
    unsigned long bytes, crc;
    int result;
    FILE *summary;
    const char *material_error;
    BHTextureImage *loaded;
    a = (BHGameApp *)calloc(1, sizeof(*a));
    if (!a)
        return 2;
    a->instance = instance;
#ifdef DEBUGGING
    a->box_level = (startup_check & 16) != 0;
    a->box_auto = (startup_check & 32) != 0;
    if (startup_check & 64)
        a->box_auto = 2;
#endif
    a->fullscreen = 1;
#ifdef DEBUGGING
    a->perf_enabled = (startup_check & 8) != 0 && !(startup_check & 1);
#endif
    startup_check &= 1;
    a->startup_check = startup_check;
    a->width = 640;
    a->height = 480;
#ifdef DEBUGGING
    a->log = fopen(startup_check ? "startup-check.log" : "startup.log", "w");
#endif
    if (!directory || strlen(directory) > 450)
    {
        if (a->log)
            fclose(a->log);
        free(a);
        return 2;
    }
    sprintf(a->archive, "%s\\Bhunter.bin", directory);
    strcpy(a->config_path, "source-preview.cfg");
    if (!bh_math_environment_enter(&a->math))
    {
        if (a->log)
            fclose(a->log);
        free(a);
        return 2;
    }
    stage(a, "Restored source startup; no original executable loaded");
    if (!bh_game_assets_verify(a->archive, &bytes, &crc))
    {
        failure(a, 2, "Archive missing or differs from pinned data");
        goto done;
    }
    if (!platform(a, show) || !source_owners(a))
        goto done;
    if (!bh_vfile_mount(a->archive))
    {
        failure(a, 2, "Archive mount failed");
        goto done;
    }
    if (!loading_begin(a))
        goto done;
    if (!bh_sound_startup(&a->sound, a->window, &a->sound_stats))
    {
        failure(a, 6, "Native sound initialization failed");
        goto done;
    }
    bh_provider_startup(&a->provider_startup);
    if (!bh_texture_set_path(&a->render_mode.engine.textures, "textures", 0) ||
        !bh_texture_policy_set(&a->render_mode.engine.textures, 2, 0) || !bh_bootstrap_resources(&a->bootstrap, 5) ||
        !bh_bootstrap_resources(&a->bootstrap, 6))
    {
        char message[256];

#ifdef DEBUGGING
        sprintf(message, "Engine resources failed: game=%d sound=%d texture=%d loaded=%lu", a->bootstrap.error,
                a->sounds.error, a->render_mode.engine.textures.error, a->bootstrap.resources_loaded);
#endif
        failure(a, 6, message);
        goto done;
    }
    if (!bh_game_bootstrap(&a->bootstrap))
    {
        char message[256];

#ifdef DEBUGGING
        sprintf(message, "Bootstrap failed: game=%d blocks=%d parser=%d world=%d vm=%d player=%d flyby=%d",
                a->bootstrap.error, a->blocks.error, a->parser.error, a->world_load.error, a->vm.error,
                a->player_create.error, a->flyby_construct.error);
#endif
        failure(a, a->error ? a->error : 10, message);
        goto done;
    }
    {
        unsigned long i;
        char text[160];

#ifdef DEBUGGING
        sprintf(text, "Loaded model handles: count=%lu reported_failure_handle475=%s", a->resources.count,
                bh_render_resources_lookup(475, &a->resources) ? "valid" : "missing");
#endif
        stage(a, text);
        for (i = 0; i < N(a->actor_models); ++i)
            if (a->actor_models[i] && !bh_render_resources_lookup(a->actor_models[i], &a->resources))
            {

#ifdef DEBUGGING
                sprintf(text, "Actor model binding invalid: index=%lu handle=%lu", i, a->actor_models[i]);
#endif
                failure(a, 9, text);
                goto done;
            }
    }
    if (!refresh_segment_actors(&a->segment, a) || a->segment.actor_count != 1 ||
        a->segment.actors[0].entity != a->player ||
        a->segment.actors[0].query->geometry != &a->player_collision.geometry)
    {
        failure(a, 9, "Live player segment collision registry is not bound");
        goto done;
    }
    if (a->player_tick.resources.menu_cue >= a->sound.resource_count ||
        a->weapon_target.lock_sound >= a->sound.resource_count ||
        a->sound.resources[a->player_tick.resources.menu_cue]->streaming ||
        !a->sound.resources[a->weapon_target.lock_sound]->streaming)
    {
        failure(a, 9, "Confirmation beep/weapon lock sound modes are incorrect");
        goto done;
    }
    /* Original00479896: final archive display before material allocation. */
    if (!bh_loading_display(&a->loading_display, 1))
    {
        failure(a, 6, "Final loading image presentation failed");
        goto done;
    }
    /* Original00479200 calls material/texture allocation after004474c0 has
      loaded all assets. Before that, every class reference count is zero. */
    material_error =
        bh_render_material_initialize(&a->material, &a->render_mode.engine.textures, a->render_mode.engine.d3d);
    if (a->material.failures || (a->hardware && !a->material.material))
    {
        failure(a, 4, material_error ? material_error : "Native renderer material/state initialization failed");
        goto done;
    }
    if (material_error)
        stage(a, material_error);
    if (a->hardware)
        for (loaded = a->render_mode.engine.textures.images; loaded; loaded = loaded->next)
            if (!loaded->format || !loaded->format->slots || !loaded->format->slots->handle)
            {
                failure(a, 4, "Hardware image has no allocated texture slot");
                goto done;
            }
    /* Original004798d5..004798f9: detach archive progress, initialize the
       producer buffers, then display twice. There is no logo wait/input loop. */
    bh_loading_progress_remove(&a->loading_progress);
    if (!bh_frame_buffers_start(&a->frames, source_produce, a, 30, 3500 * sizeof(BHDrawCommand), source_message, a))
    {
        failure(a, 12, "Source frame ownership initialization failed");
        goto done;
    }
    if (!bh_loading_display(&a->loading_display, 1) || !bh_loading_display(&a->loading_display, 1))
    {
        failure(a, 6, "Final loading image presentation failed");
        goto done;
    }
    bh_mp_start();
    if (startup_check)
    {
        stage(a, "Checking initial scene presentation without input or entity steps");
        if (!bh_draw_queue_begin(&a->queue) || !bh_flyby_scene_draw(&a->flyby))
        {
            failure(a, 13, "Initial scene composition failed");
            goto done;
        }
        bh_menu_runtime_refresh_entity(a->menu.root->entity, 0);
        if (!render_frame(a, &a->queue))
            goto done;
        if (a->view.near_z != a->screen_clip.near_z || a->view.near_z != a->primitive.raster.near_z ||
            a->schedule.viewport == &a->view)
        {
            failure(a, 13, "Scene traversal corrupted render clipping state");
            goto done;
        }
        stage(a, "Checking static source beam/boost/logo/panel bindings after scene composition");
        if (!capture_fixture(a, "level-menu") || !startup_menu_checks(a) ||
            !startup_effect_checks(a) || !startup_player_model_checks(a))
            goto done;
#ifdef DEBUGGING
        if (!startup_player_interpolation_checks(a) || !startup_portal_seam_checks(a)) goto done;
#endif
        if (a->hardware && (a->render_mode.engine.textures.software || !a->render_mode.engine.device ||
                            !a->primitive.vertices || a->primitive.locks))
        {
            failure(a, 13, "Hardware check did not use native primitives");
            goto done;
        }
    }
    a->initialized = 1;
    stage(a, "Real-data startup complete; no input collection or application frame executed");
#ifdef DEBUGGING
    summary = fopen(startup_check ? "startup-check-summary.json" : "startup-summary.json", "w");
    if (summary)
    {
        fprintf(
            summary,
            "{\"passed\":true,\"blocks\":%lu,\"placements\":%lu,\"navigation_nodes\":%lu,\"pool_entities\":%lu,\"menus\":%lu,\"rows\":%lu,\"resources\":%lu,\"physics_bodies\":%lu,\"flyby_constructor_settling_calls\":%lu,\"input_collection_calls\":%lu,\"entity_step_calls\":%lu,\"entity_refresh_calls\":%lu,\"application_frames\":%lu,\"native_primary_sound_play_calls\":%lu,\"loading_display_calls\":%lu,\"loading_native_flips\":%lu,\"loading_adapter_presentations\":%lu,\"loading_stage\":%ld,\"frame_buffer_bytes\":%lu}\n",
            a->blocks.count, a->editor_world.count, a->navigation.node_count, a->pool.used, a->bootstrap.menus_created,
            a->bootstrap.rows_created, a->bootstrap.resources_loaded, a->physics.body_count, a->flyby_construct.settled,
            a->service.input_calls, a->service.step_calls, a->service.refresh_calls, a->frame,
            a->sound_stats.play_calls, a->loading_display.calls, a->loading_display.flips,
            a->loading_display.presentations, a->loading_images.stage, a->frames.bytes);
        fclose(summary);
    }
#endif
    if (!startup_check)
    {
#ifdef DEBUGGING
        if (!box_setup(a))
            goto done;
#endif
        bh_force_feedback_init(&a->force_init, instance, a->window);
        manual_loop(a);
    }
done:
    result = a->error;
    bh_mp_save();
    cleanup(a);
    stage(a, "Source owners released");
    bh_math_environment_leave(&a->math);
    if (a->log)
        fclose(a->log);
    free(a);
    return result;
}
