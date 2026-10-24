# Original TCP/IP setup and session-list capture — 2026-10-24

One diagnostic replay `c41fc2a39889` of NETWORK/STINGER/TCP-IP/CONNECT completes 600 callbacks, with the same loopback CFG fixture and input schedule as the previously stalled route. Actual original InitializeConnection and first EnumSessions return S_OK, zero sessions are found, the session-list menu is active at callback 500 and remains active at the 600-callback stop. Breakpoint restoration verifies before diagnostic termination. This establishes original setup/session-list bring-up, not host/join, packet exchange, normal network shutdown or independent-peer interoperability.

The prior timeout `a6528d0cdfed` is preserved in NETWORK_TRACE.md and remains a failed scenario. Its cause is not established. The new probe records module loads and retains thread handles, changing instrumentation and timing; a successful replay under this new variant is not proof of a deadlock fix or reliable repeated network operation. No stalled-thread contexts were captured from the game in this successful run because the timeout did not recur.

## Diagnostic changes and controlled verification

The debugger now records actual module paths, loaded bases and image sizes from debug load events. This run includes the original game's system dplayx.dll and dpwsockx.dll, alongside its graphics/audio dependencies. Thread-create/exit ownership is retained explicitly. On a future timeout, the watchdog requests DebugBreakProcess and its breakpoint debug event stops the target before thread snapshots and breakpoint restoration, then diagnostic termination. Failure to obtain that stop is not relabeled verified cleanup.

A separate source-built waiting-child test in tests/m3_timeout_probe.c includes the real diagnostic helper definitions and exercises real Windows debug events. It starts a waiting worker, requests DebugBreakProcess, captures actual contexts and 32 raw stack words for three stopped threads, arms/restores a real breakpoint in the child, verifies original bytes and terminates the child. Compile, link and execution pass in timeout-control.json. No original BHUNTER code or data executes in this control. This verifies the shared capture/restoration mechanics; it does not prove the complete production watchdog route during the old game stall. Raw stack words are not presented as a validated unwind/call stack.

The control's exact compiler/linker commands, source snapshots/hashes and binary hash are retained. It uses the same pinned compiler/include/lib setup as the diagnostic. It can be rebuilt from those commands with the manifest environment. This control required no extra game replay.

## Evidence and remaining network scope

`python tools/check_m3_network_setup.py` audits the successful game's own source variant, loopback seed, actual API results, active session-list state, modules, verified restoration and the independent capture/restoration control. `python tools/check_m3_network_evidence.py` separately audits the older failed variant. Evidence is not transferred between variants. network-setup-validation.json reports success of this bounded setup scenario while retaining earlier_stall_explained=false and host_join_peer_validated=false.

Private registry cleanup, protected Windows file invariants and all original EXE/BIN/MDF/MDS hashes verify. Existing DDrawCompat/heap/private profile/relative cursor adaptations remain explicit. All key messages target the verified child HWND; no computer-use skills, visual identification or desktop-wide input. Stop guards prevent original host/join callbacks; they were not reached. No shipping source or recovery count changed. M3 remains active for objective/destruction/retry, fuller sound and network behavior, and other recorded gaps.
