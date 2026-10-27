#ifndef BH_GDI_TEXT_H
#define BH_GDI_TEXT_H
#include "bh_frame_render.h"
#include "bh_draw_queue.h"
#define BH_DRAW_GDI_TEXT 19UL
typedef struct BHGdiText
{
    BHFrameDisplay *display;
    HFONT font;
    long relock;
    unsigned long native_calls, failures;
    HRESULT last_hresult;
} BHGdiText;
/* Text is bounded to58 ANSI bytes plus NUL; original strncpy59 leaves no
   terminator for longer inputs. Commands retain copied text and signed16XY.
   Caller owns surface and optional lock; no screen DC or capture is used.
   Font is created lazily by whole consumer; dispose is count-zero ownership. */
int __cdecl bh_gdi_text_queue(BHDrawQueue *, short, short, const char *, unsigned long); /*478890*/
int __cdecl bh_gdi_text_execute(BHGdiText *, const BHDrawCommand *, unsigned long *);    /*477d20*/
int __cdecl bh_editor_message_queue(BHDrawQueue *);                                      /*40e050*/
unsigned long __cdecl bh_draw_command_next(const BHDrawCommand *);                       /*477380*/
void __cdecl bh_gdi_text_dispose(BHGdiText *);
#endif
