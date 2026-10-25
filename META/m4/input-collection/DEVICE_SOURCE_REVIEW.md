# Device source review

Canonical LF refresh: current-byte-audit.json pins current source/review bytes
and independently rechecks every one of255 generated key-default strings/NULLs
against key-names.json. Header normalization is proven newline-only against its
saved prior variant. No prior execution evidence is transferred to current bytes.

2026-10-25. Narrow independent review of src/bh_input_devices.c/.h against
complete pinned627/55-byte originals. No material valid-domain discrepancy
found; no source changes, gameplay runs or native input API calls for review.

Name loop executes exactly255 MapVirtualKeyA(VK,2) calls, masking returned
character to8bits. Localized replacement condition matches original slot
bounds00491e14..00491e78 inclusive: VK65..90, unequal mapping, mapped uppercase
A..Z. All other nonnull defaults remain borrowed unchanged. Missing names use
8-byte Key '%c' with mapped low byte>=32, or10-byte Key #%d with VK itself.
The exact varargs pushes at00473dac/00473dcd versus00473de5 establish that
distinction. Allocation failure stores NULL and continues original startup;
source additionally reports failure after initialization. Parent's static
default table is exactly255 entries and preserves original string bytes.

Reset and capture flow agrees: both enabled flags clear before capture0 then1,
period100ms/FALSE. Capabilities queried only after successful capture, size0x194,
and capture success alone enables device even if caps query fails. JOYCAPSA
wCaps offset0x60 maps actual004eedf4/004eef88 into collector capability fields.
Flags are enabled after caps call, as original. Caps buffers retain existing
contents if API fails without writing; source does not invent success values.

Snapshot history32/head, current255 and previous255 reset; list head reset is
represented by the explicit empty-owner precondition rather than leaking
source-owned lists. Eight joystick axes reset after capture. Recenter160/100
occurs before mouse delta/axis reset; both signed32 deltas and low-word pair4
clear, accumulated pair3 becomes512 each, filters clear. Deadzone mask all ones,
calibration flags zero and all eight min/max pairs+1/-1 agree with original.

Shutdown conditionally releases device0 then1, continuing after API failure,
and clears both enabled flags only after release calls. Return/status fields
are source error-reporting extensions; original ignored MMRESULTs. No other
snapshot/calibration/name fields are reset by stop.

Explicit ownership adapters: bind requires fresh owner and borrowed collector/
snapshot/list lifetimes. Replacing a previously allocated localized name frees
only owned memory; original replaced pointers without freeing. Dispose stops
capture then frees owned generated names, restores borrowed defaults and leaves
input-list lifetime to its actual owner. Null API/storage/allocator checks and
empty-list rejection are source guards, not claims about malformed original
states. Tests must override native capture/recenter APIs; their execution
evidence is separate from this static fidelity review.
