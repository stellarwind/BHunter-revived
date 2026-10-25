#include "../src/bh_model_path.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){++failures;printf("FAIL model path %d: %s\n",__LINE__,label);}} while(0)
static void word(unsigned char *p,unsigned long value){memcpy(p,&value,4);}
static int fixture(const char *name,float radius) {
    unsigned char data[272];FILE *f;memset(data,0,sizeof(data));word(data,3);
    strcpy((char *)data+45,"PATHIMG.PCX");word(data+100,1);word(data+164,1);
    word(data+172,0xffffffffUL);word(data+184,1);word(data+188,1);word(data+200,1);
    memcpy(data+216,&radius,4);{float one=1;memcpy(data+228,&one,4);}
    word(data+244,0xffffffffUL);word(data+248,0xffffffffUL);word(data+252,0xffffffffUL);
    word(data+256,0xffffffffUL);word(data+260,1);word(data+268,0xffffffffUL);
    f=fopen(name,"wb");if(!f)return 0;if(fwrite(data,1,sizeof(data),f)!=sizeof(data)){fclose(f);return 0;}return !fclose(f);
}
static int image_fixture(void) {
    unsigned char data[950];unsigned long at=128,left=1024;FILE *f;memset(data,0,sizeof(data));
    data[0]=10;data[1]=2;data[2]=1;data[3]=8;data[8]=31;data[10]=31;data[65]=1;data[66]=32;
    while(left){unsigned long n=left>63?63:left;data[at++]=(unsigned char)(0xc0|n);data[at++]=1;left-=n;}
    data[at+3]=255;at+=768;f=fopen("PATHIMG.PCX","wb");if(!f)return 0;
    if(fwrite(data,1,at,f)!=at){fclose(f);return 0;}return !fclose(f);
}
int bh_model_path_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHModelContext context;BHModel *model;
    BHMathEnvironment fp;char buffer[257],saved[256];long policy;int ready;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real native backend");
    CreateDirectoryA("bhpath-prefix",NULL);
    CHECK(fixture("PATHMODEL.BSP",6) && fixture("bhpath-prefix\\PATHMODEL.BSP",8) && image_fixture(),"distinguishable model and real PCX fixtures");
    if(view.device) {
        ready=bh_texture_context_init(&textures,view.draw,view.device,NULL);
        CHECK(ready && bh_texture_formats(&textures) && bh_model_context_init(&context,&textures,NULL),"real model/cache/native definitions");
        if(ready) {
            for(policy=-2;policy<=3;++policy) {
                CHECK(bh_model_path_set(&context,"bhpath-prefix",policy) && !strcmp(context.prefix,"bhpath-prefix\\") && context.search==(policy>=0 && policy<=2?policy:-1),"original separator and policy clamp");
                model=bh_model_load(&context,"PATHMODEL.BSP");
                CHECK(model && model->radius==(policy==0 || policy==2?8:6) && model->images[1] && model->images[1]->source,"actual model load follows source recovered policy and binds native image");
                bh_model_delete(&context,model);
            }
            CHECK(bh_model_path_set(&context,"bhpath-prefix\\",2) && !strcmp(context.prefix,"bhpath-prefix\\"),"existing separator remains single");
            CHECK(bh_model_path_set(&context,"",2) && !context.prefix[0] && context.search==BH_BSP_BARE_ONLY,"empty original path disables prefix-only");
            model=bh_model_load(&context,"PATHMODEL.BSP");CHECK(model && model->radius==6,"empty prefix executes real bare load");bh_model_delete(&context,model);
            memset(buffer,'A',256);buffer[256]=0;strcpy(saved,context.prefix);
            CHECK(!bh_model_path_set(&context,buffer,0) && context.error==BH_BSP_LIMIT && !strcmp(saved,context.prefix),"overlong guard preserves prior prefix");
            buffer[255]=0;CHECK(!bh_model_path_set(&context,buffer,0) && context.error==BH_BSP_LIMIT,"255chars plus missing separator rejected");
            buffer[254]='\\';CHECK(bh_model_path_set(&context,buffer,1) && strlen(context.prefix)==255,"255chars with separator fit exactly");
            CHECK(!bh_model_path_set(&context,NULL,0) && context.error==BH_BSP_ARGUMENT && !bh_model_path_set(NULL,"",0),"explicit invalid pointer guards");
            bh_texture_context_dispose(&textures);
        }
    }
    remove("PATHMODEL.BSP");remove("PATHIMG.PCX");remove("bhpath-prefix\\PATHMODEL.BSP");RemoveDirectoryA("bhpath-prefix");
    bh_vfile_shutdown();bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"model-path\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifndef BH_MODEL_PATH_LINKED
int main(void){unsigned long count;return bh_model_path_checkpoint(&count);}
#endif
