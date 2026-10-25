#ifndef BH_TEXT_QUEUE_H
#define BH_TEXT_QUEUE_H
#include "bh_atlas_glyph.h"

typedef enum BHTextCommandKind {
    BH_TEXT_COMMAND_TEXT=1,BH_TEXT_COMMAND_CLEAR=2,BH_TEXT_COMMAND_PLAIN=3
} BHTextCommandKind;
/* Original84-byte record. Semantic kind replaces original function pointer;
   page is borrowed, text includes the original59-byte strncpy region and NUL.
   Unused words and next remain untouched by both recovered producers. */
typedef struct BHTextCommand {
    unsigned long kind;
    BHAtlasPage *page;
    union {short xy[2];unsigned long color;} position;
    unsigned char text[60];
    unsigned long unused[2],next;
} BHTextCommand;
typedef enum BHTextQueueError {BH_TEXT_QUEUE_OK,BH_TEXT_QUEUE_ARGUMENT,BH_TEXT_QUEUE_FULL,BH_TEXT_QUEUE_KIND} BHTextQueueError;
typedef struct BHTextQueue {
    BHTextCommand commands[256];
    unsigned long read,write; /* only low byte participates; high24 bits retained */
    BHTextQueueError error;
} BHTextQueue;
/* Fresh source owner initialization (global BSS equivalent), not an admitted
   original function. Caller serializes producers/consumer and retains pages. */
void __cdecl bh_text_queue_init(BHTextQueue *);
unsigned long __cdecl bh_text_queue_count(const BHTextQueue *);
/* Complete00478bd0 /00478ce0. Return1 queued,0 full,-1 invalid. Capacity255.
   Coordinates retain low16 bits, consumed as signed16. Clear retains text and
   every unused byte; text pads its first59 bytes with NUL as CRT strncpy does. */
int __cdecl bh_text_queue_text(BHTextQueue *,BHAtlasPage *,const char *,long x,long y);
int __cdecl bh_text_queue_clear(BHTextQueue *,BHAtlasPage *,unsigned long color);
/* Complete00477ea0 /00477f00. Execute the real cursor/text or clear owner,
   ignoring its native status as original, then return original record.next.
   Command is live/valid and its text terminated. NULL page is original no-op. */
unsigned long __cdecl bh_text_queue_execute_text(const BHTextCommand *);
unsigned long __cdecl bh_text_queue_execute_clear(BHAtlasContext *,const BHTextCommand *);
/* Source FIFO consumption adapter for the original ring portion of00479090;
   not admission of that complete render callback. This generic adapter accepts
   positioned TEXT and CLEAR only; panel PLAIN records use bh_message_text_drain.
   Ring ignores returned next.
   Returns1 drained,-1 invalid selector/arguments; prior commands stay consumed.
   Native failures remain observable in real borrowed atlas/texture owners. */
int __cdecl bh_text_queue_drain(BHTextQueue *,BHAtlasContext *,unsigned long *processed);
#endif
