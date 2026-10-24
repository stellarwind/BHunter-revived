# Page clear/color conversion and cursor004705c0/004708d0

Classification corrected before implementation: these are complete ordinary
engine-candidate page clear and cursor controls, not glyph copying.00470900
performs glyph blits/text wrapping and is not claimed here.292+10instructions
cover782+33bytes. No direct calls to game helpers/CRT are present; the clear
owner uses real native surface Lock/Unlock through official SDK interfaces.
They are not compiler support, imports, shared tails or verified library code.

Clear chooses conversion from the borrowed atlas pixel format/palette, preserving
the original dependency even if another requested format would look preferable.
Indexed output minimizes Manhattan RGB distance over all256 palette entries;
strictly smaller distance changes the winner, so earliest index wins ties.
Highcolor uses original signed mask-division-by2 bit-length loops and shifts/
masks the three requested RGB bytes. Full32bit last_color caches conversion:
an identical color reuses packed_color even if palette/format changed; differing
high byte triggers recomputation despite unchanged low24bit RGB.

Every call still attempts to fill the entire page size-square using actual
native surface row pitch.8/16/24/32byte stores match original branch widths.
The decompiler's apparent2/3/4byte row increment was misleading; assembly uses
the native pitch. After the native attempt, successful or failed Lock, clear
resets left/cursor_x/cursor_y/line_height and sets the real resident slot's
16bit pixels_dirty flag to1 when resident. Palette dirty and image flags remain
unchanged. NULL page clear is an original no-op.

Cursor sets left=x,cursor_x=x,cursor_y=y,line_height=0, accepting raw negative
coordinates with no clamping. NULL page is an original no-op. It does not wrap
text, change color, dirty residency or copy glyph pixels.

Source clear's success int and q.error/last_hresult expose native errors ignored
by original void return. Binding/format/dimension/native-buffer checks reject
unsafe inputs. Constructor originally leaves packed_color undefined while
last_color=-1; a first clear0xffffffff therefore collides with the sentinel.
The recovered constructor's explicit zero initialization makes this deterministic,
not a claim about the original undefined packed value or a guessed correction.

`python META/m4/atlas-glyph/build.py` freshly compiles real new source/test TUs
and actual atlas/page dependencies, links current real m0 owners with pinned
SP3 compiler/headers, stock VC5 LINK and official native SDK/system libraries,
then runs `build/atlas-glyph/bhunter-atlas-clear-cursor.exe`.10,313checks pass:
all actual native8/16/24/32 page pixels, independent color/nearest-tie controls,
cached palette conversion/full32bit color comparison, raw signed cursor values,
all cursor resets, signed bit31 mask packing over real standard32bit storage,
real residency invalidation/re-upload, and actual native busy Lock errors.

Holding a real native surface lock forces HRESULT0x887601ae (-2005532242) in
both8/16bit cases. Checks show failed Lock still publishes cached color, resets
all cursor fields and marks the real resident dirty; after releasing the held
lock, retry uses cached conversion, fills the page and real residency uploads
the expected pixel values. No fake COM or callback stubs are used.

`audit.py` verifies every original instruction against immutable EXE and source
hashes/focused results. No original/game execution or media writes. Native
backend coverage is real RGB software DirectDraw/Direct3D2, not arbitrary HAL
failure modes. Parent full integration remains separate evidence.
