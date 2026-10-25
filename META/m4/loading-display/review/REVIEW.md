# Loading display closure: six complete owners

Read-only classification before any implementation. Active ledger339 plus the
pending two loading-image owners contains no implementation variants for any
of these six. Complete raw bodies are contiguous and immutable-PE verified in
`review.json`/per-owner disassembly; fresh decompiler text is retained but loses
two float arguments on every wedge copier and several callback stack values.
No CRT/import/compiler/EH/shared-tail bodies are assigned as gameplay work.

| Owner | Bytes/instructions | Role |
|---|---|---|
|0044a220|340/124|first top-half radial wedge indexed-page copier |
|0044a520|512/171|right-half radial wedge copier |
|0044a380|411/145|bottom-half radial wedge copier |
|0044a8a0|532/177|left-half radial wedge copier |
|0044a720|370/131|final top-half radial wedge copier |
|0044aac0|1487/444|timed MIDAS/IEL/BAD loading display/progress/presentation callback |

The five memory functions are ordinary engine candidates with unresolved
historical attribution. The callback selects game-specific loading resources
and stages, so game role is supported. Adjacency is not the evidence: every
copier reads the image arena/page, mapped indexed/native palettes, dimensions
and progress-float arguments, writes target pixels, and calls only verified
CRT `__ftol`0047de20. The callback calls exactly those five, GetTickCount and
actual DirectDraw surface methods. **There are no additional missing game
callees or callbacks in the six-owner closure.**

## Correct signatures and live bindings

Every copier's real cdecl signature is `(pixels, signed pitch, float from,
float to)`; raw callers push four arguments and clean16 bytes. Ghidra's two
argument signatures are incomplete. Their progress rescaling has explicit
binary32 stores before x87 geometric calculations/FTOL truncation. Signed
halves truncate toward zero. All use the BAD second image page at
arena+page_bytes, current normal palette and original display width/height.
They implement pixel_bytes1/2/4. Pixel_bytes3 deliberately performs no wedge
stores; this is not a missing branch to silently fill in.

Global004e0f8c is **signed integer stage**, not float progress: CMP/INC and
0/1/2 palette/image selection prove MIDAS/IEL/BAD roles. The image initializer's
zero store alone cannot establish its type.004912e0 is integer redraw countdown
(reset2, decremented when nonzero).004dfc84 is DWORD deadline, clock+5000 with
32-bit wrap, compared by signed(clock-deadline).004e0f90 alternates0/1 selecting
float history004912e4/004912e8.004e0f94 is float progress baseline and004912ec
is stored float normalization factor1/(1-baseline). The callback normalizes its
single float argument in place before subsequent rendering/history updates.

Current image arena and normal palette pairs come from the pending image
owner. MIDAS source is arena+0x96000 and its retained palette pair; IEL uses
+0xe1000 and its pair; stage2 uses BAD at0 plus current palette pair. Existing
surface owners supply color004e9c3c, optional software copy target004e9c4c,
primary004ee948 and back004ee94c. Software flag004e32f4 controls the optional
copy. Bind these real interfaces/lifetimes explicitly; no guessed globals.

## Native operations, numeric constraints and recommendation

The whole callback needs actual GetTickCount and DirectDraw IsLost/Restore,
Lock/Unlock, optional BltFast(flags0x10), and primary Flip(flags1). It does not
create a window/device, invoke WinMain, use D3D, play audio, collect input or
call any app-specific message callback. Real offscreen surfaces can exercise
memory copying, locks and software copy; full-screen Flip/lost-surface success
remains separately unvalidated if the source fixture only obtains real native
error returns. No visible UI or game execution is needed to restore the body.

Full first redraw handles pixel_bytes1/2/3/4; the 3-byte path writes high,
middle,low packed-color bytes in that order, unlike a guessed standard BMP
routine. Later wedge updates retain their original3-byte no-store behavior.
First use/history<0 or pending full-redraw countdown causes full image copy.
Progress thresholds are1/8,3/8,5/8,7/8 and1; constants.json independently pins
these plus scales8/4 and comparisonzero. Retain all ordered/unordered branch
semantics when translating FCOMP/FNSTSW/TEST, rather than assuming comparisons
from corrupted pseudocode. Baseline1 makes normalization denominator zero;
PC24/extreme/nonfinite progress needs explicit source guards/validation limits,
not fabricated FTOL outputs or inline assembly.

**Recommendation:** once root admits the initial image family, this whole
six-owner closure is bounded and materially completes its loading display
consumer. If remaining time cannot support both pure geometry and native
presentation checks, restore the five complete wedge owners as one coherent
dependency family first (2165 bytes/748 instructions); do not count an extracted
prefix of0044aac0 as its recovery. All five can compile/link against actual
image state and execute source memory fixtures for dimensions, clipping,
from/to thresholds, 1/2/4-byte formats,3-byte no-op, protected spans and exact
binary32 boundaries. No broad renderer rewrite, gameplay replay or diagnostic
controller is required. Full WinMain/bootstrap and playable game remain open.
