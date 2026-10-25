#include "../src/bh_render_material.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do{++checks;if(!(x)){++failures;printf("FAIL render-material %d: %s\n",__LINE__,label);}}while(0)
typedef struct Allocation {int fail;} Allocation;
static void *__cdecl allocate(unsigned long n,void *user) {return ((Allocation *)user)->fail?NULL:malloc(n);}
static void __cdecl release(void *p,void *user) {free(p);}
static void verify_native(BHRenderMaterial *owner,BHTextureContext *q) {
    D3DMATERIAL material;DWORD value;unsigned long i;
    static const D3DRENDERSTATETYPE states[13]={
        D3DRENDERSTATE_ZENABLE,D3DRENDERSTATE_ZFUNC,D3DRENDERSTATE_ZWRITEENABLE,D3DRENDERSTATE_SPECULARENABLE,
        D3DRENDERSTATE_SHADEMODE,D3DRENDERSTATE_DITHERENABLE,D3DRENDERSTATE_TEXTUREPERSPECTIVE,D3DRENDERSTATE_TEXTUREMAPBLEND,
        D3DRENDERSTATE_TEXTUREHANDLE,D3DRENDERSTATE_TEXTUREMAG,D3DRENDERSTATE_TEXTUREMIN,D3DRENDERSTATE_CULLMODE,D3DRENDERSTATE_COLORKEYENABLE};
    /* Independent SDK semantic names establish the recovered numeric constants. */
    static const DWORD values[13]={TRUE,D3DCMP_GREATEREQUAL,TRUE,TRUE,D3DSHADE_GOURAUD,TRUE,TRUE,D3DTBLEND_MODULATE,0,D3DFILTER_LINEAR,D3DFILTER_LINEAR,D3DCULL_CW,TRUE};
    CHECK(owner->material && owner->handle && owner->native_calls==17,"real material, real device handle, complete original native call sequence");
    CHECK(!owner->failures && owner->first_failure==DD_OK && owner->last_hresult==DD_OK,"all actual native HRESULTs succeeded on RGB backend");
    memset(&material,0,sizeof(material));material.dwSize=sizeof(material);
    CHECK(IDirect3DMaterial2_GetMaterial(owner->material,&material)==DD_OK,"native SDK reads back actual owned material");
    CHECK(material.dwSize==0x50 && material.diffuse.r==1 && material.diffuse.g==1 && material.diffuse.b==1 && material.diffuse.a==0,"original diffuse white with zero alpha");
    CHECK(material.ambient.r==1 && material.ambient.g==1 && material.ambient.b==1 && material.ambient.a==0,"original ambient white with zero alpha");
    CHECK(material.specular.r==1 && material.specular.g==1 && material.specular.b==1 && material.specular.a==0,"original specular white with zero alpha");
    CHECK(!material.emissive.r && !material.emissive.g && !material.emissive.b && !material.emissive.a && material.power==40 && !material.hTexture && material.dwRampSize==1,"emissive/power/texture/ramp complete material values");
    value=0;CHECK(IDirect3DDevice2_GetLightState(q->device,D3DLIGHTSTATE_MATERIAL,&value)==DD_OK && value==owner->handle,"actual device selected created material handle");
    for(i=0;i<13;++i){value=0xdeadbeef;CHECK(IDirect3DDevice2_GetRenderState(q->device,states[i],&value)==DD_OK && value==values[i],"actual native state matches original default");}
}
int bh_render_material_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext q;BHRenderMaterial owner;BHTextureFormat *format;BHMathEnvironment fp;
    Allocation allocation;BHSceneAllocator allocator;const char *message;unsigned long n;int initialized;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    CHECK(sizeof(D3DMATERIAL)==0x50 && offsetof(D3DMATERIAL,power)==0x44 && offsetof(D3DMATERIAL,dwRampSize)==0x4c,"official native material ABI");
    memset(&owner,0,sizeof(owner));memset(&q,0,sizeof(q));q.software=1;strcpy(q.errors,"retained diagnostic");
    CHECK(!bh_render_material_initialize(&owner,&q,NULL)&&!owner.material&&!owner.native_calls&&!strcmp(q.errors,"retained diagnostic"),"software branch exits before native bindings and ignores existing error log");
    CHECK(bh_render_material_initialize(&owner,NULL,NULL)!=NULL,"explicit NULL source guard");
    memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"real native offscreen RGB backend");
    if(view.device)for(n=0;n<3;++n) {
        memset(&owner,0,sizeof(owner));allocation.fail=n==1;
        allocator.allocate=allocate;allocator.release=release;allocator.user=&allocation;
        initialized=bh_texture_context_init(&q,view.draw,view.device,&allocator);
        CHECK(initialized && bh_texture_formats(&q),"real texture context and native format enumeration");
        if(!initialized)continue;
        q.slot_caps_override=DDSCAPS_SYSTEMMEMORY|DDSCAPS_TEXTURE;
        format=bh_texture_request_format(&q,32,32,16,1);CHECK(format!=NULL,"real required texture slot request");
        if(n==2)strcpy(q.errors,"earlier diagnostic;");
        message=bh_render_material_initialize(&owner,&q,view.d3d);
        if(n==0)CHECK(!message && format && format->count==1 && format->slots && format->slots->surface && format->slots->handle,"success allocates actual native texture slot before material");
        if(n==1)CHECK(message==q.errors && !strcmp(message," VMEMERR: Alloc texture slots failed. ") && q.error==BH_TEXTURE_MEMORY && format && !format->count,"real allocation failure appends exact original error but still initializes native material");
        if(n==2)CHECK(message==q.errors && !strcmp(message,"earlier diagnostic;") && format && format->count==1,"existing shared diagnostic returned unchanged after successful native initialization");
        verify_native(&owner,&q);
        CHECK(bh_render_material_initialize(&owner,&q,view.d3d)!=NULL && owner.native_calls==17,"source guard prevents repeated initialization leaking owned material");
        bh_render_material_dispose(&owner);CHECK(!owner.material&&!owner.handle&&!owner.native_calls,"explicit source release adapter clears owned interface and diagnostics");
        bh_render_material_dispose(&owner);bh_texture_context_dispose(&q);
    }
    bh_scene_view_dispose(&view);CHECK(bh_math_environment_supported(),"native material checks preserve admitted FP profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"render-material\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_RENDER_MATERIAL_STANDALONE
int main(void){unsigned long count;return bh_render_material_checkpoint(&count);}
#endif
