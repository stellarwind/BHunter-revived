# Original TCP/IP host session creation — 2026-10-24

One bounded run `04a72c7a5e54` extends NETWORK/STINGER/TCP-IP/CONNECT to the original new-game action. It uses the previously documented private loopback CFG fixture and verified-child key messages. It stops after the original new-game callback returns, at callback 520, without a join action or subsequent hosted-gameplay claim.

## Actual creation results

The original TCP/IP initializer and first session enumeration again return S_OK. The session-list new-game item is checked active/selected before Enter. Original callback 0042f360 calls host owner 004766e0, then unconditionally writes mode 2; therefore mode 2 alone would not prove host creation.

The probe instead captures the real DirectPlay calls and results:

| Operation | Original callsite | Request/result |
|---|---|---|
| Open | 004767c9 | flags 0x82 (CREATE with RETURNSTATUS), descriptor flags 0xc4, requested max players 8; S_OK |
| CreatePlayer | 00476810 | server-player flags 0x100; S_OK, returned player ID 1 |
| Host helper return | 004766e0 owner return | EAX 1, actual host-ready 1, local player ID 1 |
| Menu callback return | 0042f360 owner return | mode 2, host-ready 1, local player ID 1 |

The descriptor's application GUID words are retained in the raw report. Descriptor flags 0xc4 combine the pinned SDK's MIGRATEHOST, KEEPALIVE and NODATAMESSAGES bits. Eight is an actual requested session capacity, not an eight-peer test or proof of how many human clients the game supports. The primary evidence is the actual native API HRESULTs and returned player ID, not a patched host flag, invented handle or menu label.

## Boundary and limits

The copied original child is stopped at the new-game callback return after verified breakpoint restoration and private-key cleanup. Termination is diagnostic. Original session-close/destroy-player behavior, continued hosted simulation, join/discovery between peers, packet exchange, migration and independent-machine interoperability remain unvalidated. No join callback executes; its guard remains active. Loopback is the configured address, not packet-capture proof that every transport-internal operation is confined to loopback.

The earlier TCP/IP setup stall remains unexplained. Successful setup and host creation under later probe variants do not prove a deadlock fix or repeated-operation reliability. The older failed and successful setup traces retain separate source snapshots/evidence; no linked-test results are transferred to this new variant.

## Evidence and isolation

The new native/driver variant compiles, links and executes successfully. Complete host/new-game/join owner exports and role/boundary classifications are in host-session-review/; the join owner is reviewed for distinction, not executed. Unknown engine provenance remains unresolved and no gameplay recovery counts change.

`python tools/check_m3_host_session.py` verifies this run's source snapshots, exact CFG chain, actual setup/Open/CreatePlayer results, host state, guarded input path, restored instrumentation, private/Windows file and registry invariants, and all original EXE/BIN/MDF/MDS hashes. `host-session-validation.json` records the bounded result. Existing private profile/SourceDir, DDrawCompat, explicit 128 MiB original CRT initial heap and relative cursor adaptations remain qualified. No computer-use skill, visual identification, desktop-wide input, source-game admission or M3 completion.
