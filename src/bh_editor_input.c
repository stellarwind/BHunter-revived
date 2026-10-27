#include "bh_editor_input.h"
#include "bh_player_input_events.h"
#include <stddef.h>
#include <string.h>
typedef char list_packet_offset[offsetof(BHPlayerInputList, state) == 0x2010 ? 1 : -1];
typedef char entity_step_offset[offsetof(BHEntity, step) == 0xcc ? 1 : -1];
/* Count-zero source context storage, outside every original field accessed by
   this pair. Original004100b0 ignores its entity argument and reads globals. */
void __cdecl bh_editor_input_tick(BHEntity *entity)
{
    BHEditorInput *context;
    if (!entity)
        return;
    memcpy(&context, entity->payload + 420, sizeof(context));
    if (context && context->list && context->state)
        bh_player_input_apply_packet(context->state, context->list->state);
}
int __cdecl bh_editor_input_setup(BHEditorInput *context)
{
    static const unsigned short keys[29] = {0x4e, 0x2e, 0x21, 0x22, 0x25, 0x27, 0x26, 0x28, 0,    2,
                                            1,    0x5a, 0x58, 0x43, 0x56, 0x53, 0x4d, 0x71, 0x6d, 0x30,
                                            0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    unsigned long i;
    const char *name;
    if (!context || !context->owner || !context->owner->allocator.allocate || !context->pool ||
        !context->pool->allocator.allocate || !context->state || context->list || context->dispatcher)
        return 0;
    name = context->name ? context->name : "";
    context->list = bh_player_input_create(context->owner, 1);
    if (!context->list)
        return 0;
    for (i = 0; i < 17; ++i)
        if (i != 8)
            bh_player_input_bind_key(context->list, i, name, 0, (unsigned short)((i == 13 || i == 14) ? 0x11 : 0),
                                     keys[i]);
    bh_player_input_bind_kind4(context->list, 8);
    for (i = 17; i < 29; ++i)
        bh_player_input_bind_key(context->list, i, name, 0, 0, keys[i]);
    context->dispatcher = bh_entity_acquire(context->pool);
    if (context->dispatcher)
    {
        memcpy(context->dispatcher->payload + 420, &context, sizeof(context));
        context->dispatcher->step = bh_editor_input_tick;
    }
    return 1;
}
