#ifndef BH_INPUT_MODE_H
#define BH_INPUT_MODE_H
#include "bh_input_evaluate.h"
#include "bh_menu_settings.h"
#include "bh_sound_queue.h"
#include "bh_entities.h"
typedef void(__cdecl *BHMenuKeyCommit)(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
typedef struct BHInputKeyCapture
{
    BHEntityPool *pool;
    BHPlayerInputList *input;
    BHInputSnapshot *snapshot;
    BHPlayerInputState *state;
    BHMenuSettings *settings;
    BHMenuSettingsMenu *menu;
    BHMenuKeyCommit commit;
    BHSoundQueue *sound;
    unsigned long sound_resource;
    unsigned char *result; /* borrowed two-byte original004d4388 output */
    BHEntity *entity;
    unsigned long sounds, last_voice;
    int error;
} BHInputKeyCapture;
/* Count-zero source binding of an actual temporary entity and real step.
   Fresh entity==NULL; all borrowed owners outlive capture. commit must be one
   of the four actual admitted bh_menu_keys_* owners. No device collection or
   native input API is invoked. Result bytes retain prior contents until a key.
   Original parent+1f4/global bindings become these explicit typed owners. */
int __cdecl bh_input_key_capture_begin(BHInputKeyCapture *);
/* Whole0042be70. Apply actual encoded packet each step. Phase0 primes0..254;
   phase1 skipsEscape, captures every new down in ascending order (last wins),
   and completes only on a later step. Final flush→cached mode0→real commit→
   clear commit→release temporaryentity. result255 is never scanned.
   Result/settings/menu must not alias capture entity; no independent release.
   Source diagnostics preserve subsequent original side effects. */
int __cdecl bh_input_key_capture_step(BHInputKeyCapture *);
/* Count-zero cancellation for source ownership; no key commit occurs. */
int __cdecl bh_input_key_capture_cancel(BHInputKeyCapture *);
#endif
