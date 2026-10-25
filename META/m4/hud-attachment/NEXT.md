# Next source boundary after the HUD block

The next coherent owner is **the complete player scene callback `0041c860`**.
It can start in the next user-authorized block without revisiting skipped M3 or
automating gameplay. Recover its whole body and test source-built states; do
not admit a HUD-only prefix. Fresh read-only exports in `next-player-review/`
verify all 3934 bytes / 1162 instructions, end exclusive `0041d7be`, against the
immutable PE. Raw SHA256 is
`bc70be7151cd87fe0e345ae1849ab830289fabbe25c129ed47474aa83d30f939`.
`frontier_review.py` reproduces these facts. `player-frontier/` holds the
independent earlier boundary declaration/constants/notes. Boundary is whole
render behavior, not CRT/import/compiler helper or an assignable shared tail;
historical engine authorship remains unresolved. No implementation is claimed.

The old `../frontier-90min/NEXT.md` HUD/attachment prerequisite is superseded
for this family. Final admission status and exact tested variants remain in
the parent block's evidence and function ledger, not inferred from file counts.

## Reuse map

| Original direct callee | Current real source owner |
|---|---|
| `004011f0` | `bh_vec3_normalize`, `src/bh_math.c` |
| `00409130` | `bh_scene_owner_step`, `src/bh_scene_owner.c` |
| `00418f00` | `bh_hud_world_draw`, `src/bh_hud_world.c` |
| `0041a3d0` | `bh_hud_flat_draw`, `src/bh_hud_flat.c` |
| `0041b2d0` | `bh_hud_setup`, `src/bh_hud.c` |
| `0041b650` | `bh_radar_marker`, `src/bh_radar.c` |
| `0041bbf0` | `bh_hud_markers_draw`, `src/bh_hud_markers.c` |
| `0041c020` | `bh_scene_background_step`, `src/bh_scene_background.c` |
| `00427710` | `bh_hud_scores_draw`, `src/bh_hud_scores.c` |
| `00477530` | `bh_queue_reserve`, `src/bh_queue_reserve.c` |
| `00477fe0` | `bh_camera_queue_pose`, `src/bh_camera_queue.c` |
| `00478060` | `bh_material_queue_words`, `src/bh_material_queue.c` |
| `00478170` | `bh_draw_submit_resource`, `src/bh_draw_submit.c` |
| `00478390` | `bh_draw_segment`, `src/bh_draw_queue.c` |
| `004784c0` | `bh_horizon_queue`, `src/bh_horizon_queue.c` |
| `004785f0` | `bh_draw_oriented`, `src/bh_draw_queue.c` |
| `00478730` | `bh_quad_queue`, `src/bh_quad_queue.c` |
| `00478930` | `bh_fade_queue`, `src/bh_fade.c`; complete setter/consumer/native/software fade family, subject to final parent admission |
| `00478e50` | `bh_screen_quad_queue`, `src/bh_screen_quad.c` |
| `0047b710` | `bh_sound_listener`, `src/bh_sound_listener.c`; audio listener, **not** camera |
| `0047ddf0`, `0047de20` | Verified CRT rand/ftol; reuse CRT, exclude from game-function assignments |

Indirect dependencies omitted by that direct-call list:

* Both selected weapon bytes `+2b6/+2b7` index the ten borrowed weapons at
  `+258`, then invoke weapon method `+130`. Actual constructor `00421d20`
  constructs ten records via factory `00433ea0`; table `0048fa60` has six method
  pointers per record. Index4 is `00432ff0` arc, index5 `004314f0` target reticle;
  the other eight HUD entries share verified RET `0047d680`, represented by the
  existing explicit NULL sentinel convention. Do not invent eight callbacks.
  See `weapon-hud-target/dispatch-table.json`, `dispatch_review.py`.
* Arc source `src/bh_weapon_hud_arc.c` includes complete producer `00432050`.
  `src/bh_path_queue.c` supplies `00455ee0/00477880/004779e0`, and the corrected
  real ribbon `00455f70/004569b0` is `src/bh_sprite_oriented.c`. Target reticle
  source is `src/bh_weapon_hud_target.c`. Both still require the actual installed
  target event3 query and valid target lifetime; the implementations create none.
* `00478390` installs segment consumer **`004777c0`**, which calls complete
  append **`00455e40`** twice, then `004569b0`. These two formerly missing small
  owners are now `src/bh_segment_queue.c`, tested independently and through
  actual native segment images. Check final parent integration before admission.
  `00455e40` implicit UV differs from the explicit UV of `00455ee0`; do not
  conflate them. This closes the previously missed segment native branch.
* Two raw reservations install `00477360` mode and `00477680` screen-quad
  consumers; both already have real source definitions. Preserve exact raw
  payload/partial reservation effects, including integer width/2 before float.
* Scene traversal has installed entity draw methods `+e8`, refresh methods and
  mutation-sensitive successor reads. Existing scheduler owners are real, but
  borrowed scene lists must contain real source methods. A generic entity draw
  field `+e8` is **not** the player's embedded-camera mode at the same offset.

## Required live bindings and lifetime closure

The full callback has no guessed global table or vtable substitute available.
It needs one explicit context mapping the following already-observed state to
real source owners. This is a source binding contract; a compact context does
not establish original constructor completion or permit copying mutable state
once at entry when callbacks can change later reads.

* Player pose `+0c`, basis `+18/+24/+30`, angles `+3c`, velocity `+48`, current
  scene `+c4`; embedded camera starts at `+e8`, with draw/listener eye `+160`,
  angles `+16c`, velocity `+178`. Reuse the verified player camera storage and
  existing camera owners, rather than treating camera mode as an entity method.
* Player resource `+21c` is a real borrowed model/LOD descriptor handle; keep
  `BHRenderResources`, model instances, images and their contexts alive through
  queue drain. Global `00496338` is actual `cockpit.bsp` from `004474c0`, not a
  synthetic resource handle. This callback does not load models or construct
  weapons; full `00421d20/00433ea0` construction remains a separate frontier.
* Shared gauge/counter/ammo/countdown state already has verified
  `BHHudStatusState` bindings: `+230/+234/+238`, `+23c/+24c`, weapon amount
  `+10c`, selectors `+2b6/+2b7`, flags `+2b8`, countdowns `+2b9/+2ba`.
  Keep storage shared with callbacks and copy back actual mutations when an
  adapter is required. Full callback additionally mutates damage flash `+254`
  after fade; it reads transient effect `+240/+244`, aim `+280`, aim offset
  `+28c`, gate byte `+2bb`, flags `+2b4`, four motion vectors `+2c0..2ef`, and
  eight trail endpoints `+2fc..35b`. Do not fabricate these from screen output.
* Global `004dfb08` is the shared player/mode count used by gating/scoreboard;
  `004df6b4` drives the `<75` and `>=150` branches. Bind live signed values,
  display width/height `004e2998/004e2dec`, confirmation-overlay gate `004df6b0`, actual
  material handle `004d1aa0`, eight background slots `004ce2b0`, shared CRT
  state, mutable radar frame/world center, and current near plane `0049175c`.
  Camera-pose queue updates are deferred: preserve the original producer and
  consumer order instead of eagerly replacing the current camera everywhere.
* Actor markers head `004dfb28` uses event3 queries, kind `+fc`, successor `+f0`
  read after callbacks. Pickup head `004dfb3c` uses distinct successor `+118`,
  already `BHPickupState.global_next`. Entity storage must survive each query
  and subsequent source draw. NULL/empty lists are valid inputs; fake receivers
  do not establish production query-method completion for populated lists.
* Scoreboards borrow actual eight `004dfad8` slots, local score/name/index and
  actual text page. `00427920` packet owner statically establishes remote actor
  slot `+114`, score `+11c`, name `+11e` and marker-list participation; it remains
  unrecovered network lifecycle, not a render dependency to replace with a stub.
  `weapon-hud-target/remote-label-review.json` records exact review facts.
* Images are real resolved source handles: horizon `004f005c`, aiming quads
  `004f0040/004f00d0`, trail `004f00cc`, overlay `004f002c`, particle `004f54d8`,
  confirmation overlay `004962f4`, plus HUD/status/weapon atlases. Startup
  `004474c0` builds page `004d198c` from `textures/tvfont.pcx`, renders the exact
  strings `Abandon`, `Game ?`, `[Y/N]`, then gets its image using already-admitted
  `00470d40` (`bh_atlas_page_image`, `src/bh_atlas_text.c`). It is not a crosshair
  asset. Reuse the real page/create/clear/cursor/text owners to bind this image;
  supplying a generic crosshair would change behavior. Startup composition as
  a whole remains unrecovered. Native image loads use verified palette policy2 and require
  `bh_texture_preallocate` after loading newly referenced image formats.
* Complete native queue dispatch must use all actual mode/camera/material,
  resource, horizon, segment, plane-quad, oriented-quad, screen-quad/rectangle and
  path consumers with one consistent queue/camera/clip/texture/renderer lifetime.
  No original address is callable, mapped or loadable by the shipping target.

## Exact selected-weapon registry contract to settle first

No such registry currently exists in source. This is a proposed interface
contract, not an admitted constructor, a new fake dispatch table or a claimed
full weapon layout. The final bounded shared-callback family adds
`BHWeaponCommonState` in `src/bh_weapon_common.h`: actual+e8..117 subrecord over
source BHEntity storage, with embedded shared HUD state at+f8 and ammo at+10c.
Its accessor makes those views share real storage, but supplies no factory,
identity registry or gameplay method lifecycle. Check final parent admission
for complete shared delay/reset owners `004309b0/00431050`. Before implementing
the complete player callback:

* **Identity and ownership:** each entry must identify the actual source-owned
  weapon allocation, its pool generation/lifetime and constructor index0..9.
  The owning player holds the actual ten-entry sequence corresponding to
  `+258..+27c`; selection bytes remain borrowed live storage. Registry entries
  borrow allocations and do not silently extend released object lifetimes.
  A reused pool address with a changed generation is a different weapon.
* **One authoritative state:** an entry supplies the same weapon's signed ammo
  `+10c`, target `+f8`, signed charge `+fc` and verified selected-HUD method
  identity `+130`. All gameplay/query writers must see the same storage.
  `+f8/+fc` are contiguous and can be an actual `BHWeaponHudState` subobject in
  a reviewed source record; this must be established by its real owner/layout,
  not by assuming a compact copied struct is an original allocation. The ammo
  view and HUD view must not own divergent copies or require deferred copyback.
* **Live resolution:** after markers, reload primary selector, then its current
  object identity/method. After the entire primary callback, reload secondary
  selector and resolve its current object again. Do not cache both selectors,
  weapon entries, target, charge, flags or flat mode before the first callback.
  Each actual target query receives the original40-byte event3 packet; charge
  must remain live through that query and (for arc) its three subsequent rand
  calls. Reload flat mode and all status inputs after both callbacks. A status
  adapter may be formed at that boundary only because the actual status owners
  perform no callback; publish countdown mutations to authoritative player
  storage immediately. Unknown object/method or a released generation reports
  an explicit source binding error, never a generic successful no-op.
* **Dispatch:** source method identities are the verified table entries:
  index4 arc, index5 target, others the verified common RET sentinel. The
  constructor must establish that identity; registry position alone must not
  guess which method an arbitrary object owns. Real target query methods and
  their synchronous object lifetimes remain required. Isolated test receivers
  cannot become production defaults.

Actual initializer/writer frontiers are concrete. Complete factory `00433ea0`
(203B/46instructions) calls admitted pool acquire `00476f20`, initializes
`+e8/+ec/+f0/+f4/+f8/+fc=0`, `+100=-1`, `+104/+108/+10c/+114=0`, sets ammo limit
`+110` from `0048fa38[index]`, installs all six table methods and registers query
`00433e50` via admitted `00476e90`. Complete player constructor `00421d20`
(1204B/298instructions) writes the actual ten pointers, selectors0/9, marks
weapon0/9 `+ec=1`, then adds/clamps their initial ammo by300/1 against `+110`.
It installs player refresh `0041c860` at **`+c8`**, step `0041e850` at `+cc` and
query `0041d7c0` at `+d0`; embedded-camera mode is still `+e8`. Its nonzero first
argument branch returns early as a different partial/proxy construction path;
do not give that path invented ten-weapon ownership. Fresh whole PE-verified
constructor/factory exports are in `next-player-review/` and are read-only,
not source admission.

Before treating the registry as populated by gameplay, recover/review the
factory's actual installed query `00433e50` and the real field writers behind
index4 methods `00432c60/00432560/004327a0/004324a0/00432510`, index5 methods
`00431180/00431810/004310c0/00431130`, and player query/step
`0041d7c0/0041e850`. Trace their writes to target/charge/ammo/selectors, target
replacement/release and weapon cleanup; this list is a concrete review frontier,
not a claim that every listed method is necessarily a direct render prerequisite
or that its complete writer closure is already known. The existing pool/query
owners provide real storage/lifetime mechanisms, but not these gameplay writers.
A bounded full-render source harness may borrow legitimately owned reviewed
records; it must label those bindings and keep gameplay construction open.

No partial `bh_player_scene` source was started in the final24-minute window:
the whole owner's numerical/order tests and this binding contract cannot be
completed responsibly by the10:47 integration cutoff. This is a bounded scope
decision, not a missing-M3 prerequisite or a claim that explicit bindings are
impossible.

## Next block acceptance questions

1. Main gate is `byte+2bb==1 && 004dfb08!=0`; failing it skips only the main
   section. Model/tail drawing still happens. Test both gates and all tail
   branches, not only an active local HUD.
2. Preserve main order: listener, queued camera, horizon, material/eight
   backgrounds, fade/damage mutation, aiming quads, HUD/actors/pickups, both
   selected weapons, status, multiplayer scoreboard, scene owner, confirmation overlay,
   then optional transient32-particle burst. Its burst consumes **128 CRT rand
   calls** (three normalized coordinates and one angle per particle), including
   queue-full drops. Arc consumes its own real65 calls when full17point output
   is reached; preserve combined global ordering rather than independent RNGs.
3. Tail draws resource `+21c` when nonzero. For global frame `<75`, optional
   mode/overlay records precede cockpit resource and four trail segments. Trail
   consumes **four** random calls even if submission drops. Signed raw motion
   square-root expression, negative widths, FP stores and fade integer color
   conversion require raw instruction review; decompiler expressions alone
   are insufficient. No universal broad player snapshot is justified.
4. Compile the complete owner in its real TU, link current actual definitions,
   run isolated state/queue mutation tests and an offscreen source native scene
   with genuine owned assets and real consumers. Do not admit just a callback
   table or copied state record. Full player constructors/network/script startup,
   software renderer dispatch, PC24, CD audio and DirectPlay remain distinct
   open product paths; mark them explicitly, without deleting them or making
   M3 replay a prerequisite for this source owner.

This assessment executes no game/original code. Resume only when authorized;
the current parent block's deadline is 2026-10-25 10:56:47 UTC.

Final stop: user requested exactly300 admitted functions and this checkpoint achieves it. Resume only when authorized. Target/arc ready/cancel/tracking, target-removal query and weapon-index3 float timers now have source owners and live common state; constructor/registry, trigger writers and remaining firing/mission ownership stay open. No full player scene callback or partial prefix was admitted.
