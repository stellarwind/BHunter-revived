# Original TCP/IP setup: partial success, then stall — 2026-10-24

One original-reference run `a6528d0cdfed` follows a different menu route: NETWORK → STINGER → TCP/IP Internet → CONNECT. It does not select GAME/prologue or join a discovered session. The diagnostic compiles and links, but its full scenario fails: after successful original connection setup, simulation callbacks stop returning and the 45-second watchdog terminates the child. No second attempt was launched in this block.

## Controlled input and provider selection

A private 644-byte CFG starts from the output of the verified normal quit. Only bytes 140–148 change to the address `127.0.0.1`; the entire 128-byte address field at settings+0x8c is explicitly initialized and retained in `network-seed.json`. Original world bootstrap copies settings 00496ac4 to the type-1 provider address at record+0x1c, and the original address builder consumes that field. The loaded settings snapshot matches this fixture. No registry/global/connection pointer or API return is patched to fake success.

The actual original provider table contains TCP/IP Internet at menu index 3, record 004efa00, type 1. The probe verifies the TCP/IP GUID, uses original child-only arrow/Enter messages, and checks active menus/selections before each action. It verifies the actual selected type/address before allowing the original initializer to run. Host/new-game and join-game callbacks have stop guards; neither is reached. Loopback is the configured target; this trace is not a packet capture proving the destination of every transport-internal operation.

## Actual results

At callback 440, original 00476410 receives the verified TCP/IP record and loopback address. The real InitializeConnection call returns S_OK at 0047649c. The real first EnumSessions call returns S_OK at 0047653e, with zero sessions. The original initializer returns 1, with actual non-null DirectPlay and lobby interfaces. This advances beyond the earlier source-built host-capability probe: the original game's address construction, connection initializer and enumeration call actually execute.

The earlier static blank-string shortcut is specific to type 2 (modem), not this type-1 TCP/IP provider. API results are captured independently of the owning function's return value; reaching a menu target or receiving initializer return 1 alone would not prove enumeration.

Afterward no simulation-return progress or planned callback-442 key release is observed. The watchdog captures the main thread's EIP/stack/control word and terminates the child with diagnostic code 4d330102. The exact cause is undiagnosed: there is insufficient thread/module evidence to attribute it to DirectPlay, renderer, synchronization or another owner. Session-list interaction at callback 500 and the 600-callback stop are not reached. This is not a passed network scenario, host/join, peer exchange or normal shutdown.

The existing timeout path kills the diagnostic process without a breakpoint-restoration verification event. That absence is explicit in the checker/report; no verified restoration is claimed for this run. All instrumentation existed only in the now-terminated copied child. Original EXE/BIN/MDF/MDS hashes, protected Windows files and original SourceDir registry views remain unchanged; the private registry key is removed. Targeted key messages cannot leave a desktop key held because no desktop-wide input was sent.

## Evidence and next question

Complete setup/menu/provider/address-owner reviews and role/boundary classifications are in network-review/. Unknown network/UI engine provenance is retained. `python tools/check_m3_network_evidence.py` verifies source snapshots, CFG changes, actual provider/address, API results and the retained timeout; it reports `evidence_audit_passed=true` separately from `scenario_passed=false`. `network-validation.json` records that distinction. The current network probe path stays WIP; there is no gameplay source admission.

The next diagnosis must capture all relevant thread contexts and module ownership at the post-setup stall, and distinguish the connection/API success from subsequent menu/worker progress. It must also handle timeout cleanup explicitly. Repeating the same route without those new observations would add no evidence. Original host/join, independent peers and broader multiplayer modes remain missing. M3 remains active.

A later, separately instrumented successful setup run and original-free timeout-helper control are recorded in NETWORK_SETUP.md. The original stall remains unexplained; this failed record is not superseded or reclassified as passing.
