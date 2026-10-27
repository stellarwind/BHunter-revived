#ifndef BH_ATLAS_WIDE_H
#define BH_ATLAS_WIDE_H
#include "bh_text_queue.h"
/* Additional semantic executor identity for original00477ed0. Existing84byte
   source command layout remains unchanged; caller retains page/resource until
   synchronous execution. Input is16bit glyph codes, not shaping/surrogates. */
#define BH_TEXT_COMMAND_WIDE 4
/* Whole00470ac0 /00470ce0. Signed16metrics,32bit wrapped cursor/advance,
   newline/space distinctions and ignored native Blt results match ANSI owner.
   Text codes32..(31+rectangle_count), including>255, select glyphs directly.
   NULL text is untouched, empty nonnull text still marks residency dirty. */
long __cdecl bh_atlas_page_wide_text(BHAtlasPage *, const unsigned short *);
long __cdecl bh_atlas_page_wide_width(const BHAtlasPage *, const unsigned short *);
/* Whole00478c50. Capacity255/high24 ring words preserved. Copies at most29
   units then NUL, retaining every tail/unused/next byte (no zero padding).
   Coordinates truncate to signed16 on consumption.1queued,0full,-1invalid. */
int __cdecl bh_text_queue_wide(BHTextQueue *, BHAtlasPage *, const unsigned short *, long, long);
/* Whole00477ed0. Current next read after actual cursor/draw calls. Command and
   terminated text must be valid; NULL page is original no-op. */
unsigned long __cdecl bh_text_queue_execute_wide(const BHTextCommand *);
#endif
