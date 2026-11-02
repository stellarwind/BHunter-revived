#ifndef BH_BINDING_INDEX_H
#define BH_BINDING_INDEX_H
#include "bh_scene.h"
#include <string.h>
/* Private adapter index. Values are array positions, never traversal order.
   Keys (including NULL geometry) remain borrowed until dispose/rebuild. */
typedef struct BHBindingEntry
{
    const void *first, *second;
    unsigned long value;
} BHBindingEntry;
typedef struct BHBindingIndex
{
    BHBindingEntry *entries;
    unsigned long capacity;
} BHBindingIndex;
static unsigned long bh_binding_hash(const void *a, const void *b)
{
    unsigned long x = ((unsigned long)a >> 3) ^ ((unsigned long)b * 2654435761UL);
    x ^= x >> 16;
    return x * 2654435761UL;
}
static int bh_binding_init(BHBindingIndex *index, unsigned long count, const BHSceneAllocator *mem)
{
    unsigned long capacity = 2;
    if (count > 0x20000000UL / sizeof(BHBindingEntry))
        return 0;
    while (capacity < count * 2)
        capacity *= 2;
    index->entries = (BHBindingEntry *)mem->allocate(capacity * sizeof(BHBindingEntry), mem->user);
    if (!index->entries)
        return 0;
    index->capacity = capacity;
    memset(index->entries, 0, capacity * sizeof(BHBindingEntry));
    return 1;
}
static void bh_binding_dispose(BHBindingIndex *index, const BHSceneAllocator *mem)
{
    if (index->entries)
        mem->release(index->entries, mem->user);
    memset(index, 0, sizeof(*index));
}
static long bh_binding_find(const BHBindingIndex *index, const void *a, const void *b)
{
    unsigned long slot, remaining;
    if (!index->entries || !index->capacity)
        return -1;
    slot = bh_binding_hash(a, b) & (index->capacity - 1);
    for (remaining = index->capacity; remaining; --remaining)
    {
        const BHBindingEntry *e = index->entries + slot;
        if (!e->value)
            return -1;
        if (e->first == a && e->second == b)
            return (long)(e->value - 1);
        slot = (slot + 1) & (index->capacity - 1);
    }
    return -1;
}
static int bh_binding_insert(BHBindingIndex *index, const void *a, const void *b, unsigned long value)
{
    unsigned long slot = bh_binding_hash(a, b) & (index->capacity - 1), remaining;
    for (remaining = index->capacity; remaining; --remaining)
    {
        BHBindingEntry *e = index->entries + slot;
        if (!e->value)
        {
            e->first = a;
            e->second = b;
            e->value = value + 1;
            return 1;
        }
        if (e->first == a && e->second == b)
            return 0;
        slot = (slot + 1) & (index->capacity - 1);
    }
    return 0;
}
#endif
