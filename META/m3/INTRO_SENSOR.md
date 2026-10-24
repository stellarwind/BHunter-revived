# Original introductory sensor reached by keyboard flight — 2026-10-24

One bounded run `14bdee1a5329` selects original GAME/STINGER/prologue, flies into the introductory sensor's actual region and captures event 2010 returning through the original mission consumer at callback 544. This is the first actual introductory trigger capture, not another straight-line collision replay. No second attempt was needed.

## Sensor condition and actual target

Script opcode 10 installs sensor step 00443ac0. Its zero-range case tests whether the player point lies inclusively inside the owner's world bounds, following owner+0x1c8 when present; positive range uses a different distance check. Thus raw SENSOR2.BSP local bounds or its placement coordinate alone do not define the runtime trigger region. Complete sensor/callback/controller exports and classifications are retained in sensor-review/.

The diagnostic follows the actual scene ring to placement ordinal 4939, then follows the same optional bounds owner. It captures world bounds (-90,-25,30)..(-30,-10,90), with the original placement owner distinct from its bounds owner. These are read from original objects, not invented target dimensions. Starting player position is (0,-14,120), outside that region.

## Recorded input policy and original response

The new m3_flight_probe.h is diagnostic keyboard policy, not recovered ship physics. It reads actual position/basis/velocity and selected profile keys, checks the active original ship controller 00416120, and sends only verified-child key messages. It first applies short right/pitch pulses to measure steering response signs. Both measured signs are -1 in this run; no assumed sign or altered input axis is imposed on the game. Subsequent observations every six completed callbacks steer toward the actual bounds centre, using recorded thrust/brake decisions and velocity feedback. Physical collision and all original flight integration remain active.

There are 23 flight observations. At callback 542, recorded position (-60.5112,-12.7425,88.9495) lies inside the captured bounds. At callback 544, the original mission consumer receives type 0x0e, ID 2010, with sender equal to the target placement owner. A return breakpoint confirms that the original consumer body executes and returns before diagnostic stop. The preceding initialization events 9991/3332 remain separately recorded. The reviewed dispatcher case 0x7da is introductory text/mission flow; this does not establish an episode-ending/completion event.

The adaptive policy makes an input choice from each observed state and records the resulting key schedule. This is not a claim of deterministic world replay across machines or timings. The checker reconstructs key-state transitions from actual posted messages, validates calibration signs, distances and bounds entry, and connects the observed sender/ID to the actual handler return. No injected event packet, position/velocity/health write, gameplay flag patch or direct callback invocation is used.

## Evidence and remaining scope

`python tools/check_m3_intro_sensor.py` audits actual compile/link/run commands, all three executed source variants (native C, Python driver and new policy header) through their archived hashes, input/position/event observations, restored breakpoint bytes, private key cleanup, protected Windows files/registry and unchanged EXE/BIN/MDF/MDS references. `intro-sensor-validation.json` records the result. The policy header is explicitly included in source snapshots; its helper code contains no gameplay memory/context write APIs.

The copied original child terminates diagnostically after the mission handler return. No normal-shutdown evidence is claimed for this run. Existing private SourceDir/profile, DDrawCompat, explicit original CRT initial-size input and native relative cursor adapters remain qualified. No computer-use skill, visual identification or desktop-wide input. Original x87 samples remain 027f. Normal source integration and recovery counts are unchanged.

M3 still needs an actual objective/episode completion result, destruction/retry, fuller sound and peer/network behavior, and other recorded gaps. This observation closes the introductory sensor activation gap only; it does not rename that bounded success as completion of the larger milestone.
