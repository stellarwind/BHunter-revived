#include "bh_menu_object.h"
#include <string.h>
static void word(BHEntity *e, unsigned long offset, unsigned long value)
{
    memcpy((unsigned char *)e + offset, &value, 4);
}
int __cdecl bh_menu_object_create(BHMenuObject *menu, BHEntityPool *pool, BHPlayerInputOwner *owner, unsigned long tag,
                                  const char *name)
{
    BHEntity *e;
    BHPlayerInputList *list;
    BHMenuObject fresh;
    if (!menu || !pool || !owner || menu->entity || pool->dispatch_depth || !owner->allocator.allocate ||
        !owner->allocator.release)
        return 0;
    e = bh_entity_acquire(pool);
    if (!e)
        return 0;
    list = bh_player_input_create(owner, 0);
    if (!list)
    {
        bh_entity_release(pool, e);
        return 0;
    }
    memcpy((unsigned char *)e + 0x188, &list, 4);
    word(e, 0x284, tag);
    if (!name)
        name = "";
    bh_player_input_bind_key(list, 0, name, 0, 0, 0x26);
    bh_player_input_bind_key(list, 1, name, 0, 0, 0x28);
    bh_player_input_bind_key(list, 4, name, 0, 0, 0x0d);
    bh_player_input_bind_key(list, 5, name, 0, 0, 0xa6);
    bh_player_input_bind_key(list, 6, name, 0, 0, 1);
    bh_player_input_bind_key(list, 7, name, 0, 0, 0x20);
    bh_player_input_bind_key(list, 8, name, 0, 0, 0x1b);
    bh_player_input_bind_key(list, 9, name, 0, 0, 0xa7);
    bh_player_input_bind_key(list, 10, name, 0, 0, 2);
    bh_player_input_bind_kind4(list, 2);
    bh_player_input_bind_axis(list, 3, name, 0, 0, 0, 0, 3, 0);
    word(e, 0x1d8, 0xffffffffUL);
    word(e, 0x1d4, 0xffffffffUL);
    word(e, 0x1c4, 0xffffffffUL);
    word(e, 0x1bc, 0);
    word(e, 0x1cc, 0);
    word(e, 0x1d0, 0);
    word(e, 0x1dc, 0x3f800000UL);
    word(e, 0x1e0, 0);
    word(e, 0x1e4, 0x40900000UL);
    word(e, 0x1c0, 0);
    word(e, 0x1c8, 0);
    word(e, 0x1e8, 0);
    word(e, 0x1ec, 0);
    word(e, 0x1f0, 0);
    e->refresh = NULL;
    e->step = NULL; /* verified0047d680 RET */
    memset(&fresh, 0, sizeof(fresh));
    fresh.pool = pool;
    fresh.input_owner = owner;
    fresh.entity = e;
    fresh.input = list;
    *menu = fresh;
    return 1;
}
int __cdecl bh_menu_object_rows_init(BHMenuObject *m, BHAtlasContext *a, BHTextQueue *q, BHMenuRow **slots,
                                     unsigned long capacity)
{
    if (!m || !m->entity || m->rows_bound)
        return 0;
    if (!bh_menu_rows_init(&m->rows, a, q, slots, capacity))
        return 0;
    m->rows_bound = 1;
    return 1;
}
BHMenuRow *__cdecl bh_menu_object_add_row(BHMenuObject *m, unsigned long id, const unsigned char *title,
                                          unsigned char *body, unsigned long first, unsigned long second,
                                          BHMenuRowRefresh refresh)
{
    BHMenuRow *r;
    if (!m || !m->entity || !m->rows_bound)
        return NULL;
    r = bh_menu_row_create(&m->rows, id, title, body, first, second, refresh);
    if (r)
        word(m->entity, 0x1cc, m->rows.count);
    return r;
}
int __cdecl bh_menu_object_dispose(BHMenuObject *m)
{
    BHPlayerInputList *p, *list;
    BHPlayerInputOwner *o;
    if (!m || !m->entity || !m->pool || m->pool->dispatch_depth)
        return 0;
    o = m->input_owner;
    list = m->input;
    for (p = o->first; p && p != list; p = p->next)
    {
    }
    if (!p)
        return 0;
    if (m->rows_bound && !bh_menu_rows_dispose(&m->rows))
        return 0;
    if (!bh_entity_release(m->pool, m->entity))
        return 0;
    if (list->previous)
        list->previous->next = list->next;
    else
        o->first = list->next;
    if (list->next)
        list->next->previous = list->previous;
    o->allocator.release(list, o->allocator.user);
    memset(m, 0, sizeof(*m));
    return 1;
}
