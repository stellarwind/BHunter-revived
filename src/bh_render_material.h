#ifndef BH_RENDER_MATERIAL_H
#define BH_RENDER_MATERIAL_H
#include "bh_texture.h"
/* Source-owned initialization status. Lighting is baked into transformed
   vertices; the SDL renderer needs the recovered render state, not a material
   interface or a dummy material allocation. Zero initialize before use. */
typedef struct BHRenderMaterial
{
    unsigned long initialized;
    HRESULT last_hresult;
    unsigned long native_calls, failures;
} BHRenderMaterial;
/* Preallocate texture slots and install the original render-state values.
   Returns the shared error string, or NULL on success/software rendering. */
const char *bh_render_material_initialize(BHRenderMaterial *, BHTextureContext *, BHRenderer *);
/* Source lifetime adapter, not a recovered original destructor. */
void bh_render_material_dispose(BHRenderMaterial *);
#endif
