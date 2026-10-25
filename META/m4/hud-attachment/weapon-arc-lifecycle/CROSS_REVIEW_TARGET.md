# Independent target lifecycle review

Read-only review by the HUD agent of all255instructions in00431180 and whole
004310c0/00431130 bodies against `src/bh_weapon_target_lifecycle.c` and the target
agent's static precision probes/tests. No source edit or new target test run.

Acquisition spills all xyz deltas, groups dot `(aimY*Y+aimX*X)+aimZ*Z`, then
uses stored dot for strict nearest and cone, norm `(X²+Y²)+Z²`. Source matches
the actual raw stack operations0043123c..004312ef, including C0-only unordered
cone acceptance. Maintenance raw deltas store x/y; norm is
`(rawX*storedX+rawY*storedY)+rawZ²`, dot `(aimZ*rawZ+aimY*storedY)+aimX*storedX`.
Source matches004313e5..00431466, stores cone before both threshold tests, and
preserves unordered acceptance. Query-updated signed charge is reread before
48-based cone relaxation. There is no early stale charge copy at that point.

Both positive float timers use actual double1/30 subtraction; zero ammo clears
target/stops voice while preserving charge. Charge increment/wrap,17..47 bit2
beep branch and exact48 lock cue match. Ready and cancel preserve the precise
five zeroed words; ready uses negative raw pitch coefficient and live vector
addresses, cancel captures handle at its actual post-clear boundary. World
cache is a direct+104 alias, actual self/radius are correct, successor loads
after callback/world effects, and event3 initializes only once per full list.

No concrete source discrepancy found. This review is numerical/control/lifetime
evidence, not a claim of byte matching, original execution, constructor recovery
or universal PC24/extreme equivalence. Final hashes/integration belong to the
parent's current source snapshot.
