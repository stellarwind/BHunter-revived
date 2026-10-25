# Independent complete path-queue family cross-review

Read-only comparison of root's `bh_path_queue.c/.h` with complete original
00455ee0 append/setup,00477880 adaptive selector and004779e0 queued owner in
`original/`. No original code/game execution and no implementation edits by
this reviewer.

Observed point payload mappings agree: position+04/+08/+0c, width+10,
flags supplied0, texture parameters+14/+18. Setup preserves corner scratch,
increments pool count before transform and links the previous contiguous
84-byte source slot. The queued owner selects two independently masked halves
of count-1 spans, appends final point, draws through real004569b0 and reads
the last point's current next after drawing. Count0 is explicitly diagnosed as
the original negative-span/out-of-bounds domain; count1/2 can produce duplicate
projected source points and depend on the separately corrected ribbon variant.

Selector midpoint and stored radius difference match raw FST sites. One span
bypasses culling; otherwise both recursive halves inherit returned cull mask.
Adjusted depth is retained after stored-depth minus radius*.5, and the ratio
gate is strictly greater than16. Plane-mask negative returns are culled.

One concrete numeric correction was sent to root: the initial source shortcut
`radius==0 || adjusted/radius>16` accepted visible zero-radius spans but skipped
the original actual FDIV00477943 and its masked divide-by-zero sticky flag.
Root applied actual adjusted/radius division also for zero radius, preserving
positive infinity acceptance and `_SW_ZERODIVIDE`. Root's new isolated fixture
owns fresh linked validation; this note does not invent its run result.

No other finite serialized-domain mismatch was found. Source topology/capacity
guards and image-identity binding remain explicit, and no original code pointer,
fake callback or substitute ribbon renderer is supplied. Parent full integration
and native fixtures own admission/validation evidence for this family.
