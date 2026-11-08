#ifndef BH_INPUT_DEVICES_H
#define BH_INPUT_DEVICES_H
#include "bh_input_collection.h"
typedef struct BHInputDevices
{
    BHInputCollection *collection;
    const char *names[255];
    unsigned char allocated[255];
    BHSceneAllocator allocator;
    unsigned long allocation_failures;
} BHInputDevices;
/* Source ownership adapter. Fresh owner; collector/snapshot/list storage are
   borrowed. Seeds exactly255 static original key-name entries; key255 has no
   initialized table slot. NULL allocator selects actual CRT malloc/free. */
int bh_input_devices_bind(BHInputDevices *, BHInputCollection *, const BHSceneAllocator *);
int bh_input_devices_start(BHInputDevices *);
void bh_input_devices_dispose(BHInputDevices *);
#endif
