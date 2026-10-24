# Initial boundary classification

Reviewed mapped entries and call edges before decompilation (2026-10-24).
00451720 (00451720-00451744, 37 bytes), 00451750
(00451750-00451b7a, 1067 bytes), and 00452070
(00452070-0045213e, 207 bytes) are separate ordinary cdecl owners, not
CRT, imports, EH support or shared tails. The 00451750 owner calls recovered
matrix inverse/point/direction and vector normalization; 00452070 is its
pose producer called from 00477830; settings 00451720 is called by 00479200.
Role: engine_candidate, medium confidence pending complete instruction review.
No library attribution is established. They are separate from per-model
camera 00451b80. 004018a0 is reviewed only as the pose's rotation dependency.
