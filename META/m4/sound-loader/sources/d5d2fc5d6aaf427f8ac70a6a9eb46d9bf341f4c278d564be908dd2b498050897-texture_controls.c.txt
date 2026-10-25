#include "../src/bh_texture_controls.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){++failures;printf("FAIL texture controls %d: %s\n",__LINE__,label);}} while(0)
static int fixture(void) {
    unsigned char data[950];unsigned long at=128,left=1024;FILE *f;
    memset(data,0,sizeof(data));data[0]=10;data[1]=2;data[2]=1;data[3]=8;
    data[8]=31;data[10]=31;data[65]=1;data[66]=32;
    while(left){unsigned long n=left>63?63:left;data[at++]=(unsigned char)(0xc0|n);data[at++]=1;left-=n;}
    memset(data+at,0,768);data[at+3]=43;data[at+4]=72;data[at+5]=101;at+=768;
    f=fopen("CTLIMAGE.PCX","wb");if(!f)return 0;
    if(fwrite(data,1,at,f)!=at){fclose(f);return 0;}return !fclose(f);
}
int bh_texture_controls_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext q;BHTextureImage *image,*alias;
    BHMathEnvironment fp;PALETTEENTRY entry;DDSURFACEDESC lock;unsigned char before[sizeof(BHTextureImage)];
    unsigned long modes[4]={0,1,2,3},i;int initialized;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real native backend");
    CHECK(offsetof(BHTextureImage,flags)==0x50 && sizeof(BHTextureImage)==0x70,"original x86 flag layout");
    CHECK(!bh_texture_image_set_flags(NULL,2),"explicit source NULL guard");
    CHECK(fixture(),"real PCX source fixture");
    if(view.device)for(i=0;i<4;++i) {
        initialized=bh_texture_context_init(&q,view.draw,view.device,NULL);
        CHECK(initialized && bh_texture_formats(&q),"actual cache/backend context");
        if(!initialized)continue;
        q.palette_mode=(int)modes[i];q.palette_color=0x2b4865;
        image=bh_texture_image_load(&q,"ctlimage.tga");
        CHECK(image && image->source && image->palette && image->runs && image->vertices,"wrapper loads real native image and source arenas");
        CHECK(q.palette_mode==(int)modes[i] && q.palette_color==0x2b4865,"wrapper preserves current palette mode/color");
        if(image) {
            CHECK(IDirectDrawPalette_GetEntries(image->palette,0,1,1,&entry)==DD_OK,"real native palette available");
            memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
            CHECK(IDirectDrawSurface_Lock(image->source,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)==DD_OK,"actual palette policy native pixels");
            if(lock.lpSurface) {
                CHECK(*(unsigned char *)lock.lpSurface==((i==1 || i==3)?0:1),"wrapper honors current mode and exact keyed color through real loader");
                IDirectDrawSurface_Unlock(image->source,NULL);
            }
            image->flags=0x80000001UL;memcpy(before,image,sizeof(before));
            CHECK(bh_texture_image_set_flags(image,2) && image->flags==2,"original caller replaces flags1 with2 rather than OR");
            memcpy(before+offsetof(BHTextureImage,flags),&image->flags,sizeof(image->flags));
            CHECK(!memcmp(before,image,sizeof(before)),"only flags word changes, all real ownership and arenas retained");
            CHECK(bh_texture_image_set_flags(image,0xffffffffUL) && image->flags==0xffffffffUL &&
                  bh_texture_image_set_flags(image,0) && image->flags==0,"raw full word accepts high bits and zero");
            q.palette_mode=0;q.palette_color=0;
            alias=bh_texture_image_load(&q,"CTLIMAGE.JPG");
            CHECK(alias==image && alias->flags==0 && q.images==image && !image->next,"cache alias retains explicit flags and owner identity");
        }
        CHECK(!bh_texture_image_load(&q,"CTL_MISSING.PCX") && q.error==BH_TEXTURE_IO,"wrapper preserves real loader error");
        bh_texture_context_dispose(&q);
    }
    remove("CTLIMAGE.PCX");bh_vfile_shutdown();bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);
    *count=checks;printf("{\"scenario\":\"texture-controls\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifndef BH_TEXTURE_CONTROLS_LINKED
int main(void){unsigned long count;return bh_texture_controls_checkpoint(&count);}
#endif
