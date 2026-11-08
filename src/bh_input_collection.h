#ifndef BH_INPUT_COLLECTION_H
#define BH_INPUT_COLLECTION_H
#include "bh_input_evaluate.h"
typedef enum BHInputCollectionError
{
    BH_INPUT_COLLECTION_OK,
    BH_INPUT_COLLECTION_ARGUMENT,
    BH_INPUT_COLLECTION_TOPOLOGY,
    BH_INPUT_COLLECTION_DEVICE,
    BH_INPUT_COLLECTION_EVALUATE
} BHInputCollectionError;
typedef struct BHInputCollection
{
    BHInputSnapshot *snapshot;
    BHPlayerInputOwner *lists;
    long delta[2];
    int last_cursor, last_recenter;
    BHInputCollectionError error;
    int disable_mouse_motion; /* explicit source preference; buttons remain live */
} BHInputCollection;
int bh_input_collection_init(BHInputCollection *, BHInputSnapshot *, BHPlayerInputOwner *);
int bh_input_collect(BHInputCollection *, unsigned long *changed);
#endif
