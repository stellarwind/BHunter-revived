#ifndef BH_CONFIG_H
#define BH_CONFIG_H

/* Recovered x86 profile layout. The byte view is retained for exact disk I/O. */
typedef struct BHInputProfile
{
    unsigned long flags;
    struct BHInputProfile *next, *previous;
    union {
        unsigned char payload[60];
        struct
        {
            char name[16];
            unsigned char motion_keys[6], motion_joystick[3], motion_mouse[3];
            unsigned char primary_keys[9], secondary_keys[9], camera_keys[10];
            unsigned char camera_joystick[2], camera_mouse[2];
        };
    };
    const struct BHInputProfile *baseline;
} BHInputProfile;
typedef void *(*BHConfigAllocate)(unsigned long bytes, void *user);
typedef void (*BHConfigRelease)(void *pointer, void *user);
typedef struct BHConfigAllocator
{
    BHConfigAllocate allocate;
    BHConfigRelease release;
    void *user;
} BHConfigAllocator;
typedef enum BHConfigError
{
    BH_CONFIG_OK,
    BH_CONFIG_ARGUMENT,
    BH_CONFIG_IO,
    BH_CONFIG_FORMAT,
    BH_CONFIG_MEMORY,
    BH_CONFIG_LIMIT,
    BH_CONFIG_CONTAINER
} BHConfigError;
typedef struct BHConfigSettings
{
    unsigned short hardware_flags;
    short unlocked_mission;
    long hardware_acceleration, resolution;
    char device_name[128];
    unsigned char reserved[276];
} BHConfigSettings;

typedef struct BHConfig
{
    union {
        unsigned char settings[416];
        BHConfigSettings values;
    };
    BHInputProfile *profiles;
    unsigned long profile_count;
    int loaded_defaults;
    BHConfigError error;
    /* Reserved source ownership state: do not change after initialization. */
    BHConfigAllocator allocator;
    void *owned_nodes;
} BHConfig;
typedef struct BHDeviceName
{
    char name[128];
} BHDeviceName;

/* Original behaviors 00401fd0, 00402370 and 004022d0, with explicit state/path
   inputs. Only use initialized contexts; initialize once, dispose before reuse.
   On load failure retain the previous valid state; loaded pointer words are
   never followed. Disk payload remains 416 + N*76 bytes, no invented version.
   Current bound is 4096 profiles. Caller owns the allocator's lifetime. */
int bh_config_load(BHConfig *config, const char *path);
int bh_config_save(BHConfig *config, const char *path);
int bh_config_bootstrap(BHConfig *config, const char *path, const BHDeviceName *devices, unsigned long count);

/* Source lifecycle/validation adapters, not original-function claims. */
int bh_config_init(BHConfig *config, const BHConfigAllocator *allocator);
void bh_config_dispose(BHConfig *config);
int bh_config_valid(const BHConfig *config);
/* Count-zero owning adapter for original menu profile cloning. Copy a current
   owned profile's complete76-byte record, borrowing its existing baseline,
   then insert immediately after it in the circular list. The context owns
   and frees the new allocation. Failure preserves ring and profile count;
   caller supplies name/flags and current selection. Profiles and baselines
   remain borrowed until config disposal or replacement by a successful load. */
BHInputProfile *bh_config_profile_clone_after(BHConfig *, BHInputProfile *current);
/* Count-zero owning menu adapter. Removes a nondefault member only when more
   than one remains, returns its live previous selection, updates allocation
   tracking/ring/head and frees through the actual allocator. Failure leaves
   ring intact. No borrowed baseline may be freed while referenced. */
BHInputProfile *bh_config_profile_remove(BHConfig *, BHInputProfile *);

#endif
