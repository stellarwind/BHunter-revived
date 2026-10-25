# Deferred atlas text and clear ring

Four complete source owners are compiled, linked and exercised:

| Original | Source symbol |
|---|---|
| 00478bd0–00478c44 | `bh_text_queue_text` |
| 00478ce0–00478d33 | `bh_text_queue_clear` |
| 00477ea0–00477ecf | `bh_text_queue_execute_text` |
| 00477f00–00477f19 | `bh_text_queue_execute_clear` |

All four are engine candidates with unresolved library attribution. Complete
entries, outgoing calls and returns distinguish them from CRT/compiler helpers,
imports or shared tails. Every exported instruction byte was checked against
the pinned immutable original EXE. CRT `strncpy` is reused and excluded from the
recovered count. No source instruction match is claimed.

This is a separate 256-record, one-slot-empty ring, with capacity 255. It does
not use the main nine-bucket draw command queue. Each original record is 84
bytes: a selector replacing the original function pointer, a borrowed atlas
page, signed 16-bit coordinates or a full color word, 60 text bytes, two unused
words and the original `next` at offset 80. Producers preserve unused words and
`next`. Clear preserves text as well. Text uses the real CRT's 59-byte bounded
copy and NUL padding, then forces the final text byte to zero. Coordinates keep
their low 16 bits and are sign-extended by the consumer.

Read and write positions use only their low byte. Increment preserves the high
24 bits; a producer restores the entire previous write word when its next byte
would equal the read byte. Full attempts leave record storage unchanged.

The text consumer calls the real cursor owner and complete glyph-copy owner.
The clear consumer calls the real native page clear owner. Both return the
record's `next` word after execution and retain the original ignored native
status behavior. The source FIFO adapter drains in byte-index order, ignores
those return links and increments the read byte. This adapter covers only the
ring portion of 00479090; it is not admission of that complete render callback.
Caller serialization and page lifetimes are explicit contracts.

`python META/m4/text-queue/build_evidence.py` builds `text-queue.exe` from actual
source translation units and the real engine dependency closure. It links
without fabricated definitions and passes 282 checks. `standalone-build.json`
retains exact commands, output and source hashes; the named map and source
snapshots are beside it.

Checks cover full-record write scope, short-string NUL padding, long-string
termination, signed coordinate narrowing, high-word retention, byte wrap,
255-entry capacity, rejected-full storage preservation and FIFO consumption.
The native fixture decodes an actual two-glyph PCX atlas, creates a real page,
queues clear/text and compares copied destination pixels with the actual atlas
surface. It checks both consumer return links and negative coordinate
progression. Holding the real native page lock causes a verified clear failure;
the ring still consumes the command and preserves the original cursor-reset
poststate. The fixture uses no replacement callbacks or fabricated native
vtables.

No original code, game, menu, flight or controller is executed. Main integration
and per-implementation ledger admission are recorded by the owning restoration
block after linking this checkpoint into the normal harness.
