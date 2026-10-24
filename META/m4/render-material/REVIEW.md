# Default material/native state initialization 0045b1d0

Classification precedes recovery: complete ordinary engine-candidate initializer,
called by 00479200. Historical engine authorship is unknown; it is not an import,
CRT/compiler helper, EH thunk or shared tail. The sole direct call is already
admitted texture preallocation 0044e780. SDK calls use real official interfaces.
Inline SCAS/MOVS sequences are compiler string-copy idioms, represented as a
bounded source error append, not separately counted recovered functions.

The original software branch returns NULL before touching any native state or
existing error log. Native mode first preallocates real texture slots. Failure
appends ` VMEMERR: Alloc texture slots failed. ` and continues. IDirect3D2's
CreateMaterial creates an IDirect3DMaterial2; any nonzero HRESULT appends
` D3DERR: Create default material failed. ` and immediately returns the shared
error string. A successful create is followed by SetMaterial and GetHandle.

The 80-byte official D3DMATERIAL starts zero: diffuse/ambient/specular RGB=1,
their alpha values remain zero, emissive RGBA=0, power=40, texture handle=0,
ramp size=1. Do not infer opaque alpha or emissive white from the decompiler's
bad stack types. Assembly offsets and official SDK offsets establish the values.

Render states occur in this order: ZENABLE=1, ZFUNC=7 (GREATEREQUAL),
ZWRITEENABLE=1, SPECULARENABLE=1, SHADEMODE=2 (GOURAUD), DITHERENABLE=1;
SetLightState(MATERIAL, actual material handle); TEXTUREPERSPECTIVE=1,
TEXTUREMAPBLEND=2 (MODULATE), TEXTUREHANDLE=0, TEXTUREMAG=2 (LINEAR),
TEXTUREMIN=2 (LINEAR), CULLMODE=2 (CW), COLORKEYENABLE=1. ZFUNC is intentionally
the original reversed comparison, not the diagnostic renderer's default.

Only CreateMaterial's HRESULT gates original control flow. All later HRESULTs
are ignored for its return value; source diagnostics separately count/report
them without rollback or skipping subsequent calls. On completion, the original
returns NULL exactly when its shared error buffer is empty. Existing errors and
preallocation failure remain visible after successful native initialization.

The original local material COM reference is never released in this body.
Source BHRenderMaterial explicitly retains it, and bh_render_material_dispose
is a separate lifetime adapter. Zero-initialized owner is required, repeated live
initialization is guarded, and bounded appends prevent original buffer overflow.
The caller keeps the real device alive while using its material handle and
disposes the owner before releasing that device. No original destructor claim.

`python META/m4/render-material/build.py` builds the actual new source/test TUs
with pinned SP3 compiler/headers and links real current m0 owner objects with
stock VC5 LINK, official DirectDraw SDK library and system libraries into
`build/render-material/bhunter-render-material.exe`. No fake COM interfaces.
All 83 checks pass: three real native context/material cases (success, real
allocation failure at texture slot allocation, preexisting error string),
GetMaterial field readback, GetLightState actual handle, all 13 GetRenderState
values, actual native slot surface/handle, all 17 original native HRESULTs,
software early exit, guards and explicit source disposal.

`audit.py` verifies all 195 instructions and both exact error strings against
the preserved EXE, plus focused source hashes/results. Original code is never
executed; no game launches or media writes. CreateMaterial failure/HAL errors
were not forced and remain static-review-only contracts. Parent integration is
separate and pending; current m0 objects are recorded by hash in focused evidence.
