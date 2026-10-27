#ifndef BH_INPUT_TEXT_H
#define BH_INPUT_TEXT_H
#include "bh_input_mode.h"
#include "bh_menu_row.h"
#include "bh_menu_profile.h"
typedef enum BHInputTextCommit
{
    BH_TEXT_COMMIT_NONE,
    BH_TEXT_COMMIT_PROFILE,
    BH_TEXT_COMMIT_ADDRESS,
    BH_TEXT_COMMIT_PHONE
} BHInputTextCommit;
typedef struct BHInputTextCapture
{
    BHEntityPool *pool;
    BHPlayerInputList *input;
    BHInputSnapshot *snapshot;
    BHPlayerInputState *state;
    BHMenuSettings *settings;
    BHMenuSettingsMenu *menu;
    BHMenuProfileContext *profile;
    BHMenuRows *rows;
    BHSoundQueue *sound;
    unsigned long sound_resource;
    char *buffer;
    unsigned long capacity;
    long length, limit;
    BHInputTextCommit commit;
    BHEntity *entity;
    char display[20];
    unsigned long sounds, last_voice, repaints;
    int error;
} BHInputTextCapture;
/* Count-zero setup adapter. All owners/buffer remain borrowed through release.
   Commits dispatch to the three actual source owners, never a callback stub.
   Real cached packet/snapshot state only: no physical input collection. */
int __cdecl bh_input_text_begin(BHInputTextCapture *);
/* Count-zero menu-action setup: allocate first, uppercase the existing buffer,
   paint its trailing17-character cursor, then enable cached input. Paint
   failure preserves the allocated active capture and reports0. */
int __cdecl bh_input_text_begin_painted(BHInputTextCapture *);
/* Whole0042bf80. MapVirtualKeyA mode2 translates source virtual-key values;
   its return deliberately replaces the scan index. Return/Escape set a phase
   but scanning/repainting continues; actual completion happens next step.
   Current length must match a terminated buffer, limit<capacity and fit the
   selected commit record (profile15/address63/phone95). Menu row callbacks must
   retain the capture entity/context until step returns. Native translation
   follows the original ANSI low-byte append; tested host ASCII mapping only.
   Negative/dead-key indices are rejected before the original unsafe lookup. */
int __cdecl bh_input_text_step(BHInputTextCapture *);
int __cdecl bh_input_text_cancel(BHInputTextCapture *);
#endif
