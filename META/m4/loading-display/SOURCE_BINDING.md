# Source loading-family binding

This is the explicit source integration contract, not a recovered WinMain or a playable game target. Production implementations are in `src/bh_loading_image.c`, `src/bh_loading_blit.c`, `src/bh_loading_display.c` and `src/bh_loading_progress.c`; the exact linked source fixtures are in the corresponding test units.

## Ownership and ordering

1. Obtain actual borrowed DirectDraw surfaces, dimensions, pixel format/channel shifts and a real allocator from the platform/resource owners. Do not cast original globals into these source contexts or manufacture a COM vtable. The faithful real-asset display profiles are640x480/full and320x240/half; smaller native fixture surfaces do not establish additional game display modes.
2. Bind a fresh `BHLoadingImages` with `bh_loading_images_bind`, then call `bh_loading_images_init` and check its status. This owns the arena and actual decoded image data. Display pixel size/channel shifts must describe the real target format; indices remain bytes in the arena even for native color output. An indexed native surface also requires the platform-owned palette: checked index bytes alone do not establish visual color acceptance.
3. Bind a fresh explicitly initialized `BHLoadingDisplay` to that image owner and actual color/primary/back/optional software target. Dimensions describe the real logical rows, not raw pitch. Its history/baseline/scale are explicit caller state. In the normal initializer path, stage0/1 redraw counters force complete logo copies; the stage2 transition initializes baseline, scale and histories before progress geometry. Fixtures that jump directly to stage2 deliberately supply their own normalization/history state.
4. Only AFTER image initialization, install a borrowed `BHLoadingProgress` using `bh_loading_progress_install`. This takes the existing single VFILE callback slot. Actual later first-access opens forward the float into the complete display owner. Duplicate opens do not notify. Installing before image loading is not the recovered bootstrap ordering.
5. Remove the callback before ending any borrowed owner lifetime. Then dispose the image arena and release surfaces through their actual owners. Do not dispose an allocator-owned arena twice or release borrowed COM interfaces from the loading owner. Rebinding an existing image context requires prior disposal.

The original archive callback is void. The explicit source display's diagnostic return/native error is recorded by the progress owner; it does not turn the archive notification into a failed VFILE open. Offscreen fixtures deliberately observe real NOTFLIPPABLE returns while verifying copied bytes. An actual fullscreen front/back chain and successful Flip are separate platform acceptance work.

## Numeric and storage contracts

Use the established x86 PC53/nearest/masked source profile; the whole game PC24 question remains open. The image loader retains fixed arena offsets and full/half page strides, transformed palette indices and both inactive/active retained table copies. Source spans and allocation failures have explicit safe errors. Original unchecked failure paths are not permission to fabricate success.

The display retains signed native pitch, including checked negative-pitch rebasing for the geometry context. Full24-bit rows write high/middle/low packed bytes. The five3-byte wedge paths intentionally make no stores. Pixel source/destination spans must be distinct and remain accessible for the entire call. Pure geometry negative-pitch checks do not prove that this host returned a negative-pitch COM surface.

No loading-family function polls physical input, creates a game window, plays sound, mounts executable code or supplies missing world/script constructors. The current integration composes real definitions and actual offscreen native operations, while source startup/world allocation and full gameplay remain unfinished. See `NEXT.md` for the next bounded parser prerequisite and preserved external-feature limits.
