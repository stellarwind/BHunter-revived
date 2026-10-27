#include "bh_loading_progress.h"
#include "bh_vfile.h"

static BHLoadingProgress *binding;

void __cdecl bh_loading_progress(float fraction)
{
    BHLoadingProgress *owner = binding;
    if (!owner)
        return; /* Explicit source guard for an uninstalled callback. */
    ++owner->calls;
    owner->fraction = fraction;
    owner->display_result = bh_loading_display(owner->display, fraction);
}

int __cdecl bh_loading_progress_install(BHLoadingProgress *owner)
{
    if (!owner || !owner->display || (binding && binding != owner))
        return 0;
    binding = owner;
    bh_vfile_set_progress(bh_loading_progress);
    return 1;
}

void __cdecl bh_loading_progress_remove(BHLoadingProgress *owner)
{
    if (owner && binding == owner)
    {
        bh_vfile_set_progress(NULL);
        binding = NULL;
    }
}
