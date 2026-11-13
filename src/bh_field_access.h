#ifndef BH_FIELD_ACCESS_H
#define BH_FIELD_ACCESS_H
#include <string.h>

/* Bit-preserving access to named fields with more than one representation.
   In particular, original identity words may contain pointers or float bits.
   Keep these transfers separate from numerical conversions and rounding. */
static __inline unsigned long bh_field_u32(const void *field)
{
    unsigned long value;
    memcpy(&value, field, sizeof(value));
    return value;
}
static __inline long bh_field_i32(const void *field)
{
    long value;
    memcpy(&value, field, sizeof(value));
    return value;
}
static __inline unsigned short bh_field_u16(const void *field)
{
    unsigned short value;
    memcpy(&value, field, sizeof(value));
    return value;
}
static __inline short bh_field_i16(const void *field)
{
    short value;
    memcpy(&value, field, sizeof(value));
    return value;
}
static __inline float bh_field_f32(const void *field)
{
    float value;
    memcpy(&value, field, sizeof(value));
    return value;
}
static __inline void bh_field_store_u32(void *field, unsigned long value)
{
    memcpy(field, &value, sizeof(value));
}
static __inline void bh_field_store_i32(void *field, long value)
{
    memcpy(field, &value, sizeof(value));
}
static __inline void bh_field_store_u16(void *field, unsigned short value)
{
    memcpy(field, &value, sizeof(value));
}
static __inline void bh_field_store_f32(void *field, float value)
{
    memcpy(field, &value, sizeof(value));
}
#endif
