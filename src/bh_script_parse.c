#include "bh_script_parse.h"
#include "bh_vfile.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#undef isspace
static char *arguments(char *p)
{
    if (!p)
        return 0;
    while (*p && !isspace((int)(signed char)*p))
        ++p;
    while (*p && isspace((int)(signed char)*p))
        ++p;
    return p;
}
static BHScriptArgs definitions(BHScriptDefinition *d)
{
    BHScriptArgs a;
    a.model_name = d->name;
    a.used_mask = d->used_mask;
    a.values = d->values;
    a.names = d->names;
    return a;
}
static void publish(BHScriptParse *c, BHEntity *e)
{
    BHScriptParseSlot *s = c->slots + c->depth;
    if (!s->node)
        c->definition->script = e;
    else
        memcpy((char *)s->node + (s->fired ? 0xe8 : 0xec), &e, 4);
    s->node = e;
    s->fired = 0;
    ++c->created;
}
static int append(BHScriptParse *c, long opcode, long args[5], long selectors[5])
{
    BHEntity *e;
    if (!c || !c->vm || !c->definition || c->depth >= 32)
    {
        if (c)
            c->error = 1;
        return 0;
    }
    e = bh_script_vm_create(c->vm, 0, opcode, args, selectors);
    if (!e)
    {
        c->error = 2;
        return 0;
    }
    publish(c, e);
    return 1;
}
int __cdecl bh_script_parse_command(BHScriptParse *c, char *line, long opcode)
{
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    BHScriptArgs a;
    char *p;
    unsigned long i;
    if (!c || !c->definition || !line)
        return 0;
    a = definitions(c->definition);
    p = arguments(line);
    for (i = 0; i < 5; ++i)
        p = bh_script_arg_int(p, &a, (int *)(args + i), (int *)(selectors + i));
    return append(c, opcode, args, selectors);
}
int __cdecl bh_script_parse_flag(BHScriptParse *c, char *line, unsigned long flag)
{
    char *p;
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    if (!c || !c->definition || !line)
        return 0;
    p = arguments(line);
    if (!_strnicmp(p, "on", 2))
        args[0] = 1;
    else if (_strnicmp(p, "off", 3))
        return 1;
    if (!c->depth)
    {
        if (args[0])
            c->definition->flags |= flag;
        else
            c->definition->flags &= ~flag;
        return 1;
    }
    args[1] = (long)flag;
    return append(c, 0, args, selectors);
}
int __cdecl bh_script_parse_sound(BHScriptParse *c, char *line)
{
    char path[128], *p, *end;
    unsigned long resource, length, prefix;
    int mode = 0, streaming = 0;
    float level = 0, range = 0;
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    if (!c || !c->sounds || !c->sound_prefix || !line)
        return 0;
    p = arguments(line);
    end = p;
    while (*end && !isspace((int)(signed char)*end))
        ++end;
    length = (unsigned long)(end - p);
    prefix = (unsigned long)strlen(c->sound_prefix);
    if (!length || prefix + length >= sizeof(path))
    {
        c->error = 1;
        return 0;
    }
    memcpy(path, c->sound_prefix, prefix);
    memcpy(path + prefix, p, length);
    path[prefix + length] = 0;
    p = arguments(p);
    if (sscanf(p, "%d %f %f %d", &streaming, &level, &range, &mode) != 4 || !_finite(range) || !_finite(level))
    {
        c->error = 1;
        return 0;
    }
    resource = bh_sound_load(c->sounds, path, range, level, (unsigned short)streaming, 5);
    if (resource == BH_SOUND_INVALID)
        return 1;
    args[0] = (long)resource;
    args[1] = streaming;
    args[2] = mode;
    return append(c, 1, args, selectors);
}
static int nested(BHScriptParse *c, char *line, int kind)
{
    BHScriptArgs a = definitions(c->definition);
    BHEntity *e;
    char *p = arguments(line);
    BHScriptTimerState timer;
    BHScriptWaitState wait;
    float low = 0, high = 0;
    int count = 0, expected = 0, vc = -1, vl = -1, vh = -1, ve = -1;
    double n;
    if (c->depth >= 31)
    {
        c->error = 1;
        return 0;
    }
    p = bh_script_arg_int(p, &a, &count, &vc);
    if (kind == 2)
        bh_script_arg_int(p, &a, &expected, &ve);
    else
    {
        p = bh_script_arg_float(p, &a, &low, &vl);
        bh_script_arg_float(p, &a, &high, &vh);
        if (!_finite(low) || !_finite(high))
        {
            c->error = 1;
            return 0;
        }
    }
    e = bh_entity_acquire(c->vm->lifetime->timers.pool);
    if (!e)
    {
        c->error = 2;
        return 0;
    }
    e->position = c->vm->lifetime->timers.pool->origin;
    if (kind == 2)
    {
        memcpy(&wait, e->payload + 4, 48);
        wait.fired = wait.next = 0;
        wait.script_loaded = c->vm->script_loading;
        wait.remaining = count;
        wait.expected = expected;
        wait.variable_remaining = vc;
        wait.variable_expected = ve;
        if (!bh_script_wait_bind(&c->vm->lifetime->timers, e, &wait))
        {
            c->error = 2;
            return 0;
        }
    }
    else
    {
        memcpy(&timer, e->payload + 4, 48);
        timer.fired = timer.next = 0;
        timer.script_loaded = c->vm->script_loading;
        timer.remaining = count;
        n = low;
        timer.low = (long)(__int64)n;
        n = high;
        timer.high = (long)(__int64)n;
        timer.variable_remaining = vc;
        timer.variable_low = vl;
        timer.variable_high = vh;
        timer.countdown = 0;
        if (!bh_script_timer_bind(&c->vm->lifetime->timers, e, &timer, kind == 1))
        {
            c->error = 2;
            return 0;
        }
    }
    publish(c, e);
    ++c->depth;
    c->slots[c->depth].node = e;
    c->slots[c->depth].fired = 1;
    return 1;
}
typedef struct Keyword
{
    const char *word;
    unsigned long length;
    long opcode;
} Keyword;
static const Keyword commands[] = {
    {"killsounds", 10, 2}, {"killtimers", 10, 3},    {"spark", 5, 4},     {"explode", 5, 7},   {"move", 4, 5},
    {"rotate", 6, 12},     {"steam", 5, 6},          {"fountain", 8, 9},  {"deadcar", 7, 8},   {"sensor", 6, 10},
    {"train", 5, 11},      {"powerup", 7, 13},       {"trigger2", 8, 22}, {"marker", 6, 24},   {"trigger", 7, 14},
    {"broadcast", 9, 18},  {"detach", 6, 15},        {"launch", 6, 17},   {"citypart", 8, 19}, {"location", 8, 20},
    {"blocker", 6, 23},    {"rollercoaster", 13, 21}};
static const Keyword flags[] = {{"alpha", 5, 1},       {"collision", 9, 2},      {"render", 6, 4},
                                {"signtop", 7, 128},   {"signbottom", 10, 256},  {"radarblue", 9, 512},
                                {"radarred", 8, 1024}, {"radaryellow", 11, 2048}};
int __cdecl bh_script_parse_file(BHScriptParse *c, BHScriptDefinition *d)
{
    BHVFile *file;
    char path[256], line[513], *p, *arg;
    unsigned long i, length, prefix;
    int value = 0;
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    if (!c || !d || !c->vm || !c->script_prefix)
    {
        if (c)
            c->error = 1;
        return 0;
    }
    c->definition = d;
    c->depth = 0;
    c->error = 0;
    c->lines = c->created = c->ignored = 0;
    memset(c->slots, 0, sizeof(c->slots));
    for (i = 0; i < 8; ++i)
    {
        d->values[i] = 0;
        d->names[i][0] = 0;
    }
    d->flags = 6;
    d->used_mask = 0;
    length = (unsigned long)strlen(d->name);
    prefix = (unsigned long)strlen(c->script_prefix);
    if (length < 4 || _stricmp(d->name + length - 4, ".bsp"))
        return 1;
    if (prefix + length >= sizeof(path))
    {
        c->error = 1;
        return 0;
    }
    memcpy(path, c->script_prefix, prefix);
    memcpy(path + prefix, d->name, length + 1);
    memcpy(path + prefix + length - 4, ".TXT", 5);
    file = bh_vfile_open(path, "r");
    if (!file)
        return 1;
    while (bh_vfile_gets(line, 512, file))
    {
        ++c->lines;
        p = strchr(line, '\n');
        if (p)
            *p = 0;
        p = line;
        while (*p && isspace((int)(signed char)*p))
            ++p;
        if (!*p || *p == ';')
            continue;
        if (!_strnicmp(p, "grid", 4))
        {
            sscanf(line + 4, "%d", &d->grid);
            continue;
        }
        if (!_strnicmp(p, "timer2", 6))
        {
            if (!nested(c, p, 1))
                break;
            continue;
        }
        if (!_strnicmp(p, "timer", 5))
        {
            if (!nested(c, p, 0))
                break;
            continue;
        }
        if (!_strnicmp(p, "waitfor", 7))
        {
            if (!nested(c, p, 2))
                break;
            continue;
        }
        if (!_strnicmp(p, "end", 5))
        {
            if (c->depth)
                --c->depth;
            continue;
        }
        for (i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i)
            if (!_strnicmp(p, flags[i].word, flags[i].length))
                break;
        if (i < sizeof(flags) / sizeof(flags[0]))
        {
            if (!bh_script_parse_flag(c, p, (unsigned long)flags[i].opcode))
                break;
            continue;
        }
        if (!_strnicmp(p, "blockturn", 9) || !_strnicmp(p, "blockvips", 9) || !_strnicmp(p, "blockprey", 9))
        {
            arg = arguments(p);
            if (!_strnicmp(arg, "on", 2))
                args[0] = 1;
            else if (!_strnicmp(arg, "off", 3))
                args[0] = 0;
            else
                continue;
            args[1] = !_strnicmp(p, "blockturn", 9) ? 8 : !_strnicmp(p, "blockvips", 9) ? 16 : 32;
            if (!append(c, 16, args, selectors))
                break;
            continue;
        }
        if (!_strnicmp(p, "sound", 5))
        {
            if (!bh_script_parse_sound(c, p))
                break;
            continue;
        }
        if (!_strnicmp(p, "define", 6))
        {
            for (i = 0; i < 8 && (d->used_mask & (1UL << i)); ++i)
            {
            }
            if (i == 8)
                continue;
            arg = arguments(p);
            value = 0;
            if (sscanf(arg, "%31s %d", d->names[i], &value) < 1)
            {
                c->error = 1;
                break;
            }
            d->values[i] = (short)value;
            d->used_mask |= 1UL << i;
            continue;
        }
        for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
            if (!_strnicmp(p, commands[i].word, commands[i].length))
                break;
        if (i < sizeof(commands) / sizeof(commands[0]))
        {
            if (!bh_script_parse_command(c, p, commands[i].opcode))
                break;
        }
        else
            ++c->ignored;
    }
    bh_vfile_close(file);
    return !c->error;
}
