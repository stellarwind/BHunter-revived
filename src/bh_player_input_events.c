#include "bh_player_input_events.h"
#include <stddef.h>
#include <string.h>
static_assert(offsetof(BHEntity, payload) == 0xe4, "BHEntity.payload offset");
static_assert(offsetof(BHEntity, query) == 0xd0, "BHEntity.query offset");
void bh_player_input_bind_kind4(BHPlayerInputList *list, long index)
{
    if (list->count < index + 1)
        list->count = index + 1;
    list->bindings[index].kind = 4;
}
int bh_player_input_broadcast(BHEntity *head, unsigned long limit, unsigned long *visited)
{
    unsigned long count = 0;
    long selected;
    unsigned long packet[10];
    BHEntity *next;
    if (visited)
        *visited = 0;
    while (head)
    {
        if (count >= limit)
        {
            if (visited)
                *visited = count;
            return 0;
        }
        ++count;
        selected = head->actor.kind;
        if (selected == 1)
        {
            if (!head->query)
            {
                if (visited)
                    *visited = count;
                return 0;
            }
            packet[0] = 15;
            head->query(head, packet);
        }
        next = head->actor.global_next;
        head = next;
    }
    if (visited)
        *visited = count;
    return 1;
}
