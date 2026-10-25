# Queue reservation and state callback boundary review

2026-10-25, before source implementation. Both are engine-candidate ordinary
render queue owners, attribution unresolved; neither is CRT/compiler/EH/import
code or an independent shared tail.00477530 is complete155 bytes/50 instructions
through004775ca;00477360 is complete22/9 through00477375. All instruction bytes
were independently compared with immutable PE. Original snapshots are retained.

The nine-entry jump table at004775cc is separate data, not body code. Its actual
targets (jump-table.json) decode to header mapping1,2,3,4,5,0,6,7,8. Unsigned
JA for values above8 includes negative signed selector representations; all
these use default header0. Reservation writes only new+50, old-tail+50 and
header+04, increments count, and leaves every new payload/function word intact.
Original signed capacity comparison is represented inside the existing bounded
source queue contract(count/capacity at most65535); malformed source bindings
must be guarded, not used to fabricate original pointers.

00477360 loads command+08, calls admitted real0045b500 and reads command+50
after the call. No new raster/backend definitions are necessary. The source
wrapper may populate the existing semantic BH_DRAW_STATE selector and mode
word; that wrapper is an adapter, not another recovered original owner.
