#include "bh_collision_resource.h"
#include <string.h>
int __cdecl bh_collision_resource_path(BHCollisionResourcePath *context,
    const char *prefix,long policy) {
    unsigned long length=0;
    if(!context || !prefix)return 0;
    while(length<sizeof(context->prefix) && prefix[length])++length;
    if(length==sizeof(context->prefix) ||
       (length && prefix[length-1]!='\\' && length+1==sizeof(context->prefix)))return 0;
    context->search=BH_BSP_BARE_ONLY;
    memmove(context->prefix,prefix,length+1);
    if(length) {
        if(context->prefix[length-1]!='\\'){context->prefix[length++]='\\';context->prefix[length]=0;}
        if(policy>=0 && policy<3)context->search=(BHBspSearch)policy;
    }
    return 1;
}
int __cdecl bh_collision_resource_load(const BHCollisionResourcePath *context,
    BHCollisionModel *model,const char *name) {
    if(!context || !model || !name)return 0;
    return bh_collision_model_load(model,name,context->prefix,context->search);
}
