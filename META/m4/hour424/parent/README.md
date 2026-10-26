# Full force-feedback startup constructor

Complete00471260 is350 bytes/113 instructions. It is an ordinary engine startup
owner with two original arguments (HINSTANCE, HWND); the stale decompiler shows
only one. Raw stack+50 after the frame verifies the second HWND. The import
DirectInputCreateA and standard library data are not gameplay recovery counts.
All complete bytes and the original input format are independently PE-verified.

The source uses real DirectInputCreateA(version500), EnumDevices(type4,flags101)
and the admitted Device2A enumeration callback. It retains partial native roots
on failure, configures actual c_dfDIJoystick, sets foreground/exclusive level5,
and constructs the exact52-byte Cartesian/offset constant-force effect: duration
infinite, sample0, gain10000, trigger-1/repeat0, two axes0/4, directions0/0,
no envelope,4-byte zero magnitude. It then enables feedback and calls the real
recovered start owner; its final ignored Acquire/Start errors and started latch
retain original behavior. Existing owning stop releases partial/successful refs.

Explicit persistent source context replaces globals, with fresh-owner guards.
The official library joystick format is reused and every one of its44 object
GUID/offset/type/flag tuples plus all descriptor scalars matches original data.
Required GUID definitions come directly from the pinned SDK in a count-zero
platform data TU, not guessed globals or invented device/vtable implementations.
Existing input SDK header/import library hashes already cover these dependencies.

`build_check.py` compiles actual TUs with pinned VC5/DX5 declaration context,
links real native libraries and owning definitions and passes105 checks plus
missing-module link control. Native preflight enumerates feedback-only devices
without acquisition. This host has none; two whole-constructor calls execute
actual creation/enumeration/no-device return, retain the root, and dispose it
through the real source stop. There are zero physical Acquire/effect Start calls.
The fixture refuses physical execution on a feedback-equipped host; a separate
human-authorized hardware test is needed there.

Hardware-positive format/cooperative/CreateEffect/Acquire/Start remains static
reviewed, not executed. No effect is mocked and no successful effect is claimed.
No game/original code, physical input/cursor, UI capture or new playback runs.
Parent full/clean integration and fresh linked evidence are required for admission.
