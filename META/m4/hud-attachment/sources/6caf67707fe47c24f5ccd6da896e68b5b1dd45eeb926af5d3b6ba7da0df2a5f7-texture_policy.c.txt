#include "../src/bh_texture_policy.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){++failures;printf("FAIL texture policy %d: %s\n",__LINE__,label);}} while(0)
static int fixture(const char *name) {
    unsigned char data[950];unsigned long at=128,left=1024;FILE *f;
    memset(data,0,sizeof(data));data[0]=10;data[1]=2;data[2]=1;data[3]=8;
    data[8]=31;data[10]=31;data[65]=1;data[66]=32;
    while(left){unsigned long n=left>63?63:left;data[at++]=(unsigned char)(0xc0|n);data[at++]=1;left-=n;}
    data[at+3]=43;data[at+4]=72;data[at+5]=101;at+=768;
    f=fopen(name,"wb");if(!f)return 0;if(fwrite(data,1,at,f)!=at){fclose(f);return 0;}return !fclose(f);
}
static void pixel(BHTextureImage *image,unsigned long expected) {
    DDSURFACEDESC lock;memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
    CHECK(IDirectDrawSurface_Lock(image->source,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"actual native palette-consumer pixels");
    if(lock.lpSurface){CHECK(*(unsigned char *)lock.lpSurface==expected,"exact policy/color selects independently expected index");IDirectDrawSurface_Unlock(image->source,NULL);}
}
int bh_texture_policy_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext q;BHTextureImage *first,*second;
    BHMathEnvironment fp;unsigned char before[sizeof(BHTextureImage)];
    unsigned long words[7]={0,0xffffffffUL,0x80000000UL,0x12345678UL,4,5,6};
    long modes[5]={0,1,2,3,-1};unsigned long i;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real native backend");
    CHECK(offsetof(BHTextureImage,unused54)==0x54 && offsetof(BHTextureImage,padding)==0x58 && sizeof(BHTextureImage)==0x70,"seven-word original layout");
    CHECK(!bh_texture_policy_set(NULL,2,0) && !bh_texture_image_set_words(NULL,0,0,0,0,0,0,0),"source NULL guards");
    CHECK(fixture("POLICY_A.PCX") && fixture("POLICY_B.PCX"),"actual PCX source fixtures");
    if(view.device)for(i=0;i<5;++i) {
        CHECK(bh_texture_context_init(&q,view.draw,view.device,NULL) && bh_texture_formats(&q),"actual cache/backend definitions");
        CHECK(bh_texture_policy_set(&q,(int)modes[i],0x2b4865) && q.palette_mode==modes[i] && q.palette_color==0x2b4865,"raw mode/color replaced without clamp");
        first=bh_texture_load(&q,"POLICY_A.PCX");CHECK(first && first->source && first->palette && first->vertices && first->runs,"complete real native/cache image consumer");
        if(first) {
            pixel(first,modes[i]!=0 && modes[i]!=2?0:1);
            memcpy(before,first,sizeof(before));
            CHECK(bh_texture_image_set_words(first,words[0],words[1],words[2],words[3],words[4],words[5],words[6]),"all seven caller words accepted");
            memcpy(before+0x54,words,sizeof(words));CHECK(!memcmp(before,first,sizeof(before)),"only exact54..6c words change, real ownership and flags remain");
            CHECK(bh_texture_policy_set(&q,0,0xffffffffUL) && q.palette_color==0xffffffffUL,"color is a raw unmasked32-bit word");
            CHECK(bh_texture_load(&q,"POLICY_A.GIF")==first,"policy change does not invalidate original name alias cache");pixel(first,modes[i]!=0 && modes[i]!=2?0:1);
            second=bh_texture_load(&q,"POLICY_B.PCX");CHECK(second && second!=first,"new name consumes updated palette policy");if(second)pixel(second,1);
        }
        bh_texture_context_dispose(&q);
    }
    remove("POLICY_A.PCX");remove("POLICY_B.PCX");bh_vfile_shutdown();bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"texture-policy\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifndef BH_TEXTURE_POLICY_LINKED
int main(void){unsigned long count;return bh_texture_policy_checkpoint(&count);}
#endif
