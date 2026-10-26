#include "../src/bh_effect_cleanup.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks, failures;
#define CHECK(x, s)                                                            \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(x)) {                                                                \
      ++failures;                                                              \
      printf("FAIL effect-cleanup %d %s\n", __LINE__, s);                      \
    }                                                                          \
  } while (0)
int __cdecl bh_effect_cleanup_checkpoint(unsigned long *count) {
  BHEntityPool pool;
  BHSoundQueue sounds;
  BHSoundResource *r;
  BHEffectCleanupContext c;
  BHEntity *e, *other;
  unsigned long h, foreign, used, i;
  BHSoundVoice *v;
  checks = failures = 0;
  CHECK(bh_entities_init(&pool, 4, NULL) &&
            bh_sound_queue_init(&sounds, NULL, NULL, NULL),
        "actual source pool and sound queue");
  c.pool = &pool;
  c.sound = &sounds;
  r = bh_sound_resource_allocate(&sounds);
  CHECK(r != NULL, "actual resource owner");
  if (!r)
    return 1;
  r->streaming = 1;
  r->base_frequency = 8000;
  for (i = 0; i < 3; ++i) {
    e = bh_entity_acquire(&pool);
    other = bh_entity_acquire(&pool);
    h = bh_sound_request(&sounds, r->index, 1, 1, NULL, NULL);
    foreign = bh_sound_request(&sounds, r->index, 1, 1, NULL, NULL);
    CHECK(h != BH_SOUND_INVALID && foreign != BH_SOUND_INVALID,
          "actual independent queued voices");
    v = bh_sound_voice(&sounds, h);
    CHECK(v && v->resource==r, "positive sound exists before cleanup");
    if (i == 1)
      CHECK(bh_sound_stop(&sounds, h) == 1,
            "prepare actual stale sound handle");
    CHECK(bh_effect_cleanup_bind(&c, e, i == 2 ? BH_SOUND_INVALID : h) &&
              e->step == bh_effect_sound_cleanup,
          "real original handle word and complete callback installed");
    used = pool.used;
    e->step(e);
    CHECK(pool.used == used - 1 && pool.active == other,
          "whole owner releases actual entity even on stale/invalid sound");
    CHECK((i == 2 || !bh_sound_voice(&sounds, h)) &&
              bh_sound_voice(&sounds, foreign) != NULL,
          "whole cleanup stops only selected live voice and preserves foreign "
          "voice");
    if (i == 2)
      bh_sound_stop(&sounds, h);
    bh_sound_stop(&sounds, foreign);
    bh_entity_release(&pool, other);
  }
  CHECK(bh_sound_queue_dispose(&sounds) && bh_entities_dispose(&pool),
        "actual source owners disposed");
  if (count)
    *count = checks;
  printf("{\"scenario\":\"effect-cleanup\",\"checks\":%lu,\"failures\":%lu,"
         "\"original_code_required\":false,\"play_calls\":0}\n",
         checks, failures);
  return failures != 0;
}
