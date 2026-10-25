#ifndef BH_PLAYER_INPUT_H
#define BH_PLAYER_INPUT_H
#include "bh_config.h"
#include "bh_scene.h"

/* Input engine provenance remains unresolved. Layout is the original x86
   0x20 record and 0x2130 list; inactive bytes are deliberately preserved. */
typedef struct BHInputBinding {
    unsigned char kind,flags;
    unsigned short modifiers;
    unsigned short key[4];
    char name[20];
} BHInputBinding;

typedef struct BHPlayerInputList {
    struct BHPlayerInputList *previous,*next;
    unsigned long flags;
    long count;
    BHInputBinding bindings[256];
    unsigned char state[0x120];
} BHPlayerInputList;

/* Explicit owner of original global 004ef2fc. The allocator is borrowed and
   must remain valid until disposal. NULL selects real CRT malloc/free. */
typedef struct BHPlayerInputOwner {
    BHPlayerInputList *first;
    BHSceneAllocator allocator;
} BHPlayerInputOwner;

/* Source ownership adapters, not original-function admission claims.
   Initialize once; dispose before reinitializing. */
int __cdecl bh_player_input_owner_init(BHPlayerInputOwner *owner,
                                      const BHSceneAllocator *allocator);
void __cdecl bh_player_input_owner_dispose(BHPlayerInputOwner *owner);

/* 00475640. Allocate, prepend, initialize only active tags and two state bytes.
   Nonzero mode selects flags=2; allocation failure leaves owner unchanged. */
BHPlayerInputList *__cdecl bh_player_input_create(BHPlayerInputOwner *owner,int mode);

/* 004756e0 / 00475770. Callers supply index 0..255 and terminated name.
   Names truncate to 19 characters, followed by NUL. The single-key writer
   preserves key[1..3]. Both retain bytes following the name terminator. */
void __cdecl bh_player_input_bind_axis(BHPlayerInputList *list,long index,
    const char *name,unsigned char flags,unsigned short modifiers,
    unsigned short first,unsigned short second,unsigned short third,unsigned short fourth);
void __cdecl bh_player_input_bind_key(BHPlayerInputList *list,long index,
    const char *name,unsigned char flags,unsigned short modifiers,unsigned short key);
/* 00475810: kind 5 string binding, preserving flags/modifiers/key words. */
void __cdecl bh_player_input_bind_string(BHPlayerInputList *list,long index,const char *name);

/* 00421740. Original current-player +188 and current-profile globals become
   explicit borrowed inputs. No polling, automatic input or callbacks. */
void __cdecl bh_player_input_setup(BHPlayerInputList *list,const BHInputProfile *profile);

typedef struct BHPlayerInputState {unsigned long values[256];} BHPlayerInputState;
/* 00475870: packet[1] counts records starting at +2; packet[0] is ignored.
   Valid packet/storage lifetime is the caller's contract; packet must not
   overlap state. Types0/1 write
   Boolean index batches with four-byte record padding. Types2..4 transfer
   four raw bytes at an index; other tags consume one byte without a write. */
void __cdecl bh_player_input_apply_packet(BHPlayerInputState *state,const void *packet);
/* Source validation adapter: reject truncated packets without changing state.
   The packet must not overlap state. Trailing bytes are permitted. */
int __cdecl bh_player_input_apply_packet_checked(BHPlayerInputState *state,
                                                const void *packet,unsigned long bytes);
/* 00475990: signed-positive counters increment, others decrement, with x86
   32-bit wrap. Result0 first down,2 held down,1 first up,3 held up.
   index must be below256. Packed axis values use another consumer. */
unsigned char __cdecl bh_player_input_transition(BHPlayerInputState *state,unsigned long index);
#endif
