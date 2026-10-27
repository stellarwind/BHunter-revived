#ifndef BH_SCRIPT_SENSOR_H
#define BH_SCRIPT_SENSOR_H
#include "bh_script_lifetime.h"
typedef struct BHScriptSensorState
{
    BHEntity *actor;
    long script_loaded, remaining, threshold, broadcast_radius, expected;
    float countdown, latch;
} BHScriptSensorState;
typedef enum BHScriptSensorError
{
    BH_SENSOR_OK,
    BH_SENSOR_ARGUMENT,
    BH_SENSOR_ACTOR,
    BH_SENSOR_NUMERICAL,
    BH_SENSOR_QUERY,
    BH_SENSOR_MUTATION
} BHScriptSensorError;
typedef struct BHScriptSensorContext
{
    BHScriptLifetimeContext *lifetime;
    BHEntity *const *radar;
    unsigned long radar_count;
    BHEntity *player;
    BHScriptSensorError error;
} BHScriptSensorContext;
/* Count-zero field/context adapters. Radar is the caller's bounded equivalent
   of original NULL-terminated004df278 list; only the actual player identity
   participates. Actor table supplies original one-level owner for box bounds.
   Context, radar, actual actor/owner records, pool and target entities must
   retain their lifetimes while bound. No VM/parser construction is admitted. */
int __cdecl bh_script_sensor_bind(BHScriptSensorContext *, BHEntity *, const BHScriptSensorState *, int damage);
int __cdecl bh_script_sensor_read(BHEntity *, BHScriptSensorState *);
float __cdecl bh_script_sensor_damage(const BHEntity *);
void __cdecl bh_script_sensor_set_damage(BHEntity *, float);
void __cdecl bh_script_proximity_tick(BHEntity *); /*00443ac0*/
void __cdecl bh_script_damage_tick(BHEntity *);    /*00443d40*/
#endif
