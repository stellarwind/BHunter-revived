#ifndef BH_RENDER_TYPES_H
#define BH_RENDER_TYPES_H

/* Original 32-bit renderer data and state values. Interface pointers are
   opaque handles to objects owned by the SDL renderer. */
typedef unsigned char BYTE;
typedef unsigned long DWORD, ULONG;
typedef long LONG, HRESULT;
typedef void *LPVOID;
#define CALLBACK __cdecl

typedef struct GUID
{
    DWORD data1;
    unsigned short data2, data3;
    BYTE data4[8];
} GUID, IID;
extern const IID IID_IDirect3D2, IID_IDirect3DHALDevice, IID_IDirect3DTexture2;

typedef struct RECT
{
    LONG left, top, right, bottom;
} RECT, *LPRECT;

typedef struct PALETTEENTRY
{
    BYTE peRed, peGreen, peBlue, peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct IDirectDraw *LPDIRECTDRAW;
typedef struct IDirectDraw2 *LPDIRECTDRAW2;
typedef struct IDirectDrawSurface *LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawPalette *LPDIRECTDRAWPALETTE;
typedef struct IDirect3D2 *LPDIRECT3D2;
typedef struct IDirect3DDevice2 *LPDIRECT3DDEVICE2;
typedef struct IDirect3DMaterial2 *LPDIRECT3DMATERIAL2;
typedef struct IDirect3DTexture2 *LPDIRECT3DTEXTURE2;
typedef struct IDirect3DViewport2 *LPDIRECT3DVIEWPORT2;

typedef struct DDPIXELFORMAT
{
    DWORD dwSize, dwFlags, dwFourCC, dwRGBBitCount;
    DWORD dwRBitMask, dwGBitMask, dwBBitMask, dwRGBAlphaBitMask;
} DDPIXELFORMAT, *LPDDPIXELFORMAT;

typedef struct DDCOLORKEY
{
    DWORD dwColorSpaceLowValue, dwColorSpaceHighValue;
} DDCOLORKEY;

typedef struct DDSCAPS
{
    DWORD dwCaps;
} DDSCAPS, *LPDDSCAPS;

typedef struct DDSURFACEDESC
{
    DWORD dwSize, dwFlags, dwHeight, dwWidth;
    LONG lPitch;
    DWORD dwBackBufferCount;
    union
    {
        DWORD dwZBufferBitDepth;
        DWORD dwRefreshRate;
    };
    DWORD dwAlphaBitDepth, dwReserved;
    void *lpSurface;
    DDCOLORKEY ddckCKDestOverlay, ddckCKDestBlt, ddckCKSrcOverlay, ddckCKSrcBlt;
    DDPIXELFORMAT ddpfPixelFormat;
    DDSCAPS ddsCaps;
} DDSURFACEDESC, *LPDDSURFACEDESC;

typedef struct DDBLTFX
{
    DWORD dwSize;
    DWORD reserved[19];
    union
    {
        DWORD dwFillColor;
        DWORD dwFillDepth;
    };
    DDCOLORKEY ddckDestColorkey, ddckSrcColorkey;
} DDBLTFX, *LPDDBLTFX;

typedef DWORD D3DRENDERSTATETYPE, D3DLIGHTSTATETYPE, D3DPRIMITIVETYPE, D3DVERTEXTYPE;
typedef DWORD D3DMATERIALHANDLE, D3DTEXTUREHANDLE;
typedef HRESULT(CALLBACK *LPD3DENUMTEXTUREFORMATSCALLBACK)(LPDDSURFACEDESC, LPVOID);

typedef struct D3DRECT
{
    LONG x1, y1, x2, y2;
} D3DRECT;

typedef struct D3DTLVERTEX
{
    float sx, sy, sz, rhw;
    DWORD color, specular;
    float tu, tv;
} D3DTLVERTEX;

typedef struct D3DVIEWPORT2
{
    DWORD dwSize, dwX, dwY, dwWidth, dwHeight;
    float dvClipX, dvClipY, dvClipWidth, dvClipHeight, dvMinZ, dvMaxZ;
} D3DVIEWPORT2;

typedef struct D3DCOLORVALUE
{
    float r, g, b, a;
} D3DCOLORVALUE;

typedef struct D3DMATERIAL
{
    DWORD dwSize;
    D3DCOLORVALUE diffuse, ambient, specular, emissive;
    float power;
    D3DTEXTUREHANDLE hTexture;
    DWORD dwRampSize;
} D3DMATERIAL;

#define FAILED(result) ((result) < 0)
#define SUCCEEDED(result) ((result) >= 0)
#define DD_OK 0L
#define E_FAIL ((HRESULT)0x80004005UL)
#define E_INVALIDARG ((HRESULT)0x80070057UL)
#define D3DBLEND_DESTALPHA 0x7UL
#define D3DBLEND_DESTCOLOR 0x9UL
#define D3DBLEND_INVDESTALPHA 0x8UL
#define D3DBLEND_INVDESTCOLOR 0xaUL
#define D3DBLEND_INVSRCALPHA 0x6UL
#define D3DBLEND_INVSRCCOLOR 0x4UL
#define D3DBLEND_ONE 0x2UL
#define D3DBLEND_SRCALPHA 0x5UL
#define D3DBLEND_SRCCOLOR 0x3UL
#define D3DBLEND_ZERO 0x1UL
#define D3DCLEAR_TARGET 0x1UL
#define D3DCLEAR_ZBUFFER 0x2UL
#define D3DCMP_ALWAYS 0x8UL
#define D3DCMP_GREATEREQUAL 0x7UL
#define D3DCULL_CW 0x2UL
#define D3DCULL_NONE 0x1UL
#define D3DENUMRET_CANCEL 0x0UL
#define D3DENUMRET_OK 0x1UL
#define D3DLIGHTSTATE_MATERIAL 0x1UL
#define D3DPT_TRIANGLEFAN 0x6UL
#define D3DRENDERSTATE_ALPHABLENDENABLE 0x1bUL
#define D3DRENDERSTATE_COLORKEYENABLE 0x29UL
#define D3DRENDERSTATE_CULLMODE 0x16UL
#define D3DRENDERSTATE_DESTBLEND 0x14UL
#define D3DRENDERSTATE_DITHERENABLE 0x1aUL
#define D3DRENDERSTATE_FOGCOLOR 0x22UL
#define D3DRENDERSTATE_FOGENABLE 0x1cUL
#define D3DRENDERSTATE_SHADEMODE 0x9UL
#define D3DRENDERSTATE_SPECULARENABLE 0x1dUL
#define D3DRENDERSTATE_SRCBLEND 0x13UL
#define D3DRENDERSTATE_TEXTUREHANDLE 0x1UL
#define D3DRENDERSTATE_TEXTUREMAG 0x11UL
#define D3DRENDERSTATE_TEXTUREMAPBLEND 0x15UL
#define D3DRENDERSTATE_TEXTUREMIN 0x12UL
#define D3DRENDERSTATE_TEXTUREPERSPECTIVE 0x4UL
#define D3DRENDERSTATE_ZENABLE 0x7UL
#define D3DRENDERSTATE_ZFUNC 0x17UL
#define D3DRENDERSTATE_ZWRITEENABLE 0xeUL
#define D3DVT_TLVERTEX 0x3UL
#define DDBLTFAST_WAIT 0x10UL
#define DDBLT_COLORFILL 0x400UL
#define DDBLT_DEPTHFILL 0x2000000UL
#define DDBLT_WAIT 0x1000000UL
#define DDERR_INVALIDPARAMS ((HRESULT)0x80070057UL)
#define DDERR_NOTFOUND ((HRESULT)0x887600ffUL)
#define DDERR_SURFACELOST ((HRESULT)0x887601c2UL)
#define DDERR_SURFACENOTATTACHED ((HRESULT)0x887601ccUL)
#define DDERR_UNSUPPORTED ((HRESULT)0x80004001UL)
#define DDFLIP_WAIT 0x1UL
#define DDLOCK_WAIT 0x1UL
#define DDPF_PALETTEINDEXED8 0x20UL
#define DDPF_RGB 0x40UL
#define DDSCAPS_3DDEVICE 0x2000UL
#define DDSCAPS_BACKBUFFER 0x4UL
#define DDSCAPS_PRIMARYSURFACE 0x200UL
#define DDSD_BACKBUFFERCOUNT 0x20UL
#define DDSD_CAPS 0x1UL
#define DDSD_CKSRCBLT 0x10000UL
#define DDSD_HEIGHT 0x2UL
#define DDSD_PIXELFORMAT 0x1000UL
#define DDSD_WIDTH 0x4UL

#endif
