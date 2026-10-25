#include "bh_model_images.h"
#include <string.h>
long __cdecl bh_model_image_find(const BHModel *model,const char *name) {
    long i,count;
    if(!model || !name || model->frame || !model->images)return -1;
    count=(long)model->material_count;
    for(i=0;i<count;++i)if(model->images[i] && !_stricmp(name,model->images[i]->name))return i;
    return -1;
}
int __cdecl bh_model_image_replace(BHModel *model,long index,BHTextureImage *image) {
    if(index<0 || !model || model->frame || index>=(long)model->material_count || !model->images)return 0;
    model->images[index]=image;return 1;
}
