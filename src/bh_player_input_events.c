#include "bh_player_input_events.h"
#include <stddef.h>
#include <string.h>
typedef char event_payload_offset[offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
typedef char event_query_offset[offsetof(BHEntity, query) == 0xd0 ? 1 : -1];
void __cdecl bh_player_input_bind_kind4(BHPlayerInputList *list, long index)
{
    if (list->count < index + 1)
        list->count = index + 1;
    list->bindings[index].kind = 4;
}
int __cdecl bh_player_input_broadcast(BHEntity *head, unsigned long limit, unsigned long *visited)
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
        memcpy(&selected, head->payload + 24, 4);
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
        memcpy(&next, head->payload + 12, 4);
        head = next;
    }
    if (visited)
        *visited = count;
    return 1;
}
