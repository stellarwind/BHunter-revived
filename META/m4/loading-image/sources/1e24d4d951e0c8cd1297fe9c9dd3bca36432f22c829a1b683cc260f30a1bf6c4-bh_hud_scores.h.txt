#ifndef BH_HUD_SCORES_H
#define BH_HUD_SCORES_H
#include "bh_text_queue.h"
#include "bh_pickup_draw.h"

/* Borrowed equivalents of the actual eight004dfad8 remote slots, local name,
   signed local score004dfac0 and local slot0048f9c0. Remote signed16-bit score
   and terminated name live at entity+11c/+11e. This binding does not create a
   network session, remote actor, font page or gameplay callback. */
typedef struct BHHudScores {
    BHEntity *const *remote;
    const char *local_name;
    long local_score,local_index;
} BHHudScores;
typedef enum BHHudScoresError {
    BH_HUD_SCORES_OK,BH_HUD_SCORES_ARGUMENT,BH_HUD_SCORES_QUEUE
} BHHudScoresError;
typedef struct BHHudScoreText {
    BHTextQueue *queue;BHAtlasPage *page;
    unsigned long attempted,queued,dropped;
    BHHudScoresError error;
} BHHudScoreText;
typedef struct BHHudScoreDraw {
    BHDrawQueue *queue;unsigned long page_image;
    long display_width;float near_z;
    unsigned long attempted,queued,dropped;
    BHHudScoresError error;
} BHHudScoreDraw;
/* Complete004275c0: one clear, then name/decimal-score pairs for every present
   remote slot in index order, then local name/score. A populated local remote
   slot is deliberately not excluded here. Score strings use verified CRT
   sprintf("%d"), signed16-bit remote versus signed32-bit local. Right align at
   x=126-width; existing queue truncates xy to signed16 bits and text to59bytes.
   Full ring drops remain success; no queued command executes synchronously. */
int __cdecl bh_hud_scores_text(BHHudScoreText *,const BHHudScores *);
/* Complete00427710: descending distinct scores strictly between the initial
   -100000/100000 bounds; all remote ties emit in slot order, then local tie.
   Local remote slot is excluded here; local row modulation1, others.5. Width
   is signed width/40, x is signed width/2, depth is live near_z+double.01.
   All producers are real bucket3 UV rectangles; full queue drops continue.
   Inputs/page/actors survive queue consumption; serialize mutations externally.
   PC53 nearest/masked finite near-plane profile, no gameplay execution. */
int __cdecl bh_hud_scores_draw(BHHudScoreDraw *,const BHHudScores *);
#endif
