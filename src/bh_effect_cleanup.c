#include "bh_effect_cleanup.h"
#include <string.h>
int __cdecl bh_effect_cleanup_bind(BHEffectCleanupContext *c, BHEntity *e,
                                   unsigned long handle) {
  BHEntity *a;
  unsigned long n = 0;
  int found = 0;
  if (!c || !c->pool || !c->sound || !e)
    return 0;
  for (a = c->pool->active; a && n++ < c->pool->capacity; a = a->next)
    if (a == e)
      found = 1;
  if (!found)
    return 0;
  memcpy(e->payload + 604, &c, 4);
  memcpy((char *)e + 0x128, &handle, 4);
  e->step = bh_effect_sound_cleanup;
  return 1;
}
void __cdecl bh_effect_sound_cleanup(BHEntity *e) {
  BHEffectCleanupContext *c;
  unsigned long sound;
  memcpy(&c, e->payload + 604, 4);
  memcpy(&sound, (char *)e + 0x128, 4);
  bh_sound_stop(c->sound, sound);
  bh_entity_release(c->pool, e);
}
