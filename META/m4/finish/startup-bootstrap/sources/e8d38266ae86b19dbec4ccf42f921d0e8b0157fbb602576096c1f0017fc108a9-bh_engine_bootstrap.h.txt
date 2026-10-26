#ifndef BH_ENGINE_BOOTSTRAP_H
#define BH_ENGINE_BOOTSTRAP_H
#include "bh_game_bootstrap.h"
#include "bh_loading_progress.h"
#include "bh_input_devices.h"
#include "bh_input_messages.h"
#include "bh_sound_startup.h"
#include "bh_force_feedback_init.h"
#include "bh_frame_service.h"
#include "bh_frame_callback.h"
typedef struct BHEngineBootstrap {
 const char *archive;BHLoadingImages *images;BHLoadingProgress *progress;
 BHInputDevices *devices;BHProviderStartup *providers;BHForceFeedbackInit *force;
 BHSoundQueue *sound;BHSoundStartupStats sound_stats;
 BHPlayerInputOwner *inputs;BHPlayerInputList *global_input;
 BHGameBootstrap *game;BHViewConfig *view;BHRenderMaterial *material;LPDIRECT3D2 d3d;
 BHFrameBuffers *frames;BHFrameService *service;BHInputSnapshot *snapshot;
 BHPhysicsStepBody *physics_bodies;unsigned long physics_body_count;
 const BHPhysicsStepWorld *physics_worlds;unsigned long physics_world_count;
 BHGeometry *const *physics_models;unsigned long physics_model_count;
 BHPhysicsEffects *physics_effects;int physics_fallback;
 long debug;unsigned long mounted,stages;int error;
} BHEngineBootstrap;
/* Whole00479200. Archive/loading progress, actual native input/sound/provider/
   force construction, exact audio/image policy tables,9600entity pool, camera,
   full game bootstrap/material and frame buffer callbacks. It starts no worker
   and invokes no frame producer. All original returns ignored by the binary
   remain diagnostics where safe; resource/binding failures become explicit
   source errors rather than unchecked NULL use. Contexts are initialized by
   their real source owners before entry, except fresh game entity pool.
   Native positive paths include actual device operations: do not use as a
   diagnostic. Synthetic checks deliberately exercise pre-device failure.
   Progress callback is removed on every exit, preserving borrowed lifetime. */
int __cdecl bh_engine_bootstrap(BHEngineBootstrap *,HINSTANCE,HWND);
void __cdecl bh_engine_frame_produce(void *);
LRESULT __cdecl bh_engine_input_message(void *,HWND,UINT,WPARAM,LPARAM);
#endif
