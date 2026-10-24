# Original exposed features and loaded controls — 2026-10-24

This inventory derives from actual constructed menu objects, the original input-display/binding callbacks, original executable label tables, and the memory snapshot from verified saved-CFG reload. No game launch or visual inspection was used in this block. It covers exposed labels and 44 control fields across three loaded profiles; it does not establish complete gameplay feature execution.

## Exposed choices

- Ships: BARLETTA, STINGER, VIPER.
- Episode slots: prologue, episode 1, episode 2, episode 3, episode 4, episode 5, episode 6, episode 7, episode 8. Labels alone do not establish unlock state or nine independently playable missions.
- Primary weapons: ion gun, lead buster, magnetic plasma, nitro crackers, stun ray.
- Secondary weapons: homie missile, sonic blast, tnt mine, fat lady, static coating.
- Cameras: cockpit camera, tail view camera, trace camera, fly by camera, circle camera, side view camera.

The 18 constructed menus contain 118 items. Setup exposes 3D acceleration, graphics device, software resolution, sound-effect and CD-music levels, and input profile NEW/EDIT/DELETE. Network pages expose connection selection, new/join game, modem/call number, serial port/baud/stop bits/parity/flow control, and IP/host name. Eight join rows are display slots, not proof of an eight-player limit. Repeated fire labels are alternative bindings, not additional weapon types. Full item labels, target-menu links and callbacks are retained in feature-inventory.json.

## Exact loaded bindings

Labels below come from original key/button/axis pointer tables, not a newly invented naming scheme. The records are those actually loaded in run c42a7051b694. The apparent keyboard move-up/down reversal is preserved. Joystick and mouse camera axes use the nonsequential offsets selected by original display callbacks.

| Control | Profile byte | Standard | MouseView | Joy 1 View |
|---|---:|---|---|---|
| keyboard move up | 0x1c | Cursor Down | Cursor Down | Cursor Down |
| keyboard move down | 0x1d | Cursor Up | Cursor Up | Cursor Up |
| keyboard move left | 0x1e | Cursor Left | Cursor Left | Cursor Left |
| keyboard move right | 0x1f | Cursor Right | Cursor Right | Cursor Right |
| keyboard accelerate | 0x20 | Key 'A' | Key 'A' | Key 'A' |
| keyboard brakes | 0x21 | Key 'Z' | Key 'Z' | Key 'Z' |
| joy move up down | 0x22 | Joystick 1 Inv Y | Joystick 1 Inv Y | Joystick 2 Inv Y |
| joy move left right | 0x23 | Joystick 1 X | Joystick 1 X | Joystick 2 X |
| joy accl brakes | 0x24 | Joystick 1 Inv Z | Joystick 1 Inv Z | Joystick 2 Inv Z |
| mouse move up down | 0x25 | Mouse Inv Y | <No Mouse> | Mouse Inv Y |
| mouse move left right | 0x26 | Mouse X | <No Mouse> | Mouse X |
| mouse accl brakes | 0x27 | <No Mouse> | <No Mouse> | <No Mouse> |
| fire primary | 0x28 | Control | Control | Control |
| fire primary | 0x29 | Joystick 1 But 1 | Joystick 1 But 1 | Joystick 2 But 1 |
| fire primary | 0x2a | Mouse Button L | Mouse Button L | Mouse Button L |
| toggle primary | 0x2b | Joystick 1 But 3 | Joystick 1 But 3 | Joystick 2 But 3 |
| ion gun | 0x2c | Key '1' | Key '1' | Key '1' |
| lead buster | 0x2d | Key '2' | Key '2' | Key '2' |
| magnetic plasma | 0x2e | Key '3' | Key '3' | Key '3' |
| nitro crackers | 0x2f | Key '4' | Key '4' | Key '4' |
| stun ray | 0x30 | Key '5' | Key '5' | Key '5' |
| fire secondary | 0x31 | Space Bar | Space Bar | Space Bar |
| fire secondary | 0x32 | Joystick 1 But 2 | Joystick 1 But 2 | Joystick 2 But 2 |
| fire secondary | 0x33 | Mouse Button R | Mouse Button R | Mouse Button R |
| toggle secondary | 0x34 | Joystick 1 But 4 | Joystick 1 But 4 | Joystick 2 But 4 |
| homie missile | 0x35 | Key '6' | Key '6' | Key '6' |
| sonic blast | 0x36 | Key '7' | Key '7' | Key '7' |
| tnt mine | 0x37 | Key '8' | Key '8' | Key '8' |
| fat lady | 0x38 | Key '9' | Key '9' | Key '9' |
| static coating | 0x39 | Key '0' | Key '0' | Key '0' |
| cockpit camera | 0x3a | F1 | F1 | F1 |
| tail view camera | 0x3b | F2 | F2 | F2 |
| trace camera | 0x3c | F3 | F3 | F3 |
| fly by camera | 0x3d | F4 | F4 | F4 |
| circle camera | 0x3e | F5 | F5 | F5 |
| side view camera | 0x3f | F6 | F6 | F6 |
| keyboard look left | 0x40 | Numpad 4 | Numpad 4 | Numpad 4 |
| keyboard look right | 0x41 | Numpad 6 | Numpad 6 | Numpad 6 |
| keyboard look up | 0x42 | Numpad 2 | Numpad 2 | Numpad 2 |
| keyboard look down | 0x43 | Numpad 8 | Numpad 8 | Numpad 8 |
| joy look left right | 0x45 | Joystick 2 X | Joystick 2 X | Joystick 1 X |
| joy look up down | 0x44 | Joystick 2 Y | Joystick 2 Y | Joystick 1 Y |
| mouse look left right | 0x47 | <No Mouse> | Mouse X | <No Mouse> |
| mouse look up down | 0x46 | <No Mouse> | Mouse Y | <No Mouse> |

The fixed keys installed by 00421740 include Escape (abort/pause), Tab (action 0x20), Y (confirm abort) and N (cancel abort). Tab behavior is not named more specifically without its owning action review. Only acceleration and the captured abort/confirm/quit path have relevant runtime traces here; the remaining control fields are inventories.

## Evidence and remaining work

`python tools/m3_feature_inventory.py` reconstructs the report from pinned EXE data and retained captures, checks the four display-switch mappings, reads every original display label with bounded address lookup, verifies all offsets 0x1c–0x47 occur exactly once, and preserves evidence hashes. `--write-report` explicitly updates the generated inventory. Complete reviewed callback bodies/roles are in control-review/. No callback was recovered into the source game.

Remaining M3 execution includes objective/destruction/retry, weapons/cameras and other controls, settings/profile modification and progress persistence, network sessions/modes, wave playback and deterministic world/seed/time behavior. CD playback/notify looping has a recorded current-host MCI/device blocker. The feature-execution-complete flag stays false; successful inventory reconstruction is not a milestone pass.
