# Complete script target motion callbacks

Whole owners00443800 (414 bytes, end0044399e) and004439a0 (273 bytes,
end00443ab1) restore movement and rotation of the script's movable target
class. Parser installation at00444a07..00444ae2 connects the actual source
entrypoints to already admitted query00443e80 for translation and00443f00
for rotation. These are ordinary behavior owners with unresolved proprietary
engine/game authorship. No CRT, compiler support, import or shared tail is
counted. Each complete body, reviewed calls, PE bytes and constants are pinned
in `owners.json`, the assembly records and `constants.json`.

The translation owner adds each binary32 delta component to both target bounds,
position and every attachment position; actual admitted world/local matrix
translation definitions update forward/inverse matrices. The inverse uses the
negative delta through its own nonidentity basis. The actor's already added
origin delta is retained until positive duration expires, then subtracted once
before actual pool release. Movement happens before countdown handling, even
on the final tick. Zero/negative/unordered starting durations are unlimited.
Positive countdowns subtract the exact original binary64 1/30; expiration tests
the pre-store subtraction, preserving original x87 branch order.

The rotation owner first stores every updated angle, then corrects each by one
exact binary64 2*pi when outside the original binary64 +/-pi bounds. It is not
modulo normalization: values beyond a whole revolution may remain outside the
normal interval. The low-side x87 C0 branch includes unordered. NaN result class
is tested; payload/sign are not claimed. Rotation retains target position,
origin, bounds and matrices. Its expiration releases without translation undo.

No full script dispatch, target constructor or source gameplay startup is
claimed. A count-zero binding accepts actual live pool entities, persistent
actual lifetime context and already initialized movable-target fields, binds
the existing real cleanup query and complete new step. For translation, callers
must supply origin already including the delta as the real parser does. Named
field views copy only verified offsets of this particular target class; they
do not reinterpret autonomous route actor state. The private source motion
context sits at payload+408, separate from original fields and the existing
lifetime/timer binding. No globals, vtables or missing callbacks are invented.
Live ownership is a caller contract; source guards reject foreign/dead/self
targets and foreign/cyclic attachments before mutation. Rotation ignores the
attachment chain because the original does not traverse it.

`python META/m4/resume400/b/build_check.py` compiles actual source translation
units using pinned VC5 SP3 and stock LINK into named
`build/script-motion/bhunter-script-motion-check.exe`. It passes99 source-only
checks and requires link failure without both complete new owners. Fixtures
exercise nonidentity forward/inverse transforms, both bounds, two attachments,
positive/zero/negative/NaN durations, movement before release, actual cleanup
query and deferred delete, fixed binary32 angle fixtures around pi and multiple
turns, and malformed/lost ownership guards. Existing actual pool/sound/timer
owners are linked; fixtures require no native audio device or playback.

Parent current full real/clean integration is required for admission. No game,
original code, automated gameplay, physical input or Play ran. PC24/runtime
precision transitions and full movable-target construction remain open. M3 is
skipped; M4 and a playable source game remain incomplete.
