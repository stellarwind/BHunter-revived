#ifndef BH_MEDIA_CONTROL_H
#define BH_MEDIA_CONTROL_H
#include "bh_cd_audio.h"
typedef struct BHMediaControl
{
    BHCdAudio *audio;
    unsigned long stops, aux_writes, wave_writes;
    int stop_result;
    int aux_result, wave_result;
    unsigned long aux_packed, wave_packed;
} BHMediaControl;
/* Game-local playback and gain controls. */
int bh_cd_play_range(BHMediaControl *, unsigned long from, unsigned long to, unsigned long minute_second,
                     unsigned long frame, long repeat); /*449c20*/
void bh_cd_stop(BHMediaControl *);                      /*449cc0*/
void bh_cd_set_volume(BHMediaControl *, float);         /*449d40*/
void bh_wave_set_volume(BHMediaControl *, float);       /*449df0*/
#endif
