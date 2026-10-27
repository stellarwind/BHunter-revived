#include "bh_text_queue.h"
#include "bh_atlas_text.h"
#include <string.h>
#include <stddef.h>

typedef char text_command_size[sizeof(BHTextCommand) == 84 ? 1 : -1];
typedef char text_command_page[offsetof(BHTextCommand, page) == 4 ? 1 : -1];
typedef char text_command_string[offsetof(BHTextCommand, text) == 12 ? 1 : -1];
typedef char text_command_next[offsetof(BHTextCommand, next) == 80 ? 1 : -1];
static unsigned long advance(unsigned long word)
{
    return (word & 0xffffff00u) | ((word + 1) & 255);
}
void __cdecl bh_text_queue_init(BHTextQueue *q)
{
    if (q)
        memset(q, 0, sizeof(*q));
}
unsigned long __cdecl bh_text_queue_count(const BHTextQueue *q)
{
    return q ? (q->write - q->read) & 255 : 0;
}
static BHTextCommand *reserve(BHTextQueue *q)
{
    unsigned long previous = q->write, index = previous & 255;
    q->write = advance(previous);
    if ((q->write & 255) == (q->read & 255))
    {
        q->write = previous;
        q->error = BH_TEXT_QUEUE_FULL;
        return NULL;
    }
    q->error = BH_TEXT_QUEUE_OK;
    return q->commands + index;
}
int __cdecl bh_text_queue_text(BHTextQueue *q, BHAtlasPage *page, const char *text, long x, long y)
{
    BHTextCommand *command;
    unsigned short bits;
    if (!q)
        return -1;
    if (!text)
    {
        q->error = BH_TEXT_QUEUE_ARGUMENT;
        return -1;
    }
    command = reserve(q);
    if (!command)
        return 0;
    command->kind = BH_TEXT_COMMAND_TEXT;
    strncpy((char *)command->text, text, 59);
    command->text[59] = 0;
    command->page = page;
    bits = (unsigned short)x;
    memcpy(&command->position.xy[0], &bits, 2);
    bits = (unsigned short)y;
    memcpy(&command->position.xy[1], &bits, 2);
    return 1;
}
int __cdecl bh_text_queue_clear(BHTextQueue *q, BHAtlasPage *page, unsigned long color)
{
    BHTextCommand *command;
    if (!q)
        return -1;
    command = reserve(q);
    if (!command)
        return 0;
    command->kind = BH_TEXT_COMMAND_CLEAR;
    command->position.color = color;
    command->page = page;
    return 1;
}
unsigned long __cdecl bh_text_queue_execute_text(const BHTextCommand *command)
{
    bh_atlas_page_cursor(command->page, command->position.xy[0], command->position.xy[1]);
    bh_atlas_page_text(command->page, command->text);
    return command->next;
}
unsigned long __cdecl bh_text_queue_execute_clear(BHAtlasContext *atlas, const BHTextCommand *command)
{
    bh_atlas_page_clear(atlas, command->page, command->position.color);
    return command->next;
}
int __cdecl bh_text_queue_drain(BHTextQueue *q, BHAtlasContext *atlas, unsigned long *processed)
{
    BHTextCommand *command;
    unsigned long count = 0;
    if (processed)
        *processed = 0;
    if (!q)
        return -1;
    while ((q->read & 255) != (q->write & 255))
    {
        command = q->commands + (q->read & 255);
        switch (command->kind)
        {
        case BH_TEXT_COMMAND_TEXT:
            bh_text_queue_execute_text(command);
            break;
        case BH_TEXT_COMMAND_CLEAR:
            bh_text_queue_execute_clear(atlas, command);
            break;
        default:
            q->error = BH_TEXT_QUEUE_KIND;
            if (processed)
                *processed = count;
            return -1;
        }
        q->read = advance(q->read);
        ++count;
    }
    q->error = BH_TEXT_QUEUE_OK;
    if (processed)
        *processed = count;
    return 1;
}
