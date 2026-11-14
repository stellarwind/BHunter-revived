#include "bh_text_queue.h"
#include "bh_atlas_text.h"
#include <string.h>
#include <stddef.h>

static_assert(sizeof(BHTextCommand) == 84, "BHTextCommand size");
static_assert(offsetof(BHTextCommand, page) == 4, "BHTextCommand.page offset");
static_assert(offsetof(BHTextCommand, text) == 12, "BHTextCommand.text offset");
static_assert(offsetof(BHTextCommand, next) == 80, "BHTextCommand.next offset");
static unsigned long advance(unsigned long word)
{
    return (word & 0xffffff00u) | ((word + 1) & 255);
}
void bh_text_queue_init(BHTextQueue *q)
{
    if (q)
        memset(q, 0, sizeof(*q));
}
unsigned long bh_text_queue_count(const BHTextQueue *text_queue)
{
    return text_queue ? (text_queue->write - text_queue->read) & 255 : 0;
}
static BHTextCommand *reserve(BHTextQueue *text_queue)
{
    unsigned long previous = text_queue->write, index = previous & 255;
    text_queue->write = advance(previous);
    if ((text_queue->write & 255) == (text_queue->read & 255))
    {
        text_queue->write = previous;
        text_queue->error = BH_TEXT_QUEUE_FULL;
        return NULL;
    }
    text_queue->error = BH_TEXT_QUEUE_OK;
    return text_queue->commands + index;
}
int bh_text_queue_text(BHTextQueue *text_queue, BHAtlasPage *page, const char *text, long x, long y)
{
    BHTextCommand *command;
    unsigned short bits;
    if (!text_queue)
        return -1;
    if (!text)
    {
        text_queue->error = BH_TEXT_QUEUE_ARGUMENT;
        return -1;
    }
    command = reserve(text_queue);
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
int bh_text_queue_clear(BHTextQueue *q, BHAtlasPage *page, unsigned long color)
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
unsigned long bh_text_queue_execute_text(const BHTextCommand *command)
{
    bh_atlas_page_cursor(command->page, command->position.xy[0], command->position.xy[1]);
    bh_atlas_page_text(command->page, command->text);
    return command->next;
}
unsigned long bh_text_queue_execute_clear(BHAtlasContext *atlas, const BHTextCommand *command)
{
    bh_atlas_page_clear(atlas, command->page, command->position.color);
    return command->next;
}
int bh_text_queue_drain(BHTextQueue *text_queue, BHAtlasContext *atlas, unsigned long *processed)
{
    BHTextCommand *command;
    unsigned long count = 0;
    if (processed)
        *processed = 0;
    if (!text_queue)
        return -1;
    while ((text_queue->read & 255) != (text_queue->write & 255))
    {
        command = text_queue->commands + (text_queue->read & 255);
        switch (command->kind)
        {
        case BH_TEXT_COMMAND_TEXT:
            bh_text_queue_execute_text(command);
            break;
        case BH_TEXT_COMMAND_CLEAR:
            bh_text_queue_execute_clear(atlas, command);
            break;
        default:
            text_queue->error = BH_TEXT_QUEUE_KIND;
            if (processed)
                *processed = count;
            return -1;
        }
        text_queue->read = advance(text_queue->read);
        ++count;
    }
    text_queue->error = BH_TEXT_QUEUE_OK;
    if (processed)
        *processed = count;
    return 1;
}
