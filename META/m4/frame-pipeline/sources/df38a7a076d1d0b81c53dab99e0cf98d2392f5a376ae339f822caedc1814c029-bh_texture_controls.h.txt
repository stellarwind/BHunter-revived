#ifndef BH_TEXTURE_CONTROLS_H
#define BH_TEXTURE_CONTROLS_H
#include "bh_texture.h"
/* 004507c0: forwards the image name to the complete cache owner. Current
   context palette mode/color are retained; no alternate policy is selected. */
BHTextureImage *__cdecl bh_texture_image_load(BHTextureContext *context,const char *name);
/* 004582f0: replaces the full flags word at image+0x50. The source guard
   returns zero for NULL; original callers check the load result first. */
int __cdecl bh_texture_image_set_flags(BHTextureImage *image,unsigned long flags);
#endif
