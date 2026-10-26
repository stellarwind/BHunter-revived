#ifndef BH_DISPLAY_ENUM_H
#define BH_DISPLAY_ENUM_H
#include <windows.h>
#include <ddraw.h>
/* Source context for original global slots: index0 is primary; indices1..7
   are secondary drivers. secondary_count is not total device count. */
typedef struct BHDisplayCatalog {long secondary_count;char description[8][128];char label[8][128];} BHDisplayCatalog;
typedef struct BHDisplaySelection {const char *requested;LPDIRECTDRAW draw;HRESULT result;unsigned long attempts;} BHDisplaySelection;
/* Whole00472270, exact DDENUMCALLBACKA ABI; borrowed strings/context. */
BOOL CALLBACK bh_display_catalog_callback(GUID *,LPSTR,LPSTR,LPVOID);
/* Whole00472330: matched name stops enumeration even when creation fails.
   Context must have empty draw. Successful root is owned by caller; release
   or transfer before another selection. No coop/fullscreen/window/input calls. */
BOOL CALLBACK bh_display_select_callback(GUID *,LPSTR,LPSTR,LPVOID);
#endif
