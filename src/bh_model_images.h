#ifndef BH_MODEL_IMAGES_H
#define BH_MODEL_IMAGES_H
#include "bh_model.h"
/* 0045e710: first case-insensitive image name match, skipping NULL entries;
   frame!=0 or signed material count<=0 yields -1. */
long __cdecl bh_model_image_find(const BHModel *, const char *name);
/* 0045e760: replace borrowed image identity only for frame0 and valid signed
   index. NULL image is a permitted replacement; no cache owner is released.
   Source APIs guard NULL model/name/array and return replacement success. */
int __cdecl bh_model_image_replace(BHModel *, long index, BHTextureImage *image);
#endif
