# Basic pickup constructor

Whole original00441b80 is restored as bh_pickup_basic_spawn in src/bh_pickup_basic.c, with src/bh_pickup_basic.h and tests/pickup_basic.c. Boundary:340 bytes/77 instructions; ordinary application/entity owner, not CRT/import/compiler/shared tail. It is separate from already-admitted00441a00/bh_pickup_spawn. All direct and registered callback dependencies use real admitted definitions; no prior pickup sources were edited.

SOURCE_CONTRACT.md documents the six original arguments, explicit persistent context, exact RNG and fade transport, default draw including type9, zero-resource/no-fallback behavior, signed clamp, list ownership and source guards. disasm.json/decompile.json/boundary.json preserve full reference evidence; review/ contains the independent audit. Historical planning is ../script-frontier/pickup-constructor/.

Focused exact source/header/test hashes and dependency object hashes, compile/link commands,44 passing checks, and expected missing-module link failure are in focused-results.json. Reproduce using `python build/pickup_basic_focused.py`. New checkpoint: bh_pickup_basic_checkpoint(unsigned long *count); scenario pickup-basic. No extra fixture headers. The fixture uses actual renderer binding/callback identities, pool/world/list/query/tick/cleanup and owned allocation-failure instrumentation; no draw execution, original code, game, UI, input or audio playback.

Source/header/tests are frozen with hashes recorded by focused-results.json. Parent owns current full real/clean build, recorder and admission evidence; focused success alone is not admission. No byte-match evidence is claimed. Requested stopping point is365 admitted functions; no next owner is assigned.
