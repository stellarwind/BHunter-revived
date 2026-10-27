#ifndef BH_LOADING_PROGRESS_H
#define BH_LOADING_PROGRESS_H
#include "bh_loading_display.h"

typedef struct BHLoadingProgress
{
    BHLoadingDisplay *display;
    unsigned long calls;
    float fraction;
    int display_result;
} BHLoadingProgress;

/* Whole 004791f0: the cdecl float archive callback forwards to the real
   loading display owner. The explicit binding replaces original global state.
   Install/remove are lifecycle adapters, not additional original owners.
   One installation owns the existing process-wide VFILE callback slot;
   remove it before disposing the borrowed display/images/native surfaces. */
void __cdecl bh_loading_progress(float fraction);
int __cdecl bh_loading_progress_install(BHLoadingProgress *);
void __cdecl bh_loading_progress_remove(BHLoadingProgress *);
#endif
