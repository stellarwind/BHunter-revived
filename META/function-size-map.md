# BHunter function-size map

Snapshot: 2026-10-24.

887 mapped original function entries; 530,895 bytes (518.45 KiB) summed across their mapped bodies.

Sizes are original x86 machine-code body bytes, not C source sizes or estimates of remaining effort. They exclude padding between functions. Shared/overlapping bodies, if present, contribute to each entry’s size; the sum is not the executable file size. Instructions are decoded instruction counts.

Names and body sizes come from `analysis/function-map.tsv`; ranges and roles come from `META/m1/role-catalog.json`. Source names and implementation/integration states come from `META/function-status.json`, preferring the current linked functional variant. Catalog roles and implementation states are independent; older unknown provenance may coexist with restored source. “Not tracked” means no active ledger variant, not proof that an entry is game-owned or needs restoration. CRT, libraries, imports and compiler helpers remain included and labelled. No source-built game admission or completion percentage is inferred.

## Largest 30 mapped functions

| Address | Bytes | Instructions | Function / source name | Implementation | Catalog role |
|---|---:|---:|---|---|---|
| 0x0041e850 | 11530 | 3076 | FUN_0041e850 | not tracked | unknown |
| 0x004474c0 | 9584 | 2572 | FUN_004474c0 | not tracked | game |
| 0x00424160 | 7271 | 1795 | FUN_00424160 | not tracked | unknown |
| 0x0040e420 | 6741 | 2007 | FUN_0040e420 | not tracked | unknown |
| 0x00437ae0 | 5379 | 1356 | FUN_00437ae0 | not tracked | unknown |
| 0x004723c0 | 5336 | 1686 | BH_WinMain | not tracked | game |
| 0x00418f00 | 5318 | 1447 | FUN_00418f00 | not tracked | unknown |
| 0x00453c30 | 4998 | 1656 | bh_attribute_clip_polygon | func / linked_harness | unknown |
| 0x00459510 | 4772 | 1401 | bh_model_draw_lit | func / linked_harness | unknown |
| 0x00452140 | 4539 | 1511 | bh_clip_polygon | func / linked_harness | unknown |
| 0x004744f0 | 4421 | 1107 | FUN_004744f0 | not tracked | unknown |
| 0x0044f550 | 4236 | 1239 | bh_texture_load | func / linked_harness | engine_candidate |
| 0x00458470 | 3983 | 1184 | bh_model_draw_unlit | func / linked_harness | unknown |
| 0x0041c860 | 3934 | 1162 | FUN_0041c860 | not_started / not_built | unknown |
| 0x004124d0 | 3916 | 1015 | partial_physics_numerics | wip / linked_harness | unknown |
| 0x00417f70 | 3890 | 1140 | FUN_00417f70 | not tracked | unknown |
| 0x0041a3d0 | 3826 | 1123 | FUN_0041a3d0 | not tracked | unknown |
| 0x0046f490 | 3810 | 1087 | bh_atlas_image_load | func / linked_harness | unknown |
| 0x0040d140 | 3769 | 1045 | FUN_0040d140 | not tracked | unknown |
| 0x0041d7c0 | 3749 | 1110 | FUN_0041d7c0 | not tracked | unknown |
| 0x0046e600 | 3713 | 1122 | bh_software_tables_initialize | func / linked_harness | unknown |
| 0x0042f7a0 | 3659 | 902 | FUN_0042f7a0 | not tracked | unknown |
| 0x004225f0 | 3507 | 936 | FUN_004225f0 | not tracked | unknown |
| 0x00416eb0 | 3486 | 1065 | FUN_00416eb0 | not tracked | unknown |
| 0x00416120 | 3468 | 1058 | FUN_00416120 | not tracked | unknown |
| 0x004111e0 | 3351 | 1080 | BH_ParseScriptStatements | not tracked | engine_candidate |
| 0x00414880 | 3243 | 1001 | FUN_00414880 | not tracked | unknown |
| 0x00405270 | 3200 | 827 | FUN_00405270 | not tracked | unknown |
| 0x004811a0 | 3193 | 1022 | __input | not tracked | library_verified |
| 0x00403870 | 3152 | 859 | FUN_00403870 | not tracked | unknown |

## Complete map in address order

| Address | Bytes | Instructions | Function / source name | Implementation | Catalog role |
|---|---:|---:|---|---|---|
| 0x00401000 | 408 | 162 | bh_mat34_multiply | func / linked_harness | engine_candidate |
| 0x004011a0 | 67 | 26 | bh_vec3_cross | func / linked_harness | engine_candidate |
| 0x004011f0 | 113 | 45 | bh_vec3_normalize | func / linked_harness | engine_candidate |
| 0x00401270 | 111 | 43 | bh_vec3_normalize_to | func / linked_harness | engine_candidate |
| 0x004012e0 | 116 | 46 | bh_mat34_transform_point | func / linked_harness | engine_candidate |
| 0x00401360 | 107 | 43 | bh_mat34_transform_direction | func / linked_harness | engine_candidate |
| 0x004013d0 | 140 | 60 | bh_mat34_inverse_transform_point | func / linked_harness | engine_candidate |
| 0x00401460 | 45 | 16 | bh_mat34_translate_world | func / linked_harness | engine_candidate |
| 0x00401490 | 123 | 45 | bh_mat34_translate_local | func / linked_harness | engine_candidate |
| 0x00401510 | 251 | 90 | bh_mat34_rotate_world_x | func / linked_harness | unknown |
| 0x00401610 | 196 | 71 | bh_mat34_rotate_x | func / linked_harness | unknown |
| 0x004016e0 | 247 | 89 | bh_mat34_rotate_world_y | func / linked_harness | unknown |
| 0x004017e0 | 192 | 70 | bh_mat34_rotate_y | func / linked_harness | unknown |
| 0x004018a0 | 249 | 90 | bh_mat34_rotate_world_z | func / linked_harness | unknown |
| 0x004019a0 | 194 | 71 | bh_mat34_rotate_z | func / linked_harness | unknown |
| 0x00401a70 | 179 | 70 | bh_mat34_inverse_rigid | func / linked_harness | engine_candidate |
| 0x00401b30 | 210 | 81 | bh_mat4_lu_solve | func / linked_harness | unknown |
| 0x00401c10 | 574 | 204 | bh_mat4_lu | func / linked_harness | unknown |
| 0x00401e50 | 371 | 104 | bh_mat34_inverse_general | func / linked_harness | unknown |
| 0x00401fd0 | 763 | 226 | bh_config_load | func / linked_harness | game |
| 0x004022d0 | 151 | 51 | bh_config_bootstrap | func / linked_harness | game |
| 0x00402370 | 241 | 72 | bh_config_save | func / linked_harness | unknown |
| 0x00402470 | 68 | 21 | FUN_00402470 | not tracked | unknown |
| 0x004024c0 | 254 | 62 | FUN_004024c0 | not tracked | unknown |
| 0x004025c0 | 333 | 84 | FUN_004025c0 | not tracked | unknown |
| 0x00402710 | 217 | 71 | FUN_00402710 | not tracked | unknown |
| 0x004027f0 | 58 | 18 | FUN_004027f0 | not tracked | unknown |
| 0x00402830 | 414 | 133 | FUN_00402830 | not tracked | unknown |
| 0x004029d0 | 285 | 92 | FUN_004029d0 | not tracked | unknown |
| 0x00402af0 | 455 | 120 | FUN_00402af0 | not tracked | unknown |
| 0x00402cc0 | 83 | 21 | FUN_00402cc0 | not tracked | unknown |
| 0x00402d20 | 242 | 81 | FUN_00402d20 | not tracked | unknown |
| 0x00402e20 | 37 | 10 | FUN_00402e20 | not tracked | unknown |
| 0x00402e50 | 706 | 181 | FUN_00402e50 | not tracked | unknown |
| 0x00403120 | 451 | 124 | FUN_00403120 | not tracked | unknown |
| 0x004032f0 | 749 | 193 | FUN_004032f0 | not tracked | unknown |
| 0x004035e0 | 82 | 23 | FUN_004035e0 | not tracked | unknown |
| 0x00403640 | 45 | 12 | FUN_00403640 | not tracked | unknown |
| 0x00403670 | 173 | 59 | FUN_00403670 | not tracked | unknown |
| 0x00403720 | 211 | 68 | FUN_00403720 | not tracked | unknown |
| 0x00403800 | 70 | 25 | FUN_00403800 | not tracked | unknown |
| 0x00403850 | 31 | 11 | FUN_00403850 | not tracked | unknown |
| 0x00403870 | 3152 | 859 | FUN_00403870 | not tracked | unknown |
| 0x004044c0 | 761 | 221 | FUN_004044c0 | not tracked | unknown |
| 0x004047c0 | 2730 | 716 | FUN_004047c0 | not tracked | unknown |
| 0x00405270 | 3200 | 827 | FUN_00405270 | not tracked | unknown |
| 0x00405ef0 | 987 | 260 | FUN_00405ef0 | not tracked | unknown |
| 0x00406300 | 1249 | 334 | FUN_00406300 | not tracked | unknown |
| 0x00406800 | 451 | 123 | FUN_00406800 | not tracked | unknown |
| 0x004069d0 | 308 | 72 | FUN_004069d0 | not tracked | unknown |
| 0x00406b10 | 281 | 57 | FUN_00406b10 | not tracked | unknown |
| 0x00406c30 | 277 | 75 | FUN_00406c30 | not tracked | unknown |
| 0x00406d50 | 208 | 67 | FUN_00406d50 | not tracked | unknown |
| 0x00406e20 | 144 | 35 | FUN_00406e20 | not tracked | unknown |
| 0x00406eb0 | 716 | 187 | FUN_00406eb0 | not tracked | unknown |
| 0x00407180 | 41 | 12 | FUN_00407180 | not tracked | unknown |
| 0x004071b0 | 288 | 72 | FUN_004071b0 | not tracked | unknown |
| 0x004072d0 | 222 | 81 | FUN_004072d0 | not tracked | unknown |
| 0x004073b0 | 61 | 19 | spark_refresh | func / linked_harness | unknown |
| 0x004073f0 | 281 | 72 | spark_tick | func / linked_harness | unknown |
| 0x00407510 | 340 | 74 | bh_contact_spark_create | func / linked_harness | unknown |
| 0x00407670 | 172 | 59 | quad_refresh | func / linked_harness | unknown |
| 0x00407720 | 526 | 172 | rays_refresh | func / linked_harness | unknown |
| 0x00407930 | 227 | 72 | FUN_00407930 | not tracked | unknown |
| 0x00407a20 | 168 | 52 | FUN_00407a20 | not tracked | unknown |
| 0x00407ad0 | 45 | 12 | impact_tick | func / linked_harness | unknown |
| 0x00407b00 | 54 | 18 | FUN_00407b00 | not tracked | unknown |
| 0x00407b40 | 580 | 136 | FUN_00407b40 | not tracked | unknown |
| 0x00407d90 | 406 | 137 | corridor | func / linked_harness | unknown |
| 0x00407f30 | 918 | 296 | bh_navigation_build | func / linked_harness | unknown |
| 0x004082d0 | 256 | 80 | world_bounds | func / linked_harness | unknown |
| 0x004083d0 | 193 | 70 | model_bounds | func / linked_harness | unknown |
| 0x004084a0 | 1177 | 357 | bh_world_geometry_build | func / linked_harness | unknown |
| 0x00408940 | 455 | 140 | bh_portals_match | func / linked_harness | unknown |
| 0x00408b10 | 1557 | 463 | FUN_00408b10 | not tracked | unknown |
| 0x00409130 | 1012 | 296 | bh_scene_owner_step | func / linked_harness | unknown |
| 0x00409530 | 2534 | 845 | bh_entity_schedule_clip_portal | func / linked_harness | unknown |
| 0x00409f20 | 30 | 7 | bh_entity_schedule_tick | func / linked_harness | unknown |
| 0x00409f40 | 2367 | 663 | bh_entity_schedule | func / linked_harness | unknown |
| 0x0040a8c0 | 794 | 263 | bh_world_locate | func / linked_harness | unknown |
| 0x0040abe0 | 795 | 256 | collect (owned static DFS) | func / linked_harness | unknown |
| 0x0040af20 | 251 | 84 | actors (owned strict actor collector) | func / linked_harness | unknown |
| 0x0040b020 | 729 | 232 | bh_world_segment_actor_trace | func / linked_harness | unknown |
| 0x0040b300 | 263 | 109 | bh_world_query_run | func / linked_harness | unknown |
| 0x0040b410 | 1582 | 483 | bh_entity_collision_dispatch | func / linked_harness | unknown |
| 0x0040ba40 | 2625 | 809 | bh_world_segment_room_trace | func / linked_harness | unknown |
| 0x0040c4a0 | 448 | 173 | bh_world_segment_run | func / linked_harness | unknown |
| 0x0040c660 | 845 | 234 | FUN_0040c660 | not tracked | unknown |
| 0x0040c9b0 | 1610 | 464 | FUN_0040c9b0 | not tracked | unknown |
| 0x0040d000 | 320 | 69 | FUN_0040d000 | not tracked | unknown |
| 0x0040d140 | 3769 | 1045 | FUN_0040d140 | not tracked | unknown |
| 0x0040e050 | 23 | 7 | FUN_0040e050 | not tracked | unknown |
| 0x0040e070 | 393 | 131 | FUN_0040e070 | not tracked | unknown |
| 0x0040e200 | 248 | 75 | FUN_0040e200 | not tracked | unknown |
| 0x0040e300 | 277 | 101 | FUN_0040e300 | not tracked | unknown |
| 0x0040e420 | 6741 | 2007 | FUN_0040e420 | not tracked | unknown |
| 0x00410020 | 90 | 27 | FUN_00410020 | not tracked | unknown |
| 0x00410080 | 44 | 10 | FUN_00410080 | not tracked | unknown |
| 0x004100b0 | 25 | 7 | FUN_004100b0 | not tracked | unknown |
| 0x004100d0 | 1216 | 367 | level loader (binary reader subset only) | wip / linked_harness | engine_candidate |
| 0x00410590 | 750 | 204 | FUN_00410590 | not tracked | unknown |
| 0x00410880 | 318 | 128 | classify | func / linked_harness | unknown |
| 0x004109c0 | 476 | 187 | FUN_004109c0 | not tracked | unknown |
| 0x00410ba0 | 484 | 189 | FUN_00410ba0 | not tracked | unknown |
| 0x00410d90 | 454 | 153 | FUN_00410d90 | not tracked | unknown |
| 0x00410f60 | 276 | 99 | FUN_00410f60 | not tracked | unknown |
| 0x00411080 | 350 | 127 | FUN_00411080 | not tracked | unknown |
| 0x004111e0 | 3351 | 1080 | BH_ParseScriptStatements | not tracked | engine_candidate |
| 0x00411f00 | 654 | 189 | block registry (owned data subset only) | wip / linked_harness | engine_candidate |
| 0x00412190 | 820 | 265 | FUN_00412190 | not tracked | unknown |
| 0x004124d0 | 3916 | 1015 | partial_physics_numerics | wip / linked_harness | unknown |
| 0x00413420 | 1383 | 396 | bh_actor_basis_build_groups | func / linked_harness | unknown |
| 0x00413990 | 738 | 239 | FUN_00413990 | not tracked | unknown |
| 0x00413c80 | 758 | 246 | FUN_00413c80 | not tracked | unknown |
| 0x00413f80 | 1897 | 466 | bh_actor_basis_acquire | func / linked_harness | unknown |
| 0x004146f0 | 395 | 130 | FUN_004146f0 | not tracked | unknown |
| 0x00414880 | 3243 | 1001 | FUN_00414880 | not tracked | unknown |
| 0x00415530 | 3042 | 937 | FUN_00415530 | not tracked | unknown |
| 0x00416120 | 3468 | 1058 | FUN_00416120 | not tracked | unknown |
| 0x00416eb0 | 3486 | 1065 | FUN_00416eb0 | not tracked | unknown |
| 0x00417c50 | 735 | 222 | FUN_00417c50 | not tracked | unknown |
| 0x00417f30 | 61 | 21 | FUN_00417f30 | not tracked | unknown |
| 0x00417f70 | 3890 | 1140 | FUN_00417f70 | not tracked | unknown |
| 0x00418f00 | 5318 | 1447 | FUN_00418f00 | not tracked | unknown |
| 0x0041a3d0 | 3826 | 1123 | FUN_0041a3d0 | not tracked | unknown |
| 0x0041b2d0 | 893 | 233 | FUN_0041b2d0 | not tracked | unknown |
| 0x0041b650 | 1434 | 427 | FUN_0041b650 | not tracked | unknown |
| 0x0041bbf0 | 1029 | 317 | FUN_0041bbf0 | not tracked | unknown |
| 0x0041c020 | 2105 | 604 | bh_scene_background_step | func / linked_harness | unknown |
| 0x0041c860 | 3934 | 1162 | FUN_0041c860 | not_started / not_built | unknown |
| 0x0041d7c0 | 3749 | 1110 | FUN_0041d7c0 | not tracked | unknown |
| 0x0041e6b0 | 414 | 138 | bh_camera_correct | func / linked_harness | unknown |
| 0x0041e850 | 11530 | 3076 | FUN_0041e850 | not tracked | unknown |
| 0x00421560 | 94 | 31 | FUN_00421560 | not tracked | unknown |
| 0x004215c0 | 70 | 25 | bh_player_input_broadcast | func / linked_harness | unknown |
| 0x00421610 | 304 | 107 | FUN_00421610 | not tracked | unknown |
| 0x00421740 | 1147 | 407 | bh_player_input_setup | func / linked_harness | unknown |
| 0x00421bc0 | 346 | 88 | FUN_00421bc0 | not tracked | unknown |
| 0x00421d20 | 1204 | 298 | FUN_00421d20 | not tracked | unknown |
| 0x004221e0 | 910 | 234 | bh_player_camera_reset | func / linked_harness | unknown |
| 0x00422570 | 118 | 27 | FUN_00422570 | not tracked | unknown |
| 0x004225f0 | 3507 | 936 | FUN_004225f0 | not tracked | unknown |
| 0x00423400 | 193 | 79 | FUN_00423400 | not tracked | unknown |
| 0x004234d0 | 389 | 107 | bh_packet_write_position | func / linked_harness | engine_candidate |
| 0x00423660 | 175 | 52 | bh_packet_write_vector | func / linked_harness | engine_candidate |
| 0x00423710 | 129 | 48 | bh_network_packet_send | func / linked_harness | engine_candidate |
| 0x004237a0 | 157 | 63 | bh_packet_receive | func / linked_harness | engine_candidate |
| 0x00423840 | 349 | 97 | bh_position_read | func / linked_harness | unknown |
| 0x004239a0 | 107 | 31 | bh_packet_read_actor | func / linked_harness | engine_candidate |
| 0x00423a10 | 107 | 32 | FUN_00423a10 | not tracked | unknown |
| 0x00423a80 | 300 | 100 | FUN_00423a80 | not tracked | unknown |
| 0x00423bb0 | 195 | 64 | FUN_00423bb0 | not tracked | unknown |
| 0x00423c80 | 482 | 130 | FUN_00423c80 | not tracked | unknown |
| 0x00423e70 | 697 | 204 | FUN_00423e70 | not tracked | unknown |
| 0x00424160 | 7271 | 1795 | FUN_00424160 | not tracked | unknown |
| 0x00425df0 | 1335 | 379 | FUN_00425df0 | not tracked | unknown |
| 0x00426330 | 2592 | 751 | FUN_00426330 | not tracked | unknown |
| 0x00426d50 | 349 | 121 | FUN_00426d50 | not tracked | unknown |
| 0x00426eb0 | 216 | 73 | FUN_00426eb0 | not tracked | unknown |
| 0x00426f90 | 484 | 158 | FUN_00426f90 | not tracked | unknown |
| 0x00427180 | 447 | 143 | FUN_00427180 | not tracked | unknown |
| 0x00427340 | 639 | 187 | FUN_00427340 | not tracked | unknown |
| 0x004275c0 | 321 | 102 | FUN_004275c0 | not tracked | unknown |
| 0x00427710 | 527 | 157 | FUN_00427710 | not tracked | unknown |
| 0x00427920 | 1036 | 274 | FUN_00427920 | not tracked | unknown |
| 0x00427d30 | 2539 | 703 | FUN_00427d30 | not tracked | unknown |
| 0x00428750 | 1586 | 449 | FUN_00428750 | not tracked | unknown |
| 0x00428dc0 | 391 | 127 | FUN_00428dc0 | not tracked | unknown |
| 0x00428f50 | 1286 | 360 | FUN_00428f50 | not_started / not_built | unknown |
| 0x00429480 | 529 | 160 | bh_route_plan | func / linked_harness | unknown |
| 0x004296a0 | 2307 | 701 | bh_camera_route_advance | func / linked_harness | unknown |
| 0x00429fb0 | 2556 | 757 | bh_flyby_camera_step | func / linked_harness | unknown |
| 0x0042a9b0 | 34 | 7 | FUN_0042a9b0 | not tracked | unknown |
| 0x0042a9e0 | 494 | 137 | FUN_0042a9e0 | not tracked | unknown |
| 0x0042abd0 | 1805 | 502 | FUN_0042abd0 | not tracked | unknown |
| 0x0042b2e0 | 2955 | 797 | FUN_0042b2e0 | not tracked | unknown |
| 0x0042be70 | 263 | 85 | FUN_0042be70 | not tracked | unknown |
| 0x0042bf80 | 890 | 268 | FUN_0042bf80 | not tracked | unknown |
| 0x0042c300 | 447 | 132 | FUN_0042c300 | not tracked | unknown |
| 0x0042c4c0 | 781 | 241 | FUN_0042c4c0 | not tracked | unknown |
| 0x0042c7d0 | 573 | 202 | FUN_0042c7d0 | not tracked | unknown |
| 0x0042ca10 | 20 | 4 | FUN_0042ca10 | not tracked | unknown |
| 0x0042ca30 | 97 | 29 | FUN_0042ca30 | not tracked | unknown |
| 0x0042caa0 | 51 | 10 | FUN_0042caa0 | not tracked | unknown |
| 0x0042cae0 | 47 | 12 | FUN_0042cae0 | not tracked | unknown |
| 0x0042cb10 | 47 | 12 | FUN_0042cb10 | not tracked | unknown |
| 0x0042cb40 | 98 | 27 | FUN_0042cb40 | not tracked | unknown |
| 0x0042cbb0 | 779 | 217 | FUN_0042cbb0 | not tracked | unknown |
| 0x0042cec0 | 52 | 14 | FUN_0042cec0 | not tracked | unknown |
| 0x0042cf00 | 164 | 45 | FUN_0042cf00 | not tracked | unknown |
| 0x0042cfb0 | 184 | 49 | FUN_0042cfb0 | not tracked | unknown |
| 0x0042d070 | 82 | 15 | FUN_0042d070 | not tracked | unknown |
| 0x0042d0d0 | 217 | 77 | FUN_0042d0d0 | not tracked | unknown |
| 0x0042d1b0 | 211 | 71 | FUN_0042d1b0 | not tracked | unknown |
| 0x0042d290 | 227 | 69 | FUN_0042d290 | not tracked | unknown |
| 0x0042d380 | 409 | 133 | FUN_0042d380 | not tracked | unknown |
| 0x0042d550 | 163 | 49 | FUN_0042d550 | not tracked | unknown |
| 0x0042d620 | 163 | 49 | FUN_0042d620 | not tracked | unknown |
| 0x0042d6f0 | 504 | 165 | FUN_0042d6f0 | not tracked | unknown |
| 0x0042d920 | 110 | 40 | FUN_0042d920 | not tracked | unknown |
| 0x0042d990 | 171 | 64 | FUN_0042d990 | not tracked | unknown |
| 0x0042da40 | 88 | 34 | FUN_0042da40 | not tracked | unknown |
| 0x0042daa0 | 163 | 47 | FUN_0042daa0 | not tracked | unknown |
| 0x0042db60 | 767 | 208 | FUN_0042db60 | not tracked | unknown |
| 0x0042de70 | 288 | 101 | FUN_0042de70 | not tracked | unknown |
| 0x0042df90 | 448 | 149 | FUN_0042df90 | not tracked | unknown |
| 0x0042e150 | 418 | 127 | FUN_0042e150 | not tracked | unknown |
| 0x0042e300 | 94 | 34 | FUN_0042e300 | not tracked | unknown |
| 0x0042e360 | 147 | 46 | FUN_0042e360 | not tracked | unknown |
| 0x0042e410 | 270 | 88 | FUN_0042e410 | not tracked | unknown |
| 0x0042e540 | 205 | 61 | FUN_0042e540 | not tracked | unknown |
| 0x0042e640 | 66 | 14 | FUN_0042e640 | not tracked | unknown |
| 0x0042e690 | 205 | 61 | FUN_0042e690 | not tracked | unknown |
| 0x0042e790 | 66 | 14 | FUN_0042e790 | not tracked | unknown |
| 0x0042e7e0 | 225 | 66 | FUN_0042e7e0 | not tracked | unknown |
| 0x0042e8f0 | 220 | 70 | FUN_0042e8f0 | not tracked | unknown |
| 0x0042e9e0 | 130 | 29 | FUN_0042e9e0 | not tracked | unknown |
| 0x0042ea70 | 110 | 19 | FUN_0042ea70 | not tracked | unknown |
| 0x0042eae0 | 77 | 33 | FUN_0042eae0 | not tracked | unknown |
| 0x0042eb30 | 778 | 256 | FUN_0042eb30 | not tracked | unknown |
| 0x0042ee50 | 77 | 33 | FUN_0042ee50 | not tracked | unknown |
| 0x0042eea0 | 530 | 169 | FUN_0042eea0 | not tracked | unknown |
| 0x0042f0c0 | 412 | 98 | FUN_0042f0c0 | not tracked | unknown |
| 0x0042f280 | 41 | 8 | FUN_0042f280 | not tracked | unknown |
| 0x0042f2b0 | 151 | 49 | FUN_0042f2b0 | not tracked | unknown |
| 0x0042f350 | 5 | 1 | thunk_FUN_00402370 | not tracked | unknown |
| 0x0042f360 | 41 | 9 | FUN_0042f360 | not tracked | unknown |
| 0x0042f390 | 98 | 24 | FUN_0042f390 | not tracked | unknown |
| 0x0042f400 | 804 | 205 | FUN_0042f400 | not tracked | unknown |
| 0x0042f730 | 99 | 26 | FUN_0042f730 | not tracked | unknown |
| 0x0042f7a0 | 3659 | 902 | FUN_0042f7a0 | not tracked | unknown |
| 0x004305f0 | 952 | 274 | FUN_004305f0 | not tracked | unknown |
| 0x004309b0 | 22 | 7 | FUN_004309b0 | not tracked | unknown |
| 0x004309d0 | 94 | 24 | FUN_004309d0 | not tracked | unknown |
| 0x00430a30 | 848 | 241 | FUN_00430a30 | not tracked | unknown |
| 0x00430d80 | 707 | 214 | FUN_00430d80 | not tracked | unknown |
| 0x00431050 | 108 | 26 | FUN_00431050 | not tracked | unknown |
| 0x004310c0 | 108 | 26 | FUN_004310c0 | not tracked | unknown |
| 0x00431130 | 69 | 17 | FUN_00431130 | not tracked | unknown |
| 0x00431180 | 871 | 255 | FUN_00431180 | not tracked | unknown |
| 0x004314f0 | 799 | 235 | FUN_004314f0 | not tracked | unknown |
| 0x00431810 | 1426 | 410 | FUN_00431810 | not tracked | unknown |
| 0x00431db0 | 330 | 93 | FUN_00431db0 | not tracked | unknown |
| 0x00431f00 | 332 | 98 | FUN_00431f00 | not tracked | unknown |
| 0x00432050 | 794 | 233 | FUN_00432050 | not tracked | unknown |
| 0x00432370 | 303 | 101 | FUN_00432370 | not tracked | unknown |
| 0x004324a0 | 106 | 29 | FUN_004324a0 | not tracked | unknown |
| 0x00432510 | 65 | 18 | FUN_00432510 | not tracked | unknown |
| 0x00432560 | 104 | 25 | FUN_00432560 | not tracked | unknown |
| 0x004325d0 | 454 | 135 | FUN_004325d0 | not tracked | unknown |
| 0x004327a0 | 1201 | 325 | FUN_004327a0 | not tracked | unknown |
| 0x00432c60 | 900 | 280 | FUN_00432c60 | not tracked | unknown |
| 0x00432ff0 | 375 | 104 | FUN_00432ff0 | not tracked | unknown |
| 0x00433170 | 79 | 18 | FUN_00433170 | not tracked | unknown |
| 0x004331c0 | 1626 | 454 | FUN_004331c0 | not tracked | unknown |
| 0x00433820 | 42 | 10 | FUN_00433820 | not tracked | unknown |
| 0x00433850 | 678 | 192 | FUN_00433850 | not tracked | unknown |
| 0x00433b00 | 42 | 10 | FUN_00433b00 | not tracked | unknown |
| 0x00433b30 | 643 | 183 | FUN_00433b30 | not tracked | unknown |
| 0x00433dc0 | 138 | 35 | FUN_00433dc0 | not tracked | unknown |
| 0x00433e50 | 69 | 19 | FUN_00433e50 | not tracked | unknown |
| 0x00433ea0 | 203 | 46 | FUN_00433ea0 | not tracked | unknown |
| 0x00433f70 | 165 | 46 | bh_portal_center | func / linked_harness | unknown |
| 0x00434040 | 147 | 44 | bh_portal_normal | func / linked_harness | unknown |
| 0x004340f0 | 169 | 53 | FUN_004340f0 | not tracked | unknown |
| 0x004341a0 | 543 | 185 | bh_camera_curve | func / linked_harness | unknown |
| 0x004343c0 | 298 | 119 | FUN_004343c0 | not tracked | unknown |
| 0x004344f0 | 344 | 116 | FUN_004344f0 | not tracked | unknown |
| 0x00434650 | 440 | 138 | FUN_00434650 | not tracked | unknown |
| 0x00434810 | 262 | 85 | FUN_00434810 | not tracked | unknown |
| 0x00434920 | 1561 | 433 | FUN_00434920 | not tracked | unknown |
| 0x00434f40 | 1600 | 411 | FUN_00434f40 | not tracked | unknown |
| 0x004355c0 | 350 | 109 | FUN_004355c0 | not tracked | unknown |
| 0x00435720 | 102 | 28 | FUN_00435720 | not tracked | unknown |
| 0x00435790 | 58 | 17 | FUN_00435790 | not tracked | unknown |
| 0x004357d0 | 1709 | 449 | FUN_004357d0 | not tracked | unknown |
| 0x00435e80 | 1312 | 382 | FUN_00435e80 | not tracked | unknown |
| 0x004363f0 | 633 | 211 | FUN_004363f0 | not tracked | unknown |
| 0x00436670 | 85 | 31 | FUN_00436670 | not tracked | unknown |
| 0x004366d0 | 1527 | 411 | FUN_004366d0 | not tracked | unknown |
| 0x00436cd0 | 708 | 191 | FUN_00436cd0 | not tracked | unknown |
| 0x00436fa0 | 1295 | 360 | FUN_00436fa0 | not tracked | unknown |
| 0x004374b0 | 242 | 66 | FUN_004374b0 | not tracked | unknown |
| 0x004375b0 | 291 | 82 | FUN_004375b0 | not tracked | unknown |
| 0x00437770 | 98 | 28 | FUN_00437770 | not tracked | unknown |
| 0x00437a30 | 76 | 22 | FUN_00437a30 | not tracked | unknown |
| 0x00437a80 | 42 | 8 | FUN_00437a80 | not tracked | unknown |
| 0x00437ab0 | 42 | 8 | FUN_00437ab0 | not tracked | unknown |
| 0x00437ae0 | 5379 | 1356 | FUN_00437ae0 | not tracked | unknown |
| 0x00438ff0 | 2015 | 535 | FUN_00438ff0 | not tracked | unknown |
| 0x004397e0 | 212 | 56 | FUN_004397e0 | not tracked | unknown |
| 0x004398c0 | 80 | 24 | FUN_004398c0 | not tracked | unknown |
| 0x00439910 | 2231 | 578 | FUN_00439910 | not tracked | unknown |
| 0x0043a1d0 | 1793 | 512 | FUN_0043a1d0 | not tracked | unknown |
| 0x0043a920 | 2443 | 548 | FUN_0043a920 | not tracked | unknown |
| 0x0043b2d0 | 2483 | 564 | FUN_0043b2d0 | not tracked | unknown |
| 0x0043bca0 | 533 | 161 | FUN_0043bca0 | not tracked | unknown |
| 0x0043bec0 | 832 | 171 | FUN_0043bec0 | not tracked | unknown |
| 0x0043c3f0 | 1215 | 374 | FUN_0043c3f0 | not tracked | unknown |
| 0x0043c8b0 | 58 | 21 | bh_debris_remove | func / linked_harness | engine_candidate |
| 0x0043c8f0 | 987 | 341 | bh_debris_receive | func / linked_harness | engine_candidate |
| 0x0043cd00 | 1404 | 431 | FUN_0043cd00 | not tracked | unknown |
| 0x0043d2c0 | 1270 | 349 | FUN_0043d2c0 | not tracked | unknown |
| 0x0043d7c0 | 1523 | 411 | FUN_0043d7c0 | not tracked | unknown |
| 0x0043ddc0 | 2139 | 622 | FUN_0043ddc0 | not tracked | unknown |
| 0x0043e620 | 231 | 77 | FUN_0043e620 | not tracked | unknown |
| 0x0043e710 | 293 | 94 | FUN_0043e710 | not tracked | unknown |
| 0x0043e840 | 589 | 182 | FUN_0043e840 | not tracked | unknown |
| 0x0043ea90 | 2969 | 765 | FUN_0043ea90 | not tracked | unknown |
| 0x0043f630 | 2549 | 692 | FUN_0043f630 | not tracked | unknown |
| 0x00440030 | 31 | 11 | FUN_00440030 | not tracked | unknown |
| 0x00440050 | 1973 | 513 | FUN_00440050 | not tracked | unknown |
| 0x00440810 | 205 | 60 | FUN_00440810 | not tracked | unknown |
| 0x004408e0 | 235 | 65 | FUN_004408e0 | not tracked | unknown |
| 0x004409d0 | 249 | 64 | FUN_004409d0 | not tracked | unknown |
| 0x00440ad0 | 360 | 98 | FUN_00440ad0 | not tracked | unknown |
| 0x00440c40 | 1711 | 511 | bh_pickup_label | func / linked_harness | unknown |
| 0x004412f0 | 179 | 56 | bh_pickup_draw | func / linked_harness | unknown |
| 0x004413b0 | 276 | 81 | bh_entity_resource_draw_special | func / linked_harness | unknown |
| 0x004414d0 | 151 | 40 | bh_pickup_cleanup | func / linked_harness | unknown |
| 0x00441570 | 48 | 11 | bh_pickup_query | func / linked_harness | unknown |
| 0x004415a0 | 1111 | 309 | bh_pickup_tick | func / linked_harness | unknown |
| 0x00441a00 | 384 | 96 | bh_pickup_spawn | func / linked_harness | unknown |
| 0x00441b80 | 340 | 77 | FUN_00441b80 | not tracked | unknown |
| 0x00441ce0 | 150 | 56 | FUN_00441ce0 | not tracked | unknown |
| 0x00441d80 | 1454 | 400 | FUN_00441d80 | not tracked | unknown |
| 0x00442330 | 37 | 10 | FUN_00442330 | not tracked | unknown |
| 0x00442360 | 223 | 57 | FUN_00442360 | not tracked | unknown |
| 0x00442440 | 1130 | 329 | FUN_00442440 | not tracked | unknown |
| 0x004428b0 | 150 | 39 | FUN_004428b0 | not tracked | unknown |
| 0x00442950 | 224 | 58 | FUN_00442950 | not tracked | unknown |
| 0x00442a30 | 292 | 103 | FUN_00442a30 | not tracked | unknown |
| 0x00442b60 | 138 | 30 | FUN_00442b60 | not tracked | unknown |
| 0x00442bf0 | 212 | 47 | FUN_00442bf0 | not tracked | unknown |
| 0x00442cd0 | 23 | 8 | FUN_00442cd0 | not tracked | unknown |
| 0x00442cf0 | 233 | 63 | active_tick (owned timer tick) | func / linked_harness | unknown |
| 0x00442de0 | 193 | 53 | always_tick (owned timer tick) | func / linked_harness | unknown |
| 0x00442eb0 | 14 | 5 | delete_tick | func / linked_harness | unknown |
| 0x00442ec0 | 33 | 11 | FUN_00442ec0 | not tracked | unknown |
| 0x00442ef0 | 72 | 25 | running_query | func / linked_harness | unknown |
| 0x00442f40 | 202 | 63 | FUN_00442f40 | not tracked | unknown |
| 0x00443010 | 293 | 97 | template_timer (owned activate, activity-gated child) | func / linked_harness | unknown |
| 0x00443140 | 293 | 97 | template_timer2 (owned activate, unconditional child) | func / linked_harness | unknown |
| 0x00443270 | 221 | 80 | FUN_00443270 | not tracked | unknown |
| 0x00443350 | 62 | 18 | FUN_00443350 | not tracked | unknown |
| 0x00443390 | 287 | 79 | FUN_00443390 | not tracked | unknown |
| 0x004434b0 | 397 | 105 | FUN_004434b0 | not tracked | unknown |
| 0x00443640 | 161 | 53 | FUN_00443640 | not tracked | unknown |
| 0x004436f0 | 270 | 79 | FUN_004436f0 | not tracked | unknown |
| 0x00443800 | 414 | 106 | FUN_00443800 | not tracked | unknown |
| 0x004439a0 | 273 | 76 | FUN_004439a0 | not tracked | unknown |
| 0x00443ac0 | 628 | 180 | FUN_00443ac0 | not tracked | unknown |
| 0x00443d40 | 310 | 80 | FUN_00443d40 | not tracked | unknown |
| 0x00443e80 | 120 | 36 | FUN_00443e80 | not tracked | unknown |
| 0x00443f00 | 72 | 25 | FUN_00443f00 | not tracked | unknown |
| 0x00443f50 | 57 | 20 | FUN_00443f50 | not tracked | unknown |
| 0x00443f90 | 92 | 30 | FUN_00443f90 | not tracked | unknown |
| 0x004457c0 | 221 | 56 | FUN_004457c0 | not tracked | unknown |
| 0x004458a0 | 101 | 38 | FUN_004458a0 | not tracked | unknown |
| 0x00445910 | 227 | 88 | FUN_00445910 | not tracked | unknown |
| 0x00445a00 | 1087 | 221 | FUN_00445a00 | not tracked | unknown |
| 0x00445e70 | 741 | 257 | FUN_00445e70 | not tracked | unknown |
| 0x00446160 | 395 | 125 | FUN_00446160 | not tracked | unknown |
| 0x004462f0 | 2073 | 574 | FUN_004462f0 | not tracked | unknown |
| 0x00446b10 | 746 | 219 | FUN_00446b10 | not tracked | unknown |
| 0x00446e00 | 660 | 207 | FUN_00446e00 | not tracked | unknown |
| 0x004470c0 | 1014 | 284 | FUN_004470c0 | not tracked | unknown |
| 0x004474c0 | 9584 | 2572 | FUN_004474c0 | not tracked | game |
| 0x00449a50 | 238 | 75 | BH_InitCDAudio | not tracked | engine_candidate |
| 0x00449b40 | 77 | 21 | FUN_00449b40 | not tracked | unknown |
| 0x00449b90 | 132 | 39 | FUN_00449b90 | not tracked | unknown |
| 0x00449c20 | 150 | 45 | FUN_00449c20 | not tracked | unknown |
| 0x00449cc0 | 36 | 10 | FUN_00449cc0 | not tracked | unknown |
| 0x00449cf0 | 71 | 19 | FUN_00449cf0 | not tracked | unknown |
| 0x00449d40 | 88 | 28 | FUN_00449d40 | not tracked | unknown |
| 0x00449da0 | 71 | 19 | FUN_00449da0 | not tracked | unknown |
| 0x00449df0 | 88 | 28 | FUN_00449df0 | not tracked | unknown |
| 0x00449e50 | 769 | 267 | FUN_00449e50 | not tracked | unknown |
| 0x0044a160 | 178 | 42 | FUN_0044a160 | not tracked | game |
| 0x0044a220 | 340 | 124 | FUN_0044a220 | not tracked | unknown |
| 0x0044a380 | 411 | 145 | FUN_0044a380 | not tracked | unknown |
| 0x0044a520 | 512 | 171 | FUN_0044a520 | not tracked | unknown |
| 0x0044a720 | 370 | 131 | FUN_0044a720 | not tracked | unknown |
| 0x0044a8a0 | 532 | 177 | FUN_0044a8a0 | not tracked | unknown |
| 0x0044aac0 | 1487 | 444 | FUN_0044aac0 | not tracked | unknown |
| 0x0044b0a0 | 140 | 53 | bh_collision_resource_path | func / linked_harness | unknown |
| 0x0044b130 | 423 | 152 | bh_collision_cache_prepare | func / linked_harness | unknown |
| 0x0044b2e0 | 1195 | 464 | bh_collision_model_load | func / linked_harness | unknown |
| 0x0044b7a0 | 2646 | 711 | FUN_0044b7a0 | not tracked | unknown |
| 0x0044c200 | 2591 | 759 | bh_collision_query_node | func / linked_harness | unknown |
| 0x0044cc20 | 1738 | 536 | bh_collision_face_contact | func / linked_harness | unknown |
| 0x0044d2f0 | 391 | 147 | bh_collision_query_run | func / linked_harness | unknown |
| 0x0044d480 | 120 | 35 | bh_physics_overlap | func / linked_harness | unknown |
| 0x0044d500 | 34 | 9 | bh_collision_cached_contact | func / linked_harness | unknown |
| 0x0044d530 | 1147 | 375 | bh_segment_query_run (owned traversal/faces/polygon) | func / linked_harness | unknown |
| 0x0044d9b0 | 106 | 31 | FUN_0044d9b0 | not tracked | unknown |
| 0x0044da20 | 1595 | 458 | bh_sweep_run | func / linked_harness | unknown |
| 0x0044e060 | 430 | 138 | bh_geometry_each_face | func / linked_harness | unknown |
| 0x0044e210 | 19 | 6 | bh_geometry_set_flags | func / linked_harness | unknown |
| 0x0044e230 | 15 | 5 | bh_geometry_get_flags | func / linked_harness | unknown |
| 0x0044e240 | 87 | 33 | bh_geometry_vertex | func / linked_harness | unknown |
| 0x0044e2a0 | 333 | 123 | bh_geometry_face | func / linked_harness | unknown |
| 0x0044e3f0 | 146 | 54 | bh_geometry_radius | func / linked_harness | unknown |
| 0x0044e490 | 116 | 45 | bh_texture_format_callback | func / linked_harness | unknown |
| 0x0044e510 | 108 | 42 | bh_texture_formats | func / linked_harness | unknown |
| 0x0044e580 | 105 | 47 | bh_texture_request_format | func / linked_harness | unknown |
| 0x0044e5f0 | 391 | 134 | bh_texture_slot_create | func / linked_harness | unknown |
| 0x0044e780 | 245 | 92 | bh_texture_preallocate | func / linked_harness | unknown |
| 0x0044e880 | 140 | 53 | bh_texture_set_path | func / linked_harness | unknown |
| 0x0044e910 | 147 | 61 | bh_model_path_set | func / linked_harness | unknown |
| 0x0044e9b0 | 425 | 129 | bh_texture_tables_init | func / linked_harness | unknown |
| 0x0044eb60 | 1079 | 305 | bh_texture_palette_tables | func / linked_harness | unknown |
| 0x0044efa0 | 246 | 66 | bh_pcx_reader_next | func / linked_harness | unknown |
| 0x0044f0a0 | 269 | 70 | bh_pcx_reader_open | func / linked_harness | unknown |
| 0x0044f1b0 | 15 | 5 | FUN_0044f1b0 | not tracked | unknown |
| 0x0044f1c0 | 814 | 271 | bh_pcx_palette | func / linked_harness | unknown |
| 0x0044f4f0 | 20 | 5 | bh_texture_policy_set | func / linked_harness | unknown |
| 0x0044f510 | 54 | 16 | bh_texture_image_set_words | func / linked_harness | unknown |
| 0x0044f550 | 4236 | 1239 | bh_texture_load | func / linked_harness | engine_candidate |
| 0x004505f0 | 451 | 183 | bh_texture_resident | func / linked_harness | unknown |
| 0x004507c0 | 14 | 5 | bh_texture_image_load | func / linked_harness | unknown |
| 0x004507d0 | 199 | 76 | bh_texture_draw_vertex | func / linked_harness | unknown |
| 0x004508a0 | 135 | 56 | bh_texture_draw_flush | func / linked_harness | unknown |
| 0x00450980 | 827 | 257 | bh_light_prepare | func / linked_harness | unknown |
| 0x00450cd0 | 1111 | 356 | bh_light_evaluate_prepared | func / linked_harness | unknown |
| 0x00451140 | 1108 | 356 | bh_light_evaluate_world | func / linked_harness | unknown |
| 0x004515b0 | 168 | 46 | bh_view_clip_rect | func / linked_harness | unknown |
| 0x00451660 | 178 | 44 | bh_view_reset_viewport | func / linked_harness | unknown |
| 0x00451720 | 37 | 8 | bh_main_camera_settings | func / linked_harness | unknown |
| 0x00451750 | 1067 | 277 | bh_main_camera_build | func / linked_harness | unknown |
| 0x00451b80 | 1264 | 331 | bh_view_camera_build | func / linked_harness | unknown |
| 0x00452070 | 207 | 60 | bh_main_camera_pose | func / linked_harness | unknown |
| 0x00452140 | 4539 | 1511 | bh_clip_polygon | func / linked_harness | unknown |
| 0x00453300 | 2347 | 777 | bh_screen_clip_polygon | func / linked_harness | unknown |
| 0x00453c30 | 4998 | 1656 | bh_attribute_clip_polygon | func / linked_harness | unknown |
| 0x00454fc0 | 1103 | 264 | bh_pickup_rect_draw | wip / linked_harness | unknown |
| 0x00455410 | 1059 | 251 | FUN_00455410 | not tracked | unknown |
| 0x00455840 | 129 | 46 | FUN_00455840 | not tracked | unknown |
| 0x004558d0 | 139 | 49 | bh_pickup_project | func / linked_harness | unknown |
| 0x00455960 | 1234 | 291 | bh_sprite_billboard_draw | func / linked_harness | unknown |
| 0x00455e40 | 147 | 44 | FUN_00455e40 | not tracked | unknown |
| 0x00455ee0 | 136 | 41 | FUN_00455ee0 | not tracked | unknown |
| 0x00455f70 | 2620 | 677 | bh_sprite_oriented_ribbon | func / linked_harness | unknown |
| 0x004569b0 | 895 | 305 | bh_sprite_oriented_draw | func / linked_harness | unknown |
| 0x00456d30 | 2141 | 547 | FUN_00456d30 | not tracked | unknown |
| 0x00457590 | 2181 | 556 | FUN_00457590 | not tracked | unknown |
| 0x00457e20 | 800 | 338 | bh_model_read | func / linked_harness | unknown |
| 0x00458140 | 407 | 146 | bh_model_load | func / linked_harness | unknown |
| 0x004582f0 | 12 | 4 | bh_texture_image_set_flags | func / linked_harness | unknown |
| 0x00458300 | 10 | 3 | bh_draw_controls_texture_mask | func / linked_harness | unknown |
| 0x00458310 | 274 | 96 | bh_model_draw_face | func / linked_harness | unknown |
| 0x00458430 | 49 | 20 | bh_model_draw_leaf_chain | func / linked_harness | unknown |
| 0x00458470 | 3983 | 1184 | bh_model_draw_unlit | func / linked_harness | unknown |
| 0x00459400 | 272 | 77 | bh_light_configure | func / linked_harness | unknown |
| 0x00459510 | 4772 | 1401 | bh_model_draw_lit | func / linked_harness | unknown |
| 0x0045a7c0 | 661 | 217 | bh_view_cull_sphere | func / linked_harness | unknown |
| 0x0045aa60 | 899 | 268 | bh_model_instance_draw | func / linked_harness | unknown |
| 0x0045adf0 | 965 | 281 | bh_draw_resource | func / linked_harness | unknown |
| 0x0045b1d0 | 563 | 195 | bh_render_material_initialize | func / linked_harness | unknown |
| 0x0045b410 | 113 | 44 | bh_render_depth | func / linked_harness | unknown |
| 0x0045b490 | 111 | 39 | bh_render_blend | func / linked_harness | unknown |
| 0x0045b500 | 758 | 249 | bh_render_mode | func / linked_harness | unknown |
| 0x0045b810 | 1057 | 322 | FUN_0045b810 | not tracked | unknown |
| 0x0045bc40 | 920 | 276 | FUN_0045bc40 | not tracked | unknown |
| 0x0045bfe0 | 8 | 2 | FUN_0045bfe0 | not tracked | unknown |
| 0x0045bff0 | 1305 | 394 | BH_InitRenderer | not tracked | engine_candidate |
| 0x0045c540 | 1178 | 336 | FUN_0045c540 | not tracked | unknown |
| 0x0045ca10 | 429 | 130 | FUN_0045ca10 | not tracked | unknown |
| 0x0045cbc0 | 558 | 167 | FUN_0045cbc0 | not tracked | unknown |
| 0x0045cdf0 | 320 | 122 | FUN_0045cdf0 | not tracked | unknown |
| 0x0045cf30 | 20 | 5 | FUN_0045cf30 | not tracked | unknown |
| 0x0045cf50 | 1852 | 550 | FUN_0045cf50 | not tracked | unknown |
| 0x0045d690 | 180 | 58 | FUN_0045d690 | not tracked | unknown |
| 0x0045d750 | 76 | 22 | FUN_0045d750 | not tracked | engine_candidate |
| 0x0045d7a0 | 977 | 303 | bh_horizon_color_prepare | func / linked_harness | unknown |
| 0x0045db80 | 2958 | 699 | FUN_0045db80 | not tracked | unknown |
| 0x0045e710 | 80 | 41 | bh_model_image_find | func / linked_harness | unknown |
| 0x0045e760 | 33 | 12 | bh_model_image_replace | func / linked_harness | unknown |
| 0x0045e790 | 2134 | 658 | FUN_0045e790 | not tracked | unknown |
| 0x0045eff0 | 2128 | 658 | FUN_0045eff0 | not tracked | unknown |
| 0x0045f840 | 2136 | 656 | FUN_0045f840 | not tracked | unknown |
| 0x004600a0 | 2200 | 661 | FUN_004600a0 | not tracked | unknown |
| 0x00460940 | 2151 | 664 | FUN_00460940 | not tracked | unknown |
| 0x004611b0 | 2202 | 660 | FUN_004611b0 | not tracked | unknown |
| 0x00461a50 | 2144 | 661 | FUN_00461a50 | not tracked | unknown |
| 0x004622b0 | 2144 | 662 | FUN_004622b0 | not tracked | unknown |
| 0x00462b10 | 2145 | 659 | FUN_00462b10 | not tracked | unknown |
| 0x00463380 | 2212 | 664 | FUN_00463380 | not tracked | unknown |
| 0x00463c30 | 2162 | 667 | FUN_00463c30 | not tracked | unknown |
| 0x004644b0 | 2213 | 663 | FUN_004644b0 | not tracked | unknown |
| 0x00464d60 | 2175 | 671 | FUN_00464d60 | not tracked | unknown |
| 0x004655e0 | 2189 | 674 | FUN_004655e0 | not tracked | unknown |
| 0x00465e70 | 2143 | 661 | FUN_00465e70 | not tracked | unknown |
| 0x004666d0 | 2127 | 658 | FUN_004666d0 | not tracked | unknown |
| 0x00466f20 | 2132 | 655 | FUN_00466f20 | not tracked | unknown |
| 0x00467780 | 2206 | 664 | FUN_00467780 | not tracked | unknown |
| 0x00468020 | 2144 | 661 | FUN_00468020 | not tracked | unknown |
| 0x00468880 | 2199 | 660 | FUN_00468880 | not tracked | unknown |
| 0x00469120 | 2153 | 664 | FUN_00469120 | not tracked | unknown |
| 0x00469990 | 2141 | 661 | FUN_00469990 | not tracked | unknown |
| 0x0046a1f0 | 2143 | 658 | FUN_0046a1f0 | not tracked | unknown |
| 0x0046aa50 | 2216 | 667 | FUN_0046aa50 | not tracked | unknown |
| 0x0046b300 | 2152 | 663 | FUN_0046b300 | not tracked | unknown |
| 0x0046bb70 | 2208 | 662 | FUN_0046bb70 | not tracked | unknown |
| 0x0046c410 | 2305 | 690 | FUN_0046c410 | not tracked | unknown |
| 0x0046cd20 | 2320 | 693 | FUN_0046cd20 | not tracked | unknown |
| 0x0046d630 | 925 | 314 | FUN_0046d630 | not tracked | unknown |
| 0x0046d9d0 | 974 | 331 | FUN_0046d9d0 | not tracked | unknown |
| 0x0046dda0 | 1492 | 389 | FUN_0046dda0 | not tracked | unknown |
| 0x0046e390 | 155 | 55 | bh_native_primitive_emit | wip / linked_harness | unknown |
| 0x0046e430 | 244 | 85 | bh_native_attribute_emit | wip / linked_harness | unknown |
| 0x0046e530 | 197 | 63 | FUN_0046e530 | not tracked | unknown |
| 0x0046e600 | 3713 | 1122 | bh_software_tables_initialize | func / linked_harness | unknown |
| 0x0046f490 | 3810 | 1087 | bh_atlas_image_load | func / linked_harness | unknown |
| 0x00470380 | 569 | 196 | bh_atlas_page_create | func / linked_harness | unknown |
| 0x004705c0 | 782 | 292 | bh_atlas_page_clear | func / linked_harness | unknown |
| 0x004708d0 | 33 | 10 | bh_atlas_page_cursor | func / linked_harness | unknown |
| 0x00470900 | 439 | 147 | bh_atlas_page_text | func / linked_harness | unknown |
| 0x00470ac0 | 449 | 147 | FUN_00470ac0 | not tracked | unknown |
| 0x00470c90 | 78 | 35 | bh_atlas_page_text_width | func / linked_harness | unknown |
| 0x00470ce0 | 84 | 35 | FUN_00470ce0 | not tracked | unknown |
| 0x00470d40 | 13 | 6 | bh_atlas_page_image | func / linked_harness | unknown |
| 0x00470d50 | 164 | 61 | FUN_00470d50 | not tracked | unknown |
| 0x00470e00 | 944 | 334 | FUN_00470e00 | not tracked | unknown |
| 0x004711b0 | 104 | 42 | FUN_004711b0 | not tracked | unknown |
| 0x00471220 | 63 | 22 | FUN_00471220 | not tracked | unknown |
| 0x00471260 | 350 | 113 | BH_InitDirectInput | not tracked | engine_candidate |
| 0x004713c0 | 85 | 30 | FUN_004713c0 | not tracked | unknown |
| 0x00471420 | 438 | 110 | FUN_00471420 | not tracked | unknown |
| 0x004715e0 | 1536 | 557 | FUN_004715e0 | not tracked | engine_candidate |
| 0x00471be0 | 27 | 10 | FUN_00471be0 | not tracked | engine_candidate |
| 0x00471c00 | 183 | 36 | FUN_00471c00 | not tracked | engine_candidate |
| 0x00471cc0 | 168 | 50 | FUN_00471cc0 | not tracked | unknown |
| 0x00471d70 | 119 | 32 | FUN_00471d70 | not tracked | unknown |
| 0x00471df0 | 10 | 3 | FUN_00471df0 | not tracked | engine_candidate |
| 0x00471e00 | 490 | 155 | FUN_00471e00 | not tracked | engine_candidate |
| 0x00471ff0 | 627 | 184 | FUN_00471ff0 | not tracked | game |
| 0x00472270 | 187 | 69 | FUN_00472270 | not tracked | engine_candidate |
| 0x00472330 | 144 | 51 | FUN_00472330 | not tracked | engine_candidate |
| 0x004723c0 | 5336 | 1686 | BH_WinMain | not tracked | game |
| 0x004738a0 | 102 | 26 | FUN_004738a0 | not tracked | game |
| 0x00473910 | 295 | 96 | bh_paths_init | func / linked_harness | game |
| 0x00473a40 | 508 | 185 | BH_CheckSourceFiles | not tracked | game |
| 0x00473c40 | 229 | 60 | FUN_00473c40 | not tracked | engine_candidate |
| 0x00473d60 | 627 | 143 | FUN_00473d60 | not tracked | engine_candidate |
| 0x00473fe0 | 55 | 16 | FUN_00473fe0 | not tracked | unknown |
| 0x00474020 | 1118 | 342 | bh_input_evaluate | func / linked_harness | unknown |
| 0x004744f0 | 4421 | 1107 | FUN_004744f0 | not tracked | unknown |
| 0x00475640 | 100 | 34 | bh_player_input_create | func / linked_harness | unknown |
| 0x004756b0 | 44 | 14 | bh_input_set_enabled | func / linked_harness | unknown |
| 0x004756e0 | 135 | 49 | bh_player_input_bind_axis | func / linked_harness | unknown |
| 0x00475770 | 108 | 43 | bh_player_input_bind_key | func / linked_harness | unknown |
| 0x004757e0 | 34 | 14 | bh_player_input_bind_kind4 | func / linked_harness | unknown |
| 0x00475810 | 83 | 37 | bh_player_input_bind_string | func / linked_harness | unknown |
| 0x00475870 | 256 | 86 | bh_player_input_apply_packet | func / linked_harness | unknown |
| 0x00475990 | 44 | 20 | bh_player_input_transition | func / linked_harness | unknown |
| 0x004759c0 | 177 | 56 | FUN_004759c0 | not tracked | unknown |
| 0x00475a80 | 180 | 56 | FUN_00475a80 | not tracked | unknown |
| 0x00475b40 | 29 | 11 | FUN_00475b40 | not tracked | unknown |
| 0x00475b60 | 14 | 6 | FUN_00475b60 | not tracked | unknown |
| 0x00475b70 | 43 | 14 | FUN_00475b70 | not tracked | unknown |
| 0x00475bb0 | 37 | 12 | FUN_00475bb0 | not tracked | unknown |
| 0x00475bf0 | 49 | 16 | FUN_00475bf0 | not tracked | unknown |
| 0x00475c40 | 43 | 14 | FUN_00475c40 | not tracked | unknown |
| 0x00475c80 | 73 | 35 | FUN_00475c80 | not tracked | unknown |
| 0x00475cd0 | 131 | 51 | FUN_00475cd0 | not tracked | unknown |
| 0x00475d60 | 398 | 139 | FUN_00475d60 | not tracked | unknown |
| 0x00475ef0 | 317 | 111 | FUN_00475ef0 | not tracked | engine_candidate |
| 0x00476030 | 56 | 15 | FUN_00476030 | not tracked | unknown |
| 0x00476070 | 730 | 193 | FUN_00476070 | not tracked | unknown |
| 0x00476360 | 136 | 55 | FUN_00476360 | not tracked | unknown |
| 0x004763f0 | 26 | 9 | FUN_004763f0 | not tracked | unknown |
| 0x00476410 | 323 | 98 | FUN_00476410 | not tracked | engine_candidate |
| 0x00476560 | 182 | 54 | FUN_00476560 | not tracked | unknown |
| 0x00476620 | 183 | 49 | FUN_00476620 | not tracked | unknown |
| 0x004766e0 | 465 | 142 | FUN_004766e0 | not tracked | unknown |
| 0x004768c0 | 343 | 116 | FUN_004768c0 | not tracked | unknown |
| 0x00476a20 | 527 | 161 | FUN_00476a20 | not tracked | unknown |
| 0x00476d50 | 67 | 27 | bh_network_send | func / linked_harness | engine_candidate |
| 0x00476da0 | 225 | 64 | bh_entities_init | func / linked_harness | unknown |
| 0x00476e90 | 139 | 37 | bh_entity_set_query | func / linked_harness | engine_candidate |
| 0x00476f20 | 324 | 81 | bh_entity_acquire | func / linked_harness | engine_candidate |
| 0x00477070 | 125 | 31 | bh_entity_release | func / linked_harness | unknown |
| 0x004770f0 | 498 | 171 | bh_entities_query | func / linked_harness | unknown |
| 0x004772f0 | 61 | 22 | bh_entities_step | func / linked_harness | unknown |
| 0x00477330 | 36 | 14 | bh_entities_refresh | func / linked_harness | unknown |
| 0x00477360 | 22 | 9 | FUN_00477360 | not tracked | unknown |
| 0x00477380 | 8 | 3 | FUN_00477380 | not tracked | unknown |
| 0x00477390 | 415 | 111 | bh_draw_queue_begin | func / linked_harness | unknown |
| 0x00477530 | 155 | 50 | FUN_00477530 | not tracked | unknown |
| 0x004775f0 | 80 | 29 | bh_draw_submit_execute | func / linked_harness | unknown |
| 0x00477640 | 63 | 28 | bh_draw_submit_billboard_execute | func / linked_harness | unknown |
| 0x00477680 | 50 | 23 | FUN_00477680 | not tracked | unknown |
| 0x004776c0 | 70 | 33 | bh_pickup_rect_execute | func / linked_harness | unknown |
| 0x00477710 | 78 | 37 | FUN_00477710 | not tracked | unknown |
| 0x00477760 | 94 | 45 | FUN_00477760 | not tracked | unknown |
| 0x004777c0 | 104 | 43 | FUN_004777c0 | not tracked | unknown |
| 0x00477830 | 72 | 25 | bh_camera_queue_execute | func / linked_harness | unknown |
| 0x00477880 | 345 | 135 | FUN_00477880 | not tracked | unknown |
| 0x004779e0 | 181 | 79 | FUN_004779e0 | not tracked | unknown |
| 0x00477aa0 | 52 | 22 | FUN_00477aa0 | not tracked | unknown |
| 0x00477ae0 | 570 | 189 | FUN_00477ae0 | not tracked | unknown |
| 0x00477d20 | 278 | 99 | FUN_00477d20 | not tracked | unknown |
| 0x00477e40 | 49 | 20 | bh_draw_controls_execute | func / linked_harness | unknown |
| 0x00477e80 | 26 | 11 | FUN_00477e80 | not tracked | unknown |
| 0x00477ea0 | 48 | 19 | bh_text_queue_execute_text | func / linked_harness | unknown |
| 0x00477ed0 | 48 | 19 | FUN_00477ed0 | not tracked | unknown |
| 0x00477f00 | 26 | 11 | bh_text_queue_execute_clear | func / linked_harness | unknown |
| 0x00477f20 | 26 | 11 | FUN_00477f20 | not tracked | unknown |
| 0x00477f40 | 22 | 9 | bh_draw_controls_execute | func / linked_harness | unknown |
| 0x00477f60 | 50 | 23 | FUN_00477f60 | not tracked | unknown |
| 0x00477fa0 | 50 | 23 | FUN_00477fa0 | not tracked | unknown |
| 0x00477fe0 | 122 | 43 | bh_camera_queue_pose | func / linked_harness | unknown |
| 0x00478060 | 227 | 71 | FUN_00478060 | not tracked | unknown |
| 0x00478170 | 231 | 75 | bh_draw_submit_resource | func / linked_harness | unknown |
| 0x00478280 | 236 | 75 | bh_draw_billboard | func / linked_harness | unknown |
| 0x00478390 | 259 | 83 | bh_draw_segment | func / linked_harness | unknown |
| 0x004784c0 | 116 | 37 | FUN_004784c0 | not tracked | unknown |
| 0x00478540 | 167 | 56 | FUN_00478540 | not tracked | unknown |
| 0x004785f0 | 276 | 85 | bh_draw_oriented | func / linked_harness | unknown |
| 0x00478730 | 308 | 93 | FUN_00478730 | not tracked | unknown |
| 0x00478890 | 150 | 48 | FUN_00478890 | not tracked | unknown |
| 0x00478930 | 91 | 29 | FUN_00478930 | not tracked | unknown |
| 0x00478990 | 178 | 57 | bh_draw_controls_state | func / linked_harness | unknown |
| 0x00478a70 | 199 | 63 | bh_draw_controls_clip_rect | func / linked_harness | unknown |
| 0x00478b60 | 99 | 31 | FUN_00478b60 | not tracked | unknown |
| 0x00478bd0 | 117 | 35 | bh_text_queue_text | func / linked_harness | unknown |
| 0x00478c50 | 142 | 46 | FUN_00478c50 | not tracked | unknown |
| 0x00478ce0 | 84 | 23 | bh_text_queue_clear | func / linked_harness | unknown |
| 0x00478d40 | 227 | 71 | FUN_00478d40 | not tracked | unknown |
| 0x00478e50 | 227 | 71 | FUN_00478e50 | not tracked | unknown |
| 0x00478f60 | 262 | 81 | bh_pickup_rect_queue | func / linked_harness | unknown |
| 0x00479090 | 207 | 64 | FUN_00479090 | not tracked | engine_candidate |
| 0x00479120 | 19 | 5 | FUN_00479120 | not tracked | engine_candidate |
| 0x00479140 | 175 | 47 | FUN_00479140 | not tracked | game |
| 0x004791f0 | 14 | 5 | FUN_004791f0 | not tracked | game |
| 0x00479200 | 1800 | 428 | FUN_00479200 | not tracked | game |
| 0x00479910 | 860 | 241 | bh_camera_mode_smooth | func / linked_harness | unknown |
| 0x00479c70 | 317 | 111 | bh_camera_look_at | func / linked_harness | unknown |
| 0x00479db0 | 119 | 38 | bh_camera_mode_0 | func / linked_harness | unknown |
| 0x00479e30 | 224 | 70 | bh_camera_mode_1 | func / linked_harness | unknown |
| 0x00479f10 | 224 | 70 | bh_camera_mode_10 | func / linked_harness | unknown |
| 0x00479ff0 | 246 | 81 | bh_camera_mode_2 | func / linked_harness | unknown |
| 0x0047a0f0 | 108 | 32 | bh_camera_mode_3 | func / linked_harness | unknown |
| 0x0047a160 | 134 | 41 | bh_camera_mode_4 | func / linked_harness | unknown |
| 0x0047a1f0 | 134 | 41 | bh_camera_mode_5 | func / linked_harness | unknown |
| 0x0047a280 | 108 | 32 | bh_camera_mode_6 | func / linked_harness | unknown |
| 0x0047a2f0 | 386 | 131 | bh_camera_mode_7 | func / linked_harness | unknown |
| 0x0047a480 | 397 | 139 | bh_camera_mode_8 | func / linked_harness | unknown |
| 0x0047a610 | 411 | 138 | bh_camera_mode_11 | func / linked_harness | unknown |
| 0x0047a7b0 | 783 | 248 | bh_camera_mode_9 | func / linked_harness | unknown |
| 0x0047aac0 | 1858 | 514 | bh_camera_state_update | func / linked_harness | unknown |
| 0x0047b210 | 193 | 52 | bh_camera_state_set_mode | func / linked_harness | unknown |
| 0x0047b2e0 | 56 | 15 | bh_camera_state_init | func / linked_harness | unknown |
| 0x0047b320 | 10 | 3 | bh_camera_state_set_correction | func / linked_harness | unknown |
| 0x0047b330 | 224 | 57 | bh_sound_resource_allocate | func / linked_harness | unknown |
| 0x0047b410 | 228 | 59 | bh_sound_voice_allocate | func / linked_harness | unknown |
| 0x0047b500 | 228 | 75 | FUN_0047b500 | not tracked | engine_candidate |
| 0x0047b5f0 | 273 | 93 | bh_sound_queue_dispose | func / linked_harness | unknown |
| 0x0047b710 | 287 | 74 | bh_sound_listener | func / linked_harness | engine_candidate |
| 0x0047b830 | 240 | 94 | bh_riff_scan | func / linked_harness | unknown |
| 0x0047b920 | 37 | 12 | bh_sound_set_voice_limit | func / linked_harness | engine_candidate |
| 0x0047b950 | 781 | 261 | PCM reader subset | wip / linked_harness | unknown |
| 0x0047bc60 | 212 | 73 | bh_sound_priority | func / linked_harness | unknown |
| 0x0047bd40 | 769 | 263 | bh_sound_request | func / linked_harness | unknown |
| 0x0047c050 | 231 | 77 | bh_sound_update | func / linked_harness | unknown |
| 0x0047c140 | 190 | 62 | bh_sound_stop | func / linked_harness | unknown |
| 0x0047c200 | 2151 | 666 | bh_sound_tick | func / linked_harness | unknown |
| 0x0047ca70 | 350 | 139 | bh_vfile_preload | func / linked_harness | engine_candidate |
| 0x0047cbd0 | 639 | 209 | bh_vfile_open | func / linked_harness | engine_candidate |
| 0x0047ce50 | 66 | 23 | bh_vfile_close | func / linked_harness | engine_candidate |
| 0x0047cea0 | 31 | 13 | bh_vfile_eof | func / linked_harness | engine_candidate |
| 0x0047cec0 | 146 | 60 | bh_vfile_seek | func / linked_harness | engine_candidate |
| 0x0047cf60 | 237 | 108 | bh_vfile_read | func / linked_harness | engine_candidate |
| 0x0047d050 | 467 | 188 | bh_vfile_gets | func / linked_harness | engine_candidate |
| 0x0047d230 | 58 | 26 | bh_vfile_tell | func / linked_harness | engine_candidate |
| 0x0047d270 | 240 | 65 | bh_vfile_mount | func / linked_harness | engine_candidate |
| 0x0047d360 | 6 | 1 | DirectSoundCreate | not tracked | import_thunk |
| 0x0047d366 | 6 | 1 | DirectDrawCreate | not tracked | import_thunk |
| 0x0047d36c | 6 | 1 | DirectDrawEnumerateA | not tracked | import_thunk |
| 0x0047d658 | 6 | 1 | DirectInputCreateA | not tracked | import_thunk |
| 0x0047d660 | 23 | 6 | __fpmath | not tracked | library_verified |
| 0x0047d680 | 1 | 1 | FUN_0047d680 | not tracked | shared_tail |
| 0x0047d690 | 56 | 8 | FUN_0047d690 | not tracked | library_verified |
| 0x0047d6d0 | 119 | 45 | _fclose | not tracked | library_verified |
| 0x0047d750 | 316 | 135 | _fread | not tracked | library_verified |
| 0x0047d890 | 417 | 163 | _ftell | not tracked | library_verified |
| 0x0047da40 | 150 | 60 | _fseek | not tracked | library_verified |
| 0x0047dae0 | 20 | 7 | FUN_0047dae0 | not tracked | library_verified |
| 0x0047db00 | 68 | 29 | __nh_malloc | not tracked | library_verified |
| 0x0047db50 | 56 | 21 | FUN_0047db50 | not tracked | unknown |
| 0x0047db90 | 74 | 32 | FUN_0047db90 | not tracked | library_verified |
| 0x0047dbe0 | 35 | 14 | FUN_0047dbe0 | not tracked | unknown |
| 0x0047dc10 | 21 | 8 | FUN_0047dc10 | not tracked | library_verified |
| 0x0047dc30 | 104 | 35 | _sprintf | not tracked | library_verified |
| 0x0047dca0 | 326 | 135 | _fwrite | not tracked | library_verified |
| 0x0047ddf0 | 42 | 12 | _rand | not tracked | library_verified |
| 0x0047de20 | 39 | 15 | __ftol | not tracked | compiler_support |
| 0x0047de50 | 61 | 22 | FUN_0047de50 | not tracked | library_verified |
| 0x0047de90 | 45 | 17 | FUN_0047de90 | not tracked | unknown |
| 0x0047dec0 | 42 | 13 | FUN_0047dec0 | not tracked | unknown |
| 0x0047def0 | 42 | 13 | FUN_0047def0 | not tracked | unknown |
| 0x0047df20 | 66 | 23 | _sscanf | not tracked | library_verified |
| 0x0047df70 | 25 | 8 | __CIpow | not tracked | library_verified |
| 0x0047df92 | 467 | 125 | FUN_0047df92 | not tracked | unknown |
| 0x0047e165 | 40 | 19 | FUN_0047e165 | not tracked | unknown |
| 0x0047e190 | 68 | 31 | _fflush | not tracked | library_verified |
| 0x0047e1e0 | 110 | 49 | __flush | not tracked | library_verified |
| 0x0047e260 | 114 | 50 | _flsall | not tracked | library_verified |
| 0x0047e3ca | 10 | 2 | FUN_0047e3ca | not tracked | unknown |
| 0x0047e400 | 254 | 109 | _strncpy | not tracked | library_verified |
| 0x0047e500 | 254 | 83 | _toupper | not tracked | library_verified |
| 0x0047e600 | 128 | 66 | _strstr | not tracked | library_verified |
| 0x0047e680 | 47 | 17 | FUN_0047e680 | not tracked | compiler_support |
| 0x0047e6b0 | 114 | 61 | _fgets | not tracked | library_verified |
| 0x0047e730 | 372 | 117 | entry | not tracked | library_verified |
| 0x0047e8d0 | 38 | 9 | __amsg_exit | not tracked | unknown |
| 0x0047e900 | 38 | 9 | FUN_0047e900 | not tracked | unknown |
| 0x0047e930 | 19 | 5 | __setdefaultprecision | not tracked | library_verified |
| 0x0047e950 | 80 | 25 | __ms_p5_test_fdiv | not tracked | library_verified |
| 0x0047e9a0 | 41 | 13 | FUN_0047e9a0 | not tracked | unknown |
| 0x0047e9d0 | 93 | 37 | FUN_0047e9d0 | not tracked | unknown |
| 0x0047ea30 | 107 | 53 | __cropzeros | not tracked | library_verified |
| 0x0047eaa0 | 28 | 10 | FUN_0047eaa0 | not tracked | unknown |
| 0x0047eac0 | 82 | 28 | __fassign | not tracked | library_verified |
| 0x0047eb20 | 316 | 125 | __cftoe | not tracked | library_verified |
| 0x0047ec60 | 249 | 100 | __cftof | not tracked | library_verified |
| 0x0047ed60 | 174 | 75 | __cftog | not tracked | library_verified |
| 0x0047ee10 | 43 | 13 | FUN_0047ee10 | not tracked | unknown |
| 0x0047ee40 | 38 | 11 | FUN_0047ee40 | not tracked | unknown |
| 0x0047ee70 | 101 | 38 | __cfltcvt | not tracked | library_verified |
| 0x0047eee0 | 41 | 20 | __shift | not tracked | library_verified |
| 0x0047ef10 | 210 | 76 | __close | not tracked | library_verified |
| 0x0047eff0 | 51 | 20 | __freebuf | not tracked | library_verified |
| 0x0047f030 | 236 | 89 | __filbuf | not tracked | library_verified |
| 0x0047f120 | 596 | 223 | __read | not tracked | library_verified |
| 0x0047f380 | 48 | 13 | FUN_0047f380 | not tracked | unknown |
| 0x0047f3b0 | 18 | 7 | FUN_0047f3b0 | not tracked | unknown |
| 0x0047f3d0 | 18 | 7 | __exit | not tracked | unknown |
| 0x0047f3f0 | 164 | 49 | FUN_0047f3f0 | not tracked | unknown |
| 0x0047f4a0 | 32 | 16 | __initterm | not tracked | library_verified |
| 0x0047f4c0 | 504 | 164 | __ioinit | not tracked | library_verified |
| 0x0047f6c0 | 189 | 67 | __lseek | not tracked | library_verified |
| 0x0047f780 | 32 | 13 | __callnewh | not tracked | library_verified |
| 0x0047f7a0 | 55 | 18 | FUN_0047f7a0 | not tracked | unknown |
| 0x0047f7e0 | 354 | 110 | ___sbh_new_region | not tracked | library_verified |
| 0x0047f950 | 91 | 29 | FUN_0047f950 | not tracked | unknown |
| 0x0047f9b0 | 205 | 68 | ___sbh_decommit_pages | not tracked | library_verified |
| 0x0047fa80 | 89 | 29 | ___sbh_find_block | not tracked | library_verified |
| 0x0047fae0 | 83 | 27 | ___sbh_free_block | not tracked | library_verified |
| 0x0047fb40 | 563 | 210 | ___sbh_alloc_block | not tracked | library_verified |
| 0x0047fd80 | 370 | 153 | ___sbh_alloc_block_from_page | not tracked | library_verified |
| 0x0047ff00 | 347 | 125 | __openfile | not tracked | library_verified |
| 0x004800d0 | 132 | 50 | __getstream | not tracked | library_verified |
| 0x00480160 | 301 | 117 | __flsbuf | not tracked | library_verified |
| 0x00480290 | 2172 | 646 | __output | not tracked | library_verified |
| 0x00480c20 | 66 | 26 | _write_char | not tracked | library_verified |
| 0x00480c70 | 57 | 29 | _write_multi_char | not tracked | library_verified |
| 0x00480cb0 | 61 | 31 | _write_string | not tracked | library_verified |
| 0x00480cf0 | 17 | 7 | _get_int_arg | not tracked | library_verified |
| 0x00480d10 | 18 | 7 | _get_int64_arg | not tracked | library_verified |
| 0x00480d30 | 18 | 7 | _get_short_arg | not tracked | library_verified |
| 0x00480d50 | 542 | 178 | __write | not tracked | library_verified |
| 0x00480f70 | 184 | 54 | __stbuf | not tracked | library_verified |
| 0x00481030 | 84 | 28 | __ftbuf | not tracked | library_verified |
| 0x00481090 | 103 | 26 | FUN_00481090 | not tracked | unknown |
| 0x00481100 | 149 | 52 | __isctype | not tracked | library_verified |
| 0x004811a0 | 3193 | 1022 | __input | not tracked | library_verified |
| 0x00481ee0 | 57 | 21 | __hextodec | not tracked | library_verified |
| 0x00481f20 | 33 | 15 | _fgetc | not tracked | library_verified |
| 0x00481f50 | 24 | 9 | FUN_00481f50 | not tracked | unknown |
| 0x00481f70 | 72 | 32 | FUN_00481f70 | not tracked | unknown |
| 0x0048211e | 119 | 38 | FUN_0048211e | not tracked | unknown |
| 0x004821d0 | 103 | 29 | __trandisp1 | not tracked | library_verified |
| 0x00482237 | 140 | 43 | __trandisp2 | not tracked | library_verified |
| 0x004822c8 | 5 | 3 | FUN_004822c8 | not tracked | unknown |
| 0x004823a0 | 23 | 9 | __startTwoArgErrorHandling | not tracked | library_verified |
| 0x004823b7 | 60 | 23 | __startOneArgErrorHandling | not tracked | library_verified |
| 0x00482400 | 21 | 11 | __twoToTOS | not tracked | library_verified |
| 0x00482415 | 23 | 6 | __load_CW | not tracked | library_verified |
| 0x00482445 | 67 | 21 | __fload_withFB | not tracked | library_verified |
| 0x0048249e | 13 | 5 | __fast_exit | not tracked | library_verified |
| 0x004824e9 | 163 | 54 | __check_range_exit | not tracked | library_verified |
| 0x00482590 | 545 | 187 | __powhlp | not tracked | library_verified |
| 0x004827c0 | 129 | 49 | __d_inttype | not tracked | library_verified |
| 0x00482850 | 91 | 27 | FUN_00482850 | not tracked | unknown |
| 0x004828b0 | 152 | 66 | _calloc | not tracked | library_verified |
| 0x004829c0 | 62 | 19 | __cintrindisp2 | not tracked | library_verified |
| 0x004829fe | 54 | 15 | __cintrindisp1 | not tracked | library_verified |
| 0x00482a34 | 58 | 18 | __ctrandisp2 | not tracked | library_verified |
| 0x00482a6e | 7 | 1 | FUN_00482a6e | not tracked | unknown |
| 0x00482a75 | 304 | 91 | FUN_00482a75 | not tracked | unknown |
| 0x00482ba5 | 44 | 14 | __ctrandisp1 | not tracked | library_verified |
| 0x00482bd1 | 60 | 22 | __fload | not tracked | library_verified |
| 0x00482c10 | 279 | 83 | _fdiv_main_routine | not tracked | library_verified |
| 0x00483401 | 21 | 8 | __safe_fdivr | not tracked | library_verified |
| 0x00483416 | 518 | 142 | __fprem_common | not tracked | library_verified |
| 0x0048361c | 178 | 54 | __adj_fprem | not tracked | library_verified |
| 0x004839a0 | 549 | 215 | ___crtLCMapStringA | not tracked | library_verified |
| 0x00483bd0 | 47 | 25 | _strncnt | not tracked | library_verified |
| 0x00483c16 | 187 | 87 | FUN_00483c16 | not tracked | unknown |
| 0x00483e60 | 18 | 7 | FUN_00483e60 | not tracked | unknown |
| 0x00483e80 | 58 | 19 | _x_ismbbtype | not tracked | library_verified |
| 0x00483ec0 | 238 | 92 | __setenvp | not tracked | library_verified |
| 0x00483fb0 | 158 | 52 | __setargv | not tracked | library_verified |
| 0x00484050 | 527 | 221 | _parse_cmdline | not tracked | library_verified |
| 0x00484260 | 342 | 153 | ___crtGetEnvironmentStringsA | not tracked | library_verified |
| 0x004843c0 | 528 | 182 | __setmbcp | not tracked | library_verified |
| 0x004845d0 | 77 | 15 | FUN_004845d0 | not tracked | unknown |
| 0x00484620 | 56 | 17 | _CPtoLCID | not tracked | library_verified |
| 0x00484680 | 50 | 15 | FUN_00484680 | not tracked | unknown |
| 0x004846c0 | 474 | 142 | _setSBUpLow | not tracked | library_verified |
| 0x004848a0 | 11 | 4 | FUN_004848a0 | not tracked | unknown |
| 0x004848b0 | 32 | 18 | __global_unwind2 | not tracked | compiler_support |
| 0x004848d0 | 34 | 9 | __unwind_handler | not tracked | compiler_support |
| 0x004848f2 | 104 | 33 | __local_unwind2 | not tracked | compiler_support |
| 0x00484986 | 24 | 10 | __NLG_Notify | not tracked | library_verified |
| 0x004849a8 | 189 | 73 | __except_handler3 | not tracked | compiler_support |
| 0x00484a65 | 27 | 11 | __seh_longjmp_unwind@4 | not tracked | compiler_support |
| 0x00484a80 | 61 | 18 | FUN_00484a80 | not tracked | unknown |
| 0x00484ac0 | 470 | 161 | __NMSG_WRITE | not tracked | library_verified |
| 0x00484ca0 | 59 | 26 | FUN_00484ca0 | not tracked | unknown |
| 0x00484ce0 | 24 | 8 | FUN_00484ce0 | not tracked | unknown |
| 0x00484d00 | 150 | 49 | __abstract_cw | not tracked | library_verified |
| 0x00484da0 | 139 | 46 | __hw_cw | not tracked | library_verified |
| 0x00484e30 | 254 | 83 | _tolower | not tracked | library_verified |
| 0x00484f30 | 102 | 46 | __ZeroTail | not tracked | library_verified |
| 0x00484fa0 | 105 | 48 | __IncMan | not tracked | library_verified |
| 0x00485010 | 157 | 67 | __RoundMan | not tracked | library_verified |
| 0x004850b0 | 29 | 12 | __CopyMan | not tracked | library_verified |
| 0x004850d0 | 15 | 6 | FUN_004850d0 | not tracked | unknown |
| 0x004850e0 | 29 | 12 | __IsZeroMan | not tracked | library_verified |
| 0x00485100 | 177 | 67 | __ShrMan | not tracked | library_verified |
| 0x004851c0 | 463 | 161 | __ld12cvt | not tracked | library_verified |
| 0x00485390 | 24 | 8 | FUN_00485390 | not tracked | unknown |
| 0x004853b0 | 24 | 8 | FUN_004853b0 | not tracked | unknown |
| 0x004853d0 | 56 | 21 | __atodbl | not tracked | library_verified |
| 0x00485410 | 56 | 21 | FUN_00485410 | not tracked | unknown |
| 0x00485450 | 145 | 68 | __fptostr | not tracked | library_verified |
| 0x004854f0 | 115 | 29 | __fltout | not tracked | library_verified |
| 0x00485570 | 190 | 69 | ___dtold | not tracked | library_verified |
| 0x00485630 | 664 | 231 | _memcpy | not tracked | library_verified |
| 0x00485970 | 11 | 4 | FUN_00485970 | not tracked | unknown |
| 0x00485980 | 188 | 69 | __alloc_osfhnd | not tracked | library_verified |
| 0x00485a40 | 163 | 63 | __set_osfhnd | not tracked | library_verified |
| 0x00485af0 | 148 | 52 | __free_osfhnd | not tracked | library_verified |
| 0x00485b90 | 68 | 18 | FUN_00485b90 | not tracked | unknown |
| 0x00485be0 | 93 | 27 | __getbuf | not tracked | library_verified |
| 0x00485c40 | 876 | 288 | __sopen | not tracked | library_verified |
| 0x00486000 | 40 | 13 | FUN_00486000 | not tracked | unknown |
| 0x00486030 | 113 | 35 | _wctomb | not tracked | library_verified |
| 0x004860b0 | 104 | 43 | __aulldiv | not tracked | library_verified |
| 0x00486120 | 117 | 44 | __aullrem | not tracked | library_verified |
| 0x004861a0 | 312 | 131 | ___crtGetStringTypeA | not tracked | library_verified |
| 0x004862e0 | 252 | 92 | _mbtowc | not tracked | library_verified |
| 0x004863e0 | 52 | 19 | __allmul | not tracked | library_verified |
| 0x00486420 | 31 | 15 | __allshl | not tracked | unknown |
| 0x00486440 | 131 | 62 | _ungetc | not tracked | library_verified |
| 0x004864d0 | 256 | 87 | __87except | not tracked | library_verified |
| 0x004865f0 | 57 | 15 | __set_exp | not tracked | library_verified |
| 0x00486630 | 92 | 30 | __sptype | not tracked | library_verified |
| 0x00486690 | 259 | 86 | __decomp | not tracked | library_verified |
| 0x004867a0 | 21 | 10 | __frnd | not tracked | library_verified |
| 0x004867c0 | 155 | 56 | __fpclass | not tracked | library_verified |
| 0x00486860 | 143 | 57 | ___crtMessageBoxA | not tracked | library_verified |
| 0x004868f0 | 35 | 14 | ___addl | not tracked | library_verified |
| 0x00486920 | 104 | 45 | FUN_00486920 | not tracked | unknown |
| 0x00486990 | 46 | 21 | ___shl_12 | not tracked | library_verified |
| 0x004869c0 | 45 | 21 | ___shr_12 | not tracked | library_verified |
| 0x004869f0 | 241 | 87 | ___mtold12 | not tracked | library_verified |
| 0x00486af0 | 1632 | 485 | ___strgtold12 | not tracked | library_verified |
| 0x00487280 | 898 | 303 | _$I10_OUTPUT | not tracked | library_verified |
| 0x00487610 | 378 | 126 | __chsize | not tracked | library_verified |
| 0x00487790 | 804 | 286 | __raise_exc | not tracked | library_verified |
| 0x00487ad0 | 816 | 252 | __handle_exc | not tracked | library_verified |
| 0x00487e00 | 38 | 10 | FUN_00487e00 | not tracked | unknown |
| 0x00487e30 | 3 | 2 | FUN_00487e30 | not tracked | unknown |
| 0x00487e40 | 16 | 8 | FUN_00487e40 | not tracked | unknown |
| 0x00487e50 | 17 | 9 | __clrfp | not tracked | library_verified |
| 0x00487e70 | 40 | 18 | __ctrlfp | not tracked | library_verified |
| 0x00487ea0 | 89 | 36 | __set_statfp | not tracked | library_verified |
| 0x00487f00 | 696 | 237 | ___ld12mul | not tracked | library_verified |
| 0x004881c0 | 138 | 51 | ___multtenpow12 | not tracked | library_verified |
| 0x00488250 | 126 | 38 | __setmode | not tracked | library_verified |
| 0x004882d0 | 6 | 1 | RtlUnwind | not tracked | unknown |
| 0x004882e0 | 140 | 66 | __strcmpi | not tracked | library_verified |
| 0x00488370 | 173 | 82 | __strnicmp | not tracked | library_verified |

Regenerate with `python tools/mk_function_size_map.py`. Machine-readable companion: `META/function-size-map.tsv`.
