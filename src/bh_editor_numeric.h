#ifndef BH_EDITOR_NUMERIC_H
#define BH_EDITOR_NUMERIC_H
#include "bh_entities.h"
typedef struct BHEditorNumericContext
{
    long selected, entry_state;
} BHEditorNumericContext;
/* Whole0040d000. Eight signed16 model parameters occupy original+334. Mode0
   starts a signed entry,1 appends positive digits,2 appends negative digits.
   Negative key toggles sign. Context is explicit original48f88c/4df258. */
int __cdecl bh_editor_numeric_key(BHEditorNumericContext *, BHEntity *, long key);
#endif
