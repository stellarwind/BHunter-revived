#ifndef BH_RENDER_MATERIAL_H
#define BH_RENDER_MATERIAL_H
#include "bh_texture.h"
/* Explicit source owner for the otherwise leaked local material in0045b1d0.
   Zero initialize before use. The created material remains alive until dispose;
   borrowed D3D2 and texture context need only remain live during initialize. */
typedef struct BHRenderMaterial
{
    LPDIRECT3DMATERIAL2 material;
    D3DMATERIALHANDLE handle;
    HRESULT last_hresult, first_failure;
    unsigned long native_calls, failures;
} BHRenderMaterial;
/* Complete0045b1d0 over real SDK interfaces. Returns NULL when the original
   shared error string is empty, otherwise that string. Software mode returns
   NULL immediately. Preallocation error appends and still initializes native
   material/state. Only CreateMaterial HRESULT gates the original control flow;
   later native HRESULTs are recorded separately without changing its return.
   Source guards return a literal diagnostic for invalid/previously live owner;
   bounded error append replaces original unbounded strcat. */
const char *__cdecl bh_render_material_initialize(BHRenderMaterial *, BHTextureContext *, LPDIRECT3D2);
/* Source lifetime adapter, not a recovered original destructor. */
void __cdecl bh_render_material_dispose(BHRenderMaterial *);
#endif
