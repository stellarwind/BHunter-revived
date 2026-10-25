# Fade family

## Boundary and role

Classified before recovery:00478930 is a91-byte queue producer;00477f20 is its26-byte consumer;0045cf30 is a20-byte state setter;0045cf50 is the1852-byte complete display fade owner. Their595 combined instructions are independent complete bodies with RET endings, not shared tails/imports/compiler helpers. Original authorship remains unknown; these are engine candidates. CRT allocation0047dae0 and conversion0047de20 are reused CRT, excluded from source count. Optional presentation0045d690 remains unrecovered here.

export_review.py pins whole disassemblies/decompilations/metadata/xrefs and verifies each byte against immutable original SHA c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1. No original code was executed.

## Complete source behavior

Producer reserves original header8 (source bucket8), raw float/color state in resource/words0, no publication when full. Consumer copies raw state then reads next. New enum BH_DRAW_FADE=16 appends without changing earlier selectors.

Native0045cf50 increments residency epoch, flushes real images, EndScene/BeginScene, mode4 depth/blend, draws one untextured TL triangle fan, then another real barrier and restores mode0 depth/blend. These are the admitted real bh_render_mode/texture owners and actual IDirect3DDevice2 methods. Fan positions span signed display width/height, z=near/near, rhw=1/near, UV/specular zero, flags0xc. Alpha is trunc(amount * -255), then integer subtraction from color; it is not clamped. Native failures are reported while continuing later operations, as neighboring restored render owners do.

Software branch first captures double keep=1-amount and integer truncated tinted RGB. Signed original color extraction divides with sign bias before masking each channel. Indexed mode builds all256 entries using six red, seven green and six blue levels; operation grouping uses negative -.2 and -1/6 constants followed FSUBR.16-bit mode builds all65536 entries using actual caller-bound masks/extract/expand shifts, per-channel truncate and summed packed shorts. Both walk display rows using actual pitch and leave padding untouched. The active state is deliberately overwritten FROM cached values on a rebuild, while computations retain captured prior active keep/tints. The cached fields themselves stay unchanged. Equality then reuses that generated table, even if cached state describes a different fade.

## Explicit source bindings and guards

BHFadeContext borrows real render/texture/display memory and format shifts; no fake global/device/vtable. A table is malloc-owned until bh_fade_dispose (or caller seeds malloc-compatible storage). Source table_bytes and table_ready reject original undefined cases: first allocation with equal cache would use uninitialized storage, and switching a256-byte table directly to16-bit would write outside it. Caller may dispose before format change or supply a sufficiently large initialized table. Positive dimensions require a real pixel buffer and sufficient positive row pitch. Missing native device/zero near are rejected. Normal original branch behavior is retained; these guards are not counted as original owners.

## Numeric domain

Independent validation covers finite amount[0,1], including signed32-bit colors and actual display shifts. Setter/queue preserve unrestricted raw payloads; no clamp is inserted. Extreme floating-to-integer overflow behavior is tied to pinned VC5 CRT ftol and is not claimed portable or independently validated. Left shifts use unsigned wrapping to preserve x86 low bits even for negative intermediate channels.

## Validation

build_check.py compiles the real TU plus real queue/reserve/round/FP source with pinned VC5 SP3, links against real remaining integration objects (hashes recorded), then runs source-only bhunter-fade-check.exe.65 checks cover raw NaN state transport, full queue, cache reversal/reuse/undefined firstuse guard, complete565 identity table, full tint, signed color extraction, odd pitch padding, and21 mixed independent static table fixtures. generator is a scalar instruction-operation oracle, not original execution or a broad IEEE/x87 proof. Native actual COM composition and fresh overall integration are owned by root and recorded separately; standalone software evidence alone does not imply native validation.

## Native readback limit

Final native fixture uses the real Direct3D2 device, pending blue texture and untextured fade fan. Amount0 gives4096 exact blue pixels and amount1 gives4096 exact red pixels. Amount.5 gives2048 blue and2048 purple pixels on this host; the center-pixel blend assertion passes, but this is not proof of uniform fractional blending or original-runtime image equivalence. Both endpoints, real barriers and device state restoration are independently checked. Patterned fractional output remains a backend/fidelity observation; no original game was run. See `../review/FINAL_NATIVE_REVIEW.json`.

A bounded diagnostic rejected two state explanations: STIPPLEDALPHA was already0 and explicitFALSE retained identical readbacks. Disabling DITHERENABLE retained the half-blue checkerboard while shifting the blended purple quantization from131 to123. Both endpoint controls remained exact. Production state is unchanged; see `../review/fade-stipple/result.json` and `dither-result.json`. Investigation stopped after these controls.
