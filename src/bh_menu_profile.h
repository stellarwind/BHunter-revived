#ifndef BH_MENU_PROFILE_H
#define BH_MENU_PROFILE_H
#include "bh_config.h"
#include "bh_menu_settings.h"
typedef struct BHMenuProfileContext
{
    BHConfig *config;
    BHInputProfile **current; /* actual selected profile, borrowed */
    long *pending_selection;  /* original parent menu+1d8 */
    int error;
} BHMenuProfileContext;
/* Whole0042de70. Case-insensitive circular search starts at current. A new
   profile copies all76bytes and inserts after current through actual BHConfig
   ownership; flags becomes1 and name has <=15 bytes plus NUL. Nonempty input
   refreshes row0 after successful select/clone, clears pending_selection even
   after allocation failure, then refreshes selected row. Empty input only
   refreshes selected row. Rows/callbacks are read live for each call.
   All owners must outlive callbacks; callback may modify menu row bindings,
   but may not dispose config/current storage or invalidate the new profile. */
int __cdecl bh_menu_profile_commit(BHMenuProfileContext *, BHMenuSettingsMenu *, const char *, long);
#endif
