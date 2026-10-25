#include "bh_texture_policy.h"
int __cdecl bh_texture_policy_set(BHTextureContext *context,int mode,unsigned long color) {
    if(!context)return 0;
    context->palette_mode=mode;context->palette_color=color;return 1;
}
int __cdecl bh_texture_image_set_words(BHTextureImage *image,unsigned long word0,
    unsigned long word1,unsigned long word2,unsigned long word3,
    unsigned long word4,unsigned long word5,unsigned long word6) {
    if(!image)return 0;
    image->unused54=word0;image->padding[0]=word1;image->padding[1]=word2;
    image->padding[2]=word3;image->padding[3]=word4;image->padding[4]=word5;
    image->padding[5]=word6;return 1;
}
