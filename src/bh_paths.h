#ifndef BH_PATHS_H
#define BH_PATHS_H

typedef struct BHPaths
{
    char ini[256], archive[256];
} BHPaths;
/* 00473910 behavior with explicit directory inputs. Initialize paths to zero
   or an existing valid state before use. NULL Windows directory
   leaves ini unchanged, as on failed GetWindowsDirectoryA. Returns 2 on
   success, 0 for invalid/oversize paths; failure does not change output.
   Preserve observed backslash-only separator test, case and relative paths. */
int __cdecl bh_paths_init(BHPaths *paths, const char *windows_directory, const char *current_directory);
/* Source adapters: CFG falls back to BHUNTER.CFG if Windows discovery fails;
   native discovery reads real Win32 directories and never changes the cwd. */
int __cdecl bh_paths_config(char *output, unsigned long capacity, const char *windows_directory);
int __cdecl bh_paths_discover(BHPaths *paths);

#endif
