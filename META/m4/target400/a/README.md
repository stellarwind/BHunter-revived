# Menu configuration selection and assignment owners

Ten complete ordinary game UI owners were restored in `src/bh_menu_settings.c`:
0042caa0/cae0/cb10 (menu entry and submenu selection), 0042e360/e540/e690/e7e0
(four input profile byte-assignment callbacks), 0042ea70 (188-byte network
profile selection), and 0042eae0/ee50 (selected profile address/phone text).

Classification preceded implementation: each is a separately referenced menu
callback with its own entry and RET; none is a CRT/library/EH/import/shared-tail
assignment. The original input-profile bytes and menu fields/callback dispatch
are directly visible in whole-owner disassembly and decompilation. Original
reference media are unchanged. `ReviewMenu.java` establishes the previously
uncreated function metadata using map-reviewed contiguous boundaries and emits
complete instructions; it does not patch bytes. CLI disassembly is limited to
ten instructions by the bridge, so the Java export is the complete evidence.

Bindings are explicit borrowed source state, not invented globals or vtables.
Input assignment uses the existing real76-byte `BHInputProfile`; network records
are188 bytes with bounded strings at+1c and+7c. Menu integer selection+1d0,
float display+1e0 and row+18 refresh contracts are represented by named source
fields. The optional refresh callback is invoked after mutation even when the
row index is outside the assignment switch. Tests link that branch through an
explicit source adapter into actual recovered0042cae0, showing real subsequent
selection effects. Original row-label rendering and menu/profile construction
remain external source bindings and are not admitted by this work.

The pinned VC5 build compiles the real translation unit, links
`build/target400-menu-settings/bhunter-menu-settings-check.exe` and executes
976449 checks. Omitting that TU fails linking on its real owner references.
Checks cover every byte value at all12 fixture rows for all four key callbacks,
exact76-byte profile preservation, optional refresh, first-byte-only behavior,
signed selection including LONG_MAX and32-bit action wrap, independent menus,
188-byte record stride, last-positive selection retention, all fitting string
lengths, empty strings, exact NUL and untouched-tail effects, and invalid guards.
The LONG_MAX reference is explicitly stored to binary32 before comparison so
VC5 x87 extended expression precision is not confused with a stored float.

Integration: add `src/bh_menu_settings.c` and `tests/menu_settings.c`, declare
and call `int __cdecl bh_menu_settings_checkpoint(unsigned long *)`. Scenario
name is `menu-settings`. Re-run `python META/m4/target400/a/build_check.py` for
the independent focused target. Parent owns full real/clean integration and
durable ledger admission. No game, original code, gameplay automation, input
capture, audio or network operation occurs. M3 remains skipped and M4 incomplete.
