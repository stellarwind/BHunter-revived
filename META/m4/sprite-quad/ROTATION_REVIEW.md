# Retained rotated Z components

Independent static confirmation2026-10-25: source double tz/rz correction is
consistent with original00457590. Rotated t.x/t.y are stored at00457796/
004577b8; r.x/r.y at004577ec/004577f2. Rotated t.z and r.z remain on the
x87 stack:004577f6 jumps directly to00457800 with no binary32 store.
Only angle-zero path004577f8/004577fc loads original t.z/r.z memory.
The retained pair is multiplied by height/width and0.5 alongside X/Y at
00457800..00457892; final scaled Z stores occur at0045787a and00457892.
Current src/bh_sprite_quad.c keeps the two rotated results in double tz/rz
and rounds only after scaling, matching the admitted PC53 profile. This does
not establish PC24/PC64 or transcendental edge equivalence. No source or
original gameplay execution was used for this confirmation.
