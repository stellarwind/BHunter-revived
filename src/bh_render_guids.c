#include "bh_render_types.h"
#include <stddef.h>

_Static_assert(sizeof(IID) == 16, "renderer interface ID layout");
_Static_assert(sizeof(DDPIXELFORMAT) == 32, "renderer pixel format layout");
_Static_assert(sizeof(DDSURFACEDESC) == 108 && offsetof(DDSURFACEDESC, ddpfPixelFormat) == 72,
               "renderer surface descriptor layout");
_Static_assert(sizeof(DDBLTFX) == 100 && offsetof(DDBLTFX, dwFillColor) == 80, "renderer blit layout");
_Static_assert(sizeof(D3DTLVERTEX) == 32 && offsetof(D3DTLVERTEX, color) == 16, "renderer vertex layout");
_Static_assert(sizeof(D3DVIEWPORT2) == 44 && sizeof(D3DMATERIAL) == 80, "renderer viewport and material layout");

const IID IID_IDirect3D2 = {0x6aae1ec1, 0x662a, 0x11d0, {0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a}};
const IID IID_IDirect3DHALDevice = {0x84e63de0, 0x46aa, 0x11cf, {0x81, 0x6f, 0x00, 0x00, 0xc0, 0x20, 0x15, 0x6e}};
const IID IID_IDirect3DTexture2 = {0x93281502, 0x8cf8, 0x11d0, {0x89, 0xab, 0x00, 0xa0, 0xc9, 0x05, 0x41, 0x29}};
