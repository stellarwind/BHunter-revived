#include "bh_render_material.h"
#include <string.h>

static void append(BHTextureContext *q,const char *message) {
    unsigned long n=(unsigned long)strlen(q->errors),left=(unsigned long)sizeof(q->errors)-n-1;
    if(left){strncpy(q->errors+n,message,left);q->errors[n+left]=0;}
}
static void native(BHRenderMaterial *owner,HRESULT hr) {
    ++owner->native_calls;owner->last_hresult=hr;
    if(hr!=DD_OK){if(!owner->failures)owner->first_failure=hr;++owner->failures;}
}
const char *__cdecl bh_render_material_initialize(BHRenderMaterial *owner,BHTextureContext *q,LPDIRECT3D2 d3d) {
    D3DMATERIAL material;HRESULT hr;unsigned long i;
    static const D3DRENDERSTATETYPE states[13]={
        D3DRENDERSTATE_ZENABLE,D3DRENDERSTATE_ZFUNC,D3DRENDERSTATE_ZWRITEENABLE,
        D3DRENDERSTATE_SPECULARENABLE,D3DRENDERSTATE_SHADEMODE,D3DRENDERSTATE_DITHERENABLE,
        D3DRENDERSTATE_TEXTUREPERSPECTIVE,D3DRENDERSTATE_TEXTUREMAPBLEND,D3DRENDERSTATE_TEXTUREHANDLE,
        D3DRENDERSTATE_TEXTUREMAG,D3DRENDERSTATE_TEXTUREMIN,D3DRENDERSTATE_CULLMODE,D3DRENDERSTATE_COLORKEYENABLE};
    static const DWORD values[13]={1,7,1,1,2,1,1,2,0,2,2,2,1};
    if(!q)return "Invalid render material context.";
    if(q->software)return NULL;
    if(!owner || owner->material || !d3d || !q->device || !q->draw || !q->allocator.allocate || !q->allocator.release)return "Invalid render material binding.";
    for(i=0;i<sizeof(q->errors) && q->errors[i];++i){}
    if(i==sizeof(q->errors))return "Unterminated render error string.";
    owner->handle=0;owner->last_hresult=owner->first_failure=DD_OK;owner->native_calls=owner->failures=0;
    if(!bh_texture_preallocate(q))append(q," VMEMERR: Alloc texture slots failed. ");
    hr=IDirect3D2_CreateMaterial(d3d,&owner->material,NULL);native(owner,hr);
    if(hr!=DD_OK) {
        append(q," D3DERR: Create default material failed. ");return q->errors;
    }
    memset(&material,0,sizeof(material));material.dwSize=sizeof(material);
    material.diffuse.r=material.diffuse.g=material.diffuse.b=1;
    material.ambient.r=material.ambient.g=material.ambient.b=1;
    material.specular.r=material.specular.g=material.specular.b=1;
    material.power=40;material.dwRampSize=1;
    native(owner,IDirect3DMaterial2_SetMaterial(owner->material,&material));
    native(owner,IDirect3DMaterial2_GetHandle(owner->material,q->device,&owner->handle));
    for(i=0;i<13;++i) {
        /* Original binds material between dither and perspective states. */
        if(i==6)native(owner,IDirect3DDevice2_SetLightState(q->device,D3DLIGHTSTATE_MATERIAL,owner->handle));
        native(owner,IDirect3DDevice2_SetRenderState(q->device,states[i],values[i]));
    }
    return q->errors[0]?q->errors:NULL;
}
void __cdecl bh_render_material_dispose(BHRenderMaterial *owner) {
    if(!owner)return;
    if(owner->material)IDirect3DMaterial2_Release(owner->material);
    memset(owner,0,sizeof(*owner));
}
