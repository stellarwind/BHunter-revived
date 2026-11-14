#include "bh_script_parse.h"
#include "../../Assets/bh_vfile.h"
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
static BHScriptArgs definitions(BHScriptDefinition *script_definition)
{
    BHScriptArgs a;
    a.model_name = script_definition->name;
    a.used_mask = script_definition->used_mask;
    a.values = script_definition->values;
    a.names = script_definition->names;
    return a;
}
static void publish(BHScriptParse *script_parse, BHEntity *entity)
{
    BHScriptParseSlot *script_parse_slot = script_parse->slots + script_parse->depth;
    if (!script_parse_slot->node)
        script_parse->definition->script = entity;
    else
        memcpy((char *)script_parse_slot->node + (script_parse_slot->fired ? 0xe8 : 0xec), &entity, 4);
    script_parse_slot->node = entity;
    script_parse_slot->fired = 0;
    ++script_parse->created;
}
static int append(BHScriptParse *script_parse, long opcode, long args[5], long selectors[5])
{
    BHEntity *entity;
    if (!script_parse || !script_parse->vm || !script_parse->definition || script_parse->depth >= 32)
    {
        if (script_parse)
            script_parse->error = 1;
        return 0;
    }
    entity = bh_script_vm_create(script_parse->vm, 0, opcode, args, selectors);
    if (!entity)
    {
        script_parse->error = 2;
        return 0;
    }
    publish(script_parse, entity);
    return 1;
}
int bh_script_parse_command(BHScriptParse *script_parse, char *line, long opcode)
{
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    BHScriptArgs a;
    char *p;
    unsigned long i;
    if (!script_parse || !script_parse->definition || !line)
        return 0;
    a = definitions(script_parse->definition);
    p = arguments(line);
    for (i = 0; i < 5; ++i)
        p = bh_script_arg_int(p, &a, (int *)(args + i), (int *)(selectors + i));
    return append(script_parse, opcode, args, selectors);
}
int bh_script_parse_flag(BHScriptParse *script_parse, char *line, unsigned long flag)
{
    char *p;
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    if (!script_parse || !script_parse->definition || !line)
        return 0;
    p = arguments(line);
    if (!_strnicmp(p, "on", 2))
        args[0] = 1;
    else if (_strnicmp(p, "off", 3))
        return 1;
    if (!script_parse->depth)
    {
        if (args[0])
            script_parse->definition->flags |= flag;
        else
            script_parse->definition->flags &= ~flag;
        return 1;
    }
    args[1] = (long)flag;
    return append(script_parse, 0, args, selectors);
}
int bh_script_parse_sound(BHScriptParse *script_parse, char *line)
{
    char path[128], *p, *end;
    unsigned long resource, length, prefix;
    int mode = 0, streaming = 0;
    float level = 0, range = 0;
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    if (!script_parse || !script_parse->sounds || !script_parse->sound_prefix || !line)
        return 0;
    p = arguments(line);
    end = p;
    while (*end && !isspace((int)(signed char)*end))
        ++end;
    length = (unsigned long)(end - p);
    prefix = (unsigned long)strlen(script_parse->sound_prefix);
    if (!length || prefix + length >= sizeof(path))
    {
        script_parse->error = 1;
        return 0;
    }
    memcpy(path, script_parse->sound_prefix, prefix);
    memcpy(path + prefix, p, length);
    path[prefix + length] = 0;
    p = arguments(p);
    if (sscanf(p, "%d %f %f %d", &streaming, &level, &range, &mode) != 4 || !_finite(range) || !_finite(level))
    {
        script_parse->error = 1;
        return 0;
    }
    resource = bh_sound_load(script_parse->sounds, path, range, level, (unsigned short)streaming, 5);
    if (resource == BH_SOUND_INVALID)
        return 1;
    args[0] = (long)resource;
    args[1] = streaming;
    args[2] = mode;
    return append(script_parse, 1, args, selectors);
}
static int nested(BHScriptParse *script_parse, char *line, int kind)
{
    BHScriptArgs a = definitions(script_parse->definition);
    BHEntity *entity;
    char *p = arguments(line);
    BHScriptTimerState timer;
    BHScriptWaitState wait;
    float low = 0, high = 0;
    int count = 0, expected = 0, vc = -1, vl = -1, vh = -1, ve = -1;
    double n;
    if (script_parse->depth >= 31)
    {
        script_parse->error = 1;
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
            script_parse->error = 1;
            return 0;
        }
    }
    entity = bh_entity_acquire(script_parse->vm->lifetime->timers.pool);
    if (!entity)
    {
        script_parse->error = 2;
        return 0;
    }
    entity->position = script_parse->vm->lifetime->timers.pool->origin;
    if (kind == 2)
    {
        memcpy(&wait, bh_script_wait_fields(entity), sizeof(wait));
        wait.fired = wait.next = 0;
        wait.script_loaded = script_parse->vm->script_loading;
        wait.remaining = count;
        wait.expected = expected;
        wait.variable_remaining = vc;
        wait.variable_expected = ve;
        if (!bh_script_wait_bind(&script_parse->vm->lifetime->timers, entity, &wait))
        {
            script_parse->error = 2;
            return 0;
        }
    }
    else
    {
        memcpy(&timer, bh_script_timer_fields(entity), sizeof(timer));
        timer.fired = timer.next = 0;
        timer.script_loaded = script_parse->vm->script_loading;
        timer.remaining = count;
        n = low;
        timer.low = (long)(__int64)n;
        n = high;
        timer.high = (long)(__int64)n;
        timer.variable_remaining = vc;
        timer.variable_low = vl;
        timer.variable_high = vh;
        timer.countdown = 0;
        if (!bh_script_timer_bind(&script_parse->vm->lifetime->timers, entity, &timer, kind == 1))
        {
            script_parse->error = 2;
            return 0;
        }
    }
    publish(script_parse, entity);
    ++script_parse->depth;
    script_parse->slots[script_parse->depth].node = entity;
    script_parse->slots[script_parse->depth].fired = 1;
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
int bh_script_parse_file(BHScriptParse *script_parse, BHScriptDefinition *script_definition)
{
    BHVFile *file;
    char path[256], line[513], *p, *arg;
    unsigned long i, length, prefix;
    int value = 0;
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    if (!script_parse || !script_definition || !script_parse->vm || !script_parse->script_prefix)
    {
        if (script_parse)
            script_parse->error = 1;
        return 0;
    }
    script_parse->definition = script_definition;
    script_parse->depth = 0;
    script_parse->error = 0;
    script_parse->lines = script_parse->created = script_parse->ignored = 0;
    memset(script_parse->slots, 0, sizeof(script_parse->slots));
    for (i = 0; i < 8; ++i)
    {
        script_definition->values[i] = 0;
        script_definition->names[i][0] = 0;
    }
    script_definition->flags = 6;
    script_definition->used_mask = 0;
    length = (unsigned long)strlen(script_definition->name);
    prefix = (unsigned long)strlen(script_parse->script_prefix);
    if (length < 4 || _stricmp(script_definition->name + length - 4, ".bsp"))
        return 1;
    if (prefix + length >= sizeof(path))
    {
        script_parse->error = 1;
        return 0;
    }
    memcpy(path, script_parse->script_prefix, prefix);
    memcpy(path + prefix, script_definition->name, length + 1);
    memcpy(path + prefix + length - 4, ".TXT", 5);
    file = bh_vfile_open(path, "r");
    if (!file)
        return 1;
    while (bh_vfile_gets(line, 512, file))
    {
        ++script_parse->lines;
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
            sscanf(line + 4, "%d", &script_definition->grid);
            continue;
        }
        if (!_strnicmp(p, "timer2", 6))
        {
            if (!nested(script_parse, p, 1))
                break;
            continue;
        }
        if (!_strnicmp(p, "timer", 5))
        {
            if (!nested(script_parse, p, 0))
                break;
            continue;
        }
        if (!_strnicmp(p, "waitfor", 7))
        {
            if (!nested(script_parse, p, 2))
                break;
            continue;
        }
        if (!_strnicmp(p, "end", 5))
        {
            if (script_parse->depth)
                --script_parse->depth;
            continue;
        }
        for (i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i)
            if (!_strnicmp(p, flags[i].word, flags[i].length))
                break;
        if (i < sizeof(flags) / sizeof(flags[0]))
        {
            if (!bh_script_parse_flag(script_parse, p, (unsigned long)flags[i].opcode))
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
            if (!append(script_parse, 16, args, selectors))
                break;
            continue;
        }
        if (!_strnicmp(p, "sound", 5))
        {
            if (!bh_script_parse_sound(script_parse, p))
                break;
            continue;
        }
        if (!_strnicmp(p, "define", 6))
        {
            for (i = 0; i < 8 && (script_definition->used_mask & (1UL << i)); ++i)
            {
            }
            if (i == 8)
                continue;
            arg = arguments(p);
            value = 0;
            if (sscanf(arg, "%31s %d", script_definition->names[i], &value) < 1)
            {
                script_parse->error = 1;
                break;
            }
            script_definition->values[i] = (short)value;
            script_definition->used_mask |= 1UL << i;
            continue;
        }
        for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
            if (!_strnicmp(p, commands[i].word, commands[i].length))
                break;
        if (i < sizeof(commands) / sizeof(commands[0]))
        {
            if (!bh_script_parse_command(script_parse, p, commands[i].opcode))
                break;
        }
        else
            ++script_parse->ignored;
    }
    bh_vfile_close(file);
    return !script_parse->error;
}
