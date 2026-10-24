# Queued resource consumer and entity draw binding

Four complete original owners are admitted: resource callback `004775f0`,
billboard callback `00477640`, resource command producer `00478170`, and game
entity draw method `004413b0`. They cover 650 original bytes / 213 decoded
instructions; `owners.json` pins their boundaries, role decisions, complete
original instruction bytes and source owners. Runtime helpers, imports and
shared tails are excluded. Engine provenance remains unresolved for the queue
owners; the game method has verified constructor installation and caller sites.

`bh_draw_submit_resource` copies the original seven binary32 pose/modulation
words and uses the original bucket insertion order. Source handles resolve
through an explicit lookup, with no truncation or execution of original pointer
values. Both callback owners snapshot their input arguments, call actual
resource/sprite source, then read the command's **post-draw** next link. An
adversarial emitter fixture checks that ordering. Missing binding and draw
failure are reported as source errors while retaining the following link.

`bh_render_resources_*` is an explicit lifetime adapter, not a new original
constructor claim. It calls admitted `bh_model_load` and owns each model,
working cache, instance and descriptor under a stable slot handle. Required
render bindings must exist before initialization; a populated owner cannot be
reinitialized without disposal. Partial load cleanup and table growth preserve
published handles. Queues borrow handles; drain them before disposing resources.
Original constructors for LOD/billboard/ribbon descriptor tags remain unverified.

The entity method uses real `BHEntity` fields at position `+0c`, angles `+3c`,
phase `+fc` and handle `+114`, submitting two models and a glow in order. It is
called via payload `+e8`, not the generic refresh callback. Queue saturation
does not roll back previous submissions or prevent final angle mutation. The
stored intermediate positions and retained sine precision are tested. No
fabricated constructor callbacks or vtables are installed.

The real-data native fixture allocates a source entity slot, constructs an
owned `PU_9A.BSP` model resource and loads `flare_yl.bmp` through the recovered
PCX alias. Both model commands and the glow execute through source callbacks,
clipping, native queues, real state barriers and Direct3D2. The unbound-model
and unbound-image control produces an empty bitmap. This is an isolated draw
check, with no gameplay simulation or scheduling. Saved native images are
evidence of this composition only, not original-game visual equivalence.

The real level snapshot replaces its temporary model/cache/descriptor arrays
with the lifetime adapter and uses queued resource commands plus actual state
headers in chain order. Its image remains byte-identical: 1,798 polygons,
3,112 triangles and 146 textures. Shell selection, static placement scheduling,
camera and diagnostic lights remain adapters.

The 149-TU `bhunter-integration.exe` passes 3,399,044 real-data checks and
2,906,719 in a clean checkout without original assets. All 74 missing-module
controls reject the link. The 157 real-data focused checks comprise 31 queue
callback/producer, 25 lifetime, 31 entity field/order and 70 native composition
checks. Original references are unchanged; no game or original code executes.
PC53/nearest/masked is the admitted profile. Nonfinite/huge-angle trigonometry,
PC24 fidelity and actual entity scheduling remain unvalidated.

Resource tag constants were given the `BH_RESOURCE_TAG_*` namespace so the queue
and resource headers can compose. The previous `0045adf0` implementation variant
and earlier shared-source tests retain their byte-pinned historical snapshots;
the renamed variant has its own current linked evidence. No byte-match evidence
is claimed or transferred.

Next review the pickup constructor `00441a00` and its remaining draw/tick/query
dependencies (`004412f0`, `004415a0`, `00441570`). Its world-location dependency
`0040a8c0` is already source-owned as `bh_world_locate`. Actual child/entity
scheduling `00409f40`, full startup/resource registries, software rendering and
atlas glyph copy remain open. M4 has not passed.

Audit: `python tools/check_queued_resource.py`.
