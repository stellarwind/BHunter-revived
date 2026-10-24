# 00409130 scene pass: boundary and source contract

The reviewed body is 00409130–00409523 (1012 bytes). Its entry checks the live
scene pointer and a mode gate, and its sole exit path resets the cached room
when either gate is closed. The active path copies the camera identity matrix,
translates and rotates it, builds an integer-half-width/half-height projected
matrix, locates a room, advances the frame, invokes 00409f40, then emits two
00478a70 clip commands. These callees and the room/resource state identify it
as game scene orchestration, with high confidence. It is not CRT, EH, compiler
support, an import thunk, or an independent shared tail. The owning 00409f40
schedule body and 0040a8c0 room locator have real source definitions.

The original `.rdata` bytes at 00489440 are `39 9d 52 a2 46 df 91 3f`, a
double radians-per-degree factor. At 00489288, `00 00 00 3f` is binary32 one
half. The matrix at 0048c030 is twelve floats with identity diagonal. The
point at 0048c080 is the three-float vector `(0,0,1)`. These were read from
the immutable original EXE's PE sections. The projection uses integer
division of each camera dimension by two, including odd and negative-dimension
behavior at the arithmetic level; the source adapter rejects nonpositive
dimensions. The two final clip commands use separate actual target dimensions.

`BHSceneOwner` owns source state corresponding to the scattered camera and frame
globals. `BHSceneBindings` provides the typed world-to-scheduler mapping. A
cached room is retained when the eye is within inclusive bounds. Otherwise,
0040a8c0 is called with the old room as hint (or first room when no cache),
first at zero expansion, then one, then the original hint is retained. The
frame increments before the schedule pass. The scheduler receives the projected
matrix and actual eye; the separately retained inverse-projected point minus
eye belongs to the scene owner and is not substituted for that eye. Near/far
settings and full camera clip bounds are applied before traversal. The viewport
retains its caller-bound target width and height, so the reset clip commands for
buckets zero and three can restore an extent larger than the camera projection.

Source implementation is functional, not a raw-byte match. The historical
owner is void and does not report command failures. The source API reports
invalid arguments, missing bindings, scheduler errors, and queue exhaustion;
it still attempts both final reset commands on a full queue. The current
source-built tests exercise the contract without executing the original EXE.
The original x87 24-bit precision behavior and exceptional/nonfinite camera
inputs remain outside validation.
