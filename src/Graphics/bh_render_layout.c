#include "bh_render_types.h"
#include "Textures/bh_texture.h"
#include <stddef.h>

static_assert(sizeof(DDPIXELFORMAT) == 32, "renderer pixel format layout");
static_assert(sizeof(DDSURFACEDESC) == 108 && offsetof(DDSURFACEDESC, ddpfPixelFormat) == 72,
              "renderer surface descriptor layout");
static_assert(sizeof(DDBLTFX) == 100 && offsetof(DDBLTFX, dwFillColor) == 80, "renderer blit layout");
static_assert(sizeof(D3DTLVERTEX) == 32 && offsetof(D3DTLVERTEX, color) == 16, "renderer vertex layout");
static_assert(sizeof(D3DVIEWPORT2) == 44 && sizeof(D3DMATERIAL) == 80, "renderer viewport and material layout");

static_assert(sizeof(BHRenderSurface *) == 4 && sizeof(BHRenderPalette *) == 4,
              "native renderer handles preserve original x86 pointer storage");
static_assert(sizeof(BHTextureSlot) == 28 && offsetof(BHTextureSlot, surface) == 0 &&
                  offsetof(BHTextureSlot, palette) == 4 && offsetof(BHTextureSlot, handle) == 8,
              "native texture slot resource layout");
static_assert(sizeof(BHTextureImage) == 0x70 && offsetof(BHTextureImage, source) == 4 &&
                  offsetof(BHTextureImage, palette) == 8 && offsetof(BHTextureImage, vertices) == 0x4c,
              "native texture image resource layout");
