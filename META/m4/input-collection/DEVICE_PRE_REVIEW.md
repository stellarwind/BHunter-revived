# Input device lifetime and key defaults static review

Before implementation, classify00473d60 and00473fe0 as ordinary complete engine
input/key-name lifetime owners, engine attribution unresolved. Imports/verified
CRT allocation and formatting remain dependencies, not separate gameplay
recovery targets. Neither owner is compiler EH/stack support, thunk or shared
tail. Both have complete RET-ended boundaries checked against immutable PE by
export_devices.py:00473d60..00473fd2=627bytes/143instructions and
00473fe0..00474016=55bytes/16instructions. device-owners.json records SHA256;
original/ contains full raw TSV/JSON and decompiler evidence. No original game
or native input/cursor code executed for this export.

src/bh_input_key_names.h is generated directly from original255-pointer table
00491d10..0049210b (0049210c exclusive), not256 entries.126 pointers are nonzero,
129 NULL. Every pointer and its raw NUL-excluded ASCII string bytes is retained
in key-names.json together with raw pointer table and SHA256. C literals use
fixed three-digit octal escapes, so quotes/backslashes/control bytes cannot
change token meaning and following digits cannot extend an escape. No original
pointer addresses enter source runtime; static text is reusable local data.

Initializer loops VK0..254. Localized letter replacement only applies to table
slots00491e14..00491e78 inclusive, exactly VK65..90. It requires mapped low byte
in A..Z and unequal to original VK. Null defaults choose Key #%d when mapped
byte<32, with **VK** as integer argument; otherwise Key '%c' uses mapped low
byte. Both formatting argument pushes must be preserved rather than inferred
from decompiler's omitted varargs. Default nonnull text is borrowed/static;
new localized/fallback text is allocated and requires separate ownership.

Initialization resets history/list head/current/previous keys, captures
joysticks0/1 with100ms period/unconditional flag0, queries capabilities after
successful capture, clears axes/deltas/filter, recenters cursor160/100,
sets accumulator words512, deadzone mask all ones, calibration flags0,
extrema min+1/max-1. Shutdown conditionally releases captured0/1 and clears
both enabled flags. Source-owned name disposal is additional platform/source
lifetime handling, not an original shutdown-body claim.
