#ifndef BH_CONFIG_H
#define BH_CONFIG_H

/* Recovered x86 payload layout; field semantics beyond the flags/device name
   and profile links remain opaque. Source ownership context is explicit. */
typedef struct BHInputProfile {
    unsigned long flags;
    struct BHInputProfile *next, *previous;
    unsigned char payload[60];
    const struct BHInputProfile *baseline;
} BHInputProfile;
typedef void *(__cdecl *BHConfigAllocate)(unsigned long bytes, void *user);
typedef void (__cdecl *BHConfigRelease)(void *pointer, void *user);
typedef struct BHConfigAllocator {
    BHConfigAllocate allocate;
    BHConfigRelease release;
    void *user;
} BHConfigAllocator;
typedef enum BHConfigError {
    BH_CONFIG_OK, BH_CONFIG_ARGUMENT, BH_CONFIG_IO, BH_CONFIG_FORMAT,
    BH_CONFIG_MEMORY, BH_CONFIG_LIMIT, BH_CONFIG_CONTAINER
} BHConfigError;
typedef struct BHConfig {
    unsigned char settings[416];
    BHInputProfile *profiles;
    unsigned long profile_count;
    int loaded_defaults;
    BHConfigError error;
    /* Reserved source ownership state: do not change after initialization. */
    BHConfigAllocator allocator;
    void *owned_nodes;
} BHConfig;
typedef struct BHDeviceName { char name[128]; } BHDeviceName;

/* Original behaviors 00401fd0, 00402370 and 004022d0, with explicit state/path
   inputs. Only use initialized contexts; initialize once, dispose before reuse.
   On load failure retain the previous valid state; loaded pointer words are
   never followed. Disk payload remains 416 + N*76 bytes, no invented version.
   Current bound is 4096 profiles. Caller owns the allocator's lifetime. */
int __cdecl bh_config_load(BHConfig *config, const char *path);
int __cdecl bh_config_save(BHConfig *config, const char *path);
int __cdecl bh_config_bootstrap(BHConfig *config, const char *path,
                              const BHDeviceName *devices, unsigned long count);

/* Source lifecycle/validation adapters, not original-function claims. */
int __cdecl bh_config_init(BHConfig *config, const BHConfigAllocator *allocator);
void __cdecl bh_config_dispose(BHConfig *config);
int __cdecl bh_config_valid(const BHConfig *config);
/* Count-zero owning adapter for original menu profile cloning. Copy a current
   owned profile's complete76-byte record, borrowing its existing baseline,
   then insert immediately after it in the circular list. The context owns
   and frees the new allocation. Failure preserves ring and profile count;
   caller supplies name/flags and current selection. Profiles and baselines
   remain borrowed until config disposal or replacement by a successful load. */
BHInputProfile *__cdecl bh_config_profile_clone_after(BHConfig *,BHInputProfile *current);
/* Count-zero owning menu adapter. Removes a nondefault member only when more
   than one remains, returns its live previous selection, updates allocation
   tracking/ring/head and frees through the actual allocator. Failure leaves
   ring intact. No borrowed baseline may be freed while referenced. */
BHInputProfile *__cdecl bh_config_profile_remove(BHConfig *,BHInputProfile *);

#endif
