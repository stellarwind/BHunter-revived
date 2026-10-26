# Preserved restoration checkpoint

This branch preserves the 679-owner source checkpoint before the playable preview
and its subsequent input/rendering/platform integration fixes. It is an archival
snapshot, not a playable release and not a whole-game byte match.

Four math routines have raw byte-match evidence in the pinned VC5 SP3 comparison
context: vector cross product, point transform, direction transform, and world
translation (335 original bytes total). Their combined source, compiler/header
context, verification scripts and detailed JSON evidence are retained under
`analysis/astra-review/`. The direction transform requires its combined translation
unit context. These C++ matching variants are separate from the functional C harness.

The larger restoration is functionally reconstructed: 679 admitted original owners,
601 integration translation units, 5,529,939 real-asset and 5,023,991 asset-free
checks at the retained checkpoint. These are historical results, not a new run.
M4/playable gameplay is incomplete; numerical/native/gameplay limitations remain.

All 942 compiled source/header/test inputs are restored to their exact SHA256 values
from `META/m4/finish/validation.json`. The ledger restores the corresponding source
variants without transferring byte-match evidence to the functional implementation.
See `META/preservation-pre-preview.json` for the input manifest and snapshot origins.

Original game/media and compiler/SDK binaries are excluded. Rebuilding requires the
separately supplied pinned dependencies. No current working files were rolled back
when creating this branch; development continues independently.
