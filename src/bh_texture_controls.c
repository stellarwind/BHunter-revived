#include "bh_texture_controls.h"
BHTextureImage *__cdecl bh_texture_image_load(BHTextureContext *context, const char *name)
{
    return bh_texture_load(context, name);
}
int __cdecl bh_texture_image_set_flags(BHTextureImage *image, unsigned long flags)
{
    if (!image)
        return 0;
    image->flags = flags;
    return 1;
}
