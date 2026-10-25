#include "../src/bh_model_images.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){++failures;printf("FAIL model images %d: %s\n",__LINE__,label);}} while(0)
static void word(unsigned char *p,unsigned long value){memcpy(p,&value,4);}
static int fixture(const char *name,float radius) {
    unsigned char data[272];FILE *f;memset(data,0,sizeof(data));word(data,3);
    strcpy((char *)data+45,"ACCESSIMG.PCX");word(data+100,1);word(data+164,1);
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
    data[at+3]=255;at+=768;f=fopen("ACCESSIMG.PCX","wb");if(!f)return 0;
    if(fwrite(data,1,at,f)!=at){fclose(f);return 0;}return !fclose(f);
}
int bh_model_images_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHModelContext c;BHModel *model;
    BHTextureImage *image;BHTextureImage **array;BHMathEnvironment fp;unsigned char before[sizeof(BHModel)];
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real native backend");
    CHECK(offsetof(BHModel,material_count)==0x38 && offsetof(BHModel,images)==0x68 && offsetof(BHTextureImage,name)==0x1c,"original model/image offsets");
    CHECK(fixture("ACCESSMODEL.BSP",6) && image_fixture(),"real BSP and PCX fixtures");
    if(view.device) {
        CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures) && bh_model_context_init(&c,&textures,NULL),"actual model/cache/native context");
        model=bh_model_load(&c,"ACCESSMODEL.BSP");CHECK(model && model->images[1] && model->images[1]->source,"real decoded model borrows complete native image");
        if(model && model->images[1]) {
            image=model->images[1];array=model->images;
            CHECK(bh_model_image_find(model,"accessimg.pcx")==1 && bh_model_image_find(model,"AcCeSsImG.PcX")==1,"CRT case-insensitive match skips preceding NULL");
            CHECK(bh_model_image_find(model,"ACCESSIMG.GIF")==-1 && bh_model_image_find(model,"MISSING.PCX")==-1,"name lookup does not invent extension canonicalization");
            memcpy(before,model,sizeof(before));
            CHECK(bh_model_image_replace(model,0,image) && bh_model_image_find(model,"ACCESSIMG.PCX")==0,"duplicate pointer name returns first material index");
            CHECK(!memcmp(before,model,sizeof(before)) && textures.images==image && image->source,"replacement changes borrowed array slot without model/cache lifetime mutation");
            CHECK(bh_model_image_replace(model,0,NULL) && bh_model_image_find(model,"ACCESSIMG.PCX")==1,"NULL replacement clears slot while native cached owner survives");
            CHECK(bh_model_image_replace(model,2,image) && bh_model_image_find(model,"ACCESSIMG.PCX")==1,"duplicate trailing match retains first index");
            CHECK(!bh_model_image_replace(model,-1,image) && !bh_model_image_replace(model,3,image) && !model->images[0],"signed lower and upper bounds reject writes");
            model->frame=1;
            CHECK(bh_model_image_find(model,"ACCESSIMG.PCX")==-1 && !bh_model_image_replace(model,1,NULL) && model->images[1]==image,"nonzero frame freezes both access operations");
            model->frame=0xffffffffUL;
            CHECK(bh_model_image_find(model,"ACCESSIMG.PCX")==-1 && !bh_model_image_replace(model,0,image),"all nonzero frame patterns reject access");
            model->frame=0;model->material_count=0;
            CHECK(bh_model_image_find(model,"ACCESSIMG.PCX")==-1 && !bh_model_image_replace(model,0,image),"empty material set");
            model->material_count=0x80000000UL;
            CHECK(bh_model_image_find(model,"ACCESSIMG.PCX")==-1 && !bh_model_image_replace(model,0,image),"original signed count comparison retained");
            model->material_count=3;model->images=NULL;
            CHECK(bh_model_image_find(model,"ACCESSIMG.PCX")==-1 && !bh_model_image_replace(model,0,image),"source NULL array guards");model->images=array;
            CHECK(bh_model_image_find(NULL,"ACCESSIMG.PCX")==-1 && bh_model_image_find(model,NULL)==-1 && !bh_model_image_replace(NULL,0,image),"source NULL object/name guards");
            CHECK(bh_texture_load(&textures,"ACCESSIMG.JPG")==image,"borrowed replacement and rejected operations preserve real cache identity");
        }
        bh_model_delete(&c,model);CHECK(textures.images && textures.images->source,"model disposal preserves actual cached native image lifetime");
        bh_texture_context_dispose(&textures);
    }
    remove("ACCESSMODEL.BSP");remove("ACCESSIMG.PCX");bh_vfile_shutdown();bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"model-images\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifndef BH_MODEL_IMAGES_LINKED
int main(void){unsigned long count;return bh_model_images_checkpoint(&count);}
#endif
