#ifndef BH_INPUT_COLLECTION_H
#define BH_INPUT_COLLECTION_H
#include "bh_input_evaluate.h"
typedef enum BHInputCollectionError
{
    BH_INPUT_COLLECTION_OK,
    BH_INPUT_COLLECTION_ARGUMENT,
    BH_INPUT_COLLECTION_TOPOLOGY,
    BH_INPUT_COLLECTION_EVALUATE
} BHInputCollectionError;
typedef struct BHInputCollection
{
    BHInputSnapshot *snapshot;
    BHPlayerInputOwner *lists;
    BHInputCollectionError error;
    float mouse_sensitivity;
    int mouse_invert_y;
    int disable_mouse_motion; /* explicit source preference; buttons remain live */
} BHInputCollection;
int bh_input_collection_init(BHInputCollection *, BHInputSnapshot *, BHPlayerInputOwner *);
int bh_input_collect(BHInputCollection *, unsigned long *changed);
#endif
