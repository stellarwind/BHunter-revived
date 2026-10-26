# Whole segment query owner

One complete ordinary geometry owner0044d9b0,106bytes, wrapping original lazy collision metadata preparation0044b130 and actual transform/traversal0044d530. The original NULL BSP branch is a no-op before output access. Source exposes original global ownership through BHSegmentOwner and returns an explicit hit/miss/error result instead of the original void call. Complete immutable body/raw hash is recorded in owners.json.

Real source target build/segment-owner/bhunter-segment-owner-check.exe passes11checks plus omitted-module link control. Actual cache allocations, prepared-query frames and mutable face flags are shared by the source owner. Tests prove lazy hit, exact contact fraction/normal/offset, flagged-face miss with unchanged result, retained allocation identity, translated world/local pairs and busy/replacement/disposal contracts. Immutable topology is borrowed; cache first-edge topology is retained and must not be independently modified. Output and owner/BSP/transform inputs do not overlap.

PC53 finite profile, PC24 unresolved. Original global side effects become explicit stable source lifetimes. No original/game execution, input/capture or invented inverse/metadata. Parent full/clean integration remains required.

Reproduce: python META/m4/target550/b3/build_check.py.
