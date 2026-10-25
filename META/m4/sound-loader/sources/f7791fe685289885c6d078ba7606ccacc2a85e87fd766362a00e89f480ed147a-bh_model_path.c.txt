#include "bh_model_path.h"
#include <string.h>
int __cdecl bh_model_path_set(BHModelContext *context,const char *prefix,long policy) {
    unsigned long length=0;
    if(!context)return 0;
    if(!prefix){context->error=BH_BSP_ARGUMENT;return 0;}
    while(length<sizeof(context->prefix) && prefix[length])++length;
    if(length==sizeof(context->prefix) ||
       (length && prefix[length-1]!='\\' && length+1==sizeof(context->prefix))) {
        context->error=BH_BSP_LIMIT;return 0;
    }
    memmove(context->prefix,prefix,length+1);
    if(length && context->prefix[length-1]!='\\') {
        context->prefix[length++]='\\';context->prefix[length]=0;
    }
    context->search=(BHBspSearch)(length && policy>=0 && policy<=2?policy:-1);
    context->error=BH_BSP_OK;return 1;
}
