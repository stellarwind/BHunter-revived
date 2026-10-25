#ifndef BH_CD_NOTIFY_H
#define BH_CD_NOTIFY_H
#include "bh_cd_close.h"
typedef struct BHCdNotify {
 BHCdClose *cd;
 unsigned long from_track; /* original004dfb64 */
 unsigned long end_track; /* original004dfb60 */
 unsigned long minute_second; /* original004dfb74, low16 bits */
 unsigned long frame; /* original004dfb78, low8 bits */
 DWORD callback; /* original004dfb68 */
 MCI_PLAY_PARMS last;
 MCIERROR last_result;
 unsigned long plays,failures;
} BHCdNotify;
/* Complete00449b90: matching nonzero device, low16 event==1 and playing.
   Repeat MCI_PLAY(flags0x0d) with exact retained range/callback. Does not
   fabricate or open a device, normalize range or mutate playing on failure. */
int __cdecl bh_cd_notify(BHCdNotify *,short event,MCIDEVICEID);
#endif
