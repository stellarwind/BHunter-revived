#ifndef BH_TEXTURE_POLICY_H
#define BH_TEXTURE_POLICY_H
#include "bh_texture.h"
/* 0044f4f0: raw replacement of palette mode/color. No clamping or masking.
   Explicit context replaces verified globals00491750/00491754. */
int __cdecl bh_texture_policy_set(BHTextureContext *, int mode, unsigned long color);
/* 0044f510: store seven raw words at image+54..6c. Their higher-level role
   remains unresolved; this API preserves raw bit patterns. */
int __cdecl bh_texture_image_set_words(BHTextureImage *, unsigned long word0, unsigned long word1, unsigned long word2,
                                       unsigned long word3, unsigned long word4, unsigned long word5,
                                       unsigned long word6);
#endif
