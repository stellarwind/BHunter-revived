#ifndef BH_MEDIA_CONTROL_H
#define BH_MEDIA_CONTROL_H
#include "bh_cd_audio.h"
typedef struct BHMediaControl {BHCdAudio *audio;unsigned long stops,aux_writes,wave_writes;MCIERROR stop_result;MMRESULT aux_result,wave_result;DWORD aux_packed,wave_packed;} BHMediaControl;
/* Borrowed existing CD/notification/audio owners. Native device ownership is
   unchanged; no acquisition. Absent endpoints preserve prior range/diagnostics.
   Source diagnostics retain actual native arguments and return values. */
int __cdecl bh_cd_play_range(BHMediaControl *,unsigned long from,unsigned long to,unsigned long minute_second,unsigned long frame,long repeat); /*449c20*/
void __cdecl bh_cd_stop(BHMediaControl *); /*449cc0*/
void __cdecl bh_cd_set_volume(BHMediaControl *,float); /*449d40*/
void __cdecl bh_wave_set_volume(BHMediaControl *,float); /*449df0*/
#endif
