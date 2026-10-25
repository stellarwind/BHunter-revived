# Presentation evidence entry point

See [NOTES.md](NOTES.md) for role/boundary, whole-owner dependency contract, exact branch behavior, explicit actual COM ownership and validation limits. [owners.json](owners.json) and [0045d690.tsv](0045d690.tsv) pin all180original bytes/58instructions; export_review.py verifies against the immutable PE. build_check.py produces the focused source-only actual DirectDraw/Direct3D2 target and records commands, source/dependency hashes and results in standalone-build.json. No original/gameplay execution.

Source is src/bh_present.c/.h. Runner bh_present_checkpoint(unsigned long*) is tests/present_native.c. Existing fade variants remain unchanged. Healthy actual surfaces exercise both native/software paths and real copies; conditional Restore remains statically checked rather than mocked.
