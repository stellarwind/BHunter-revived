#ifndef BH_MESSAGE_PANEL_H
#define BH_MESSAGE_PANEL_H
#include "bh_messages.h"
#include "bh_text_queue.h"
#include "bh_sound_queue.h"
#include "bh_screen_quad.h"

typedef struct BHMessagePanelFields {
    float timer,scale,flash;           /* original +e8/+ec/+f0 */
    char *cursor;                      /* original +f4, into message arena */
    unsigned long icon_image;          /* original +f8 */
} BHMessagePanelFields;
typedef enum BHMessagePanelError {
    BH_PANEL_OK,BH_PANEL_ARGUMENT,BH_PANEL_POOL,BH_PANEL_STALE,
    BH_PANEL_MESSAGE,BH_PANEL_TEXT_QUEUE,BH_PANEL_SOUND,BH_PANEL_ICON,
    BH_PANEL_DRAW
} BHMessagePanelError;
typedef struct BHMessagePanel {
    BHEntityPool *pool;
    BHEntity *entity;
    unsigned long generation,mode_identity;
    BHMessageState *messages;
    BHTextQueue *text;
    BHSoundQueue *sound;
    BHDrawQueue *draw;
    BHAtlasPage *page;
    unsigned long icon_images[12];    /* actual tv1..tv9,tva..tvc images */
    unsigned long flash_image,alert_image;
    long screen_width;
    float near_depth;
    unsigned long tv_on_sound,tv_off_sound,tv_char_sound;
    unsigned long sound_requests,text_commands,clear_commands,draw_commands;
    BHMessagePanelError error;
} BHMessagePanel;

BHMessagePanelFields *__cdecl bh_message_panel_fields(BHEntity *);
/* Complete 00478b60 producer: plain queued text with selector 00477e80,
   leaving record coordinates and unused words intact. Return1 queued,0 full,
   -1 invalid source binding. Use bh_message_text_drain for these records. */
int __cdecl bh_message_text_plain(BHTextQueue *,BHAtlasPage *,const char *);
/* Complete original plain consumer 00477e80: text at the page's current
   cursor, then return record+50 next. The panel FIFO adapter handles this
   distinct selector alongside admitted positioned-text and clear consumers. */
unsigned long __cdecl bh_message_text_execute_plain(const BHTextCommand *);
int __cdecl bh_message_text_drain(BHTextQueue *,BHAtlasContext *,unsigned long *processed);
/* Source owner for the startup panel allocation subset and full four-phase
   step chain 00442bf0 -> 00442b60 -> 00442950 -> 004428b0, plus event4 query
   release 00442cd0 and refresh 00442440. Real draw, page, image and sound
   resources are borrowed from their recovered owner. Screen width is the
   original global 004e2998 binding; near_depth is original 0049175c. */
BHEntity *__cdecl bh_message_panel_create(BHMessagePanel *);
int __cdecl bh_message_panel_close(BHMessagePanel *);
int __cdecl bh_message_panel_advance(BHMessagePanel *);
void __cdecl bh_message_panel_step(BHEntity *);
void __cdecl bh_message_panel_query(BHEntity *,void *packet);
int __cdecl bh_message_panel_render(BHMessagePanel *);
void __cdecl bh_message_panel_refresh(BHEntity *,void *packet);
#endif
