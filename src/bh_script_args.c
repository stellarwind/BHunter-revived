#include "bh_script_args.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Preserve the original CRT calls, including its multibyte-locale gates. */
#undef isdigit
#undef isspace

static char *next_token(char *cursor)
{
    while (!isspace((int)(signed char)*cursor))
    {
        ++cursor;
        if (!*cursor)
            return cursor;
    }
    while (isspace((int)(signed char)*cursor))
    {
        ++cursor;
        if (!*cursor)
            return cursor;
    }
    return cursor;
}

char *__cdecl bh_script_arg_int(char *cursor, const BHScriptArgs *definitions, int *value, int *selector)
{
    int i;
    *selector = -1;
    if (!cursor || !*cursor || *cursor == '\n')
        return NULL;
    if (isdigit((int)(signed char)*cursor) || *cursor == '-' || *cursor == '.')
    {
        sscanf(cursor, "%d", value);
    }
    else
    {
        for (i = 0; i < 8; ++i)
        {
            if ((definitions->used_mask & (1UL << i)) &&
                !_strnicmp(definitions->names[i], cursor, strlen(definitions->names[i])))
            {
                *value = (int)definitions->values[i];
                *selector = i;
                return next_token(cursor);
            }
        }
#ifdef DEBUGGING
        printf("SCRIPT ERROR: Model %s, parameter '%s' undefined\n", definitions->model_name, cursor);
#endif
    }
    return next_token(cursor);
}

char *__cdecl bh_script_arg_float(char *cursor, const BHScriptArgs *definitions, float *value, int *selector)
{
    int i;
    *selector = -1;
    if (!cursor || !*cursor || *cursor == '\n')
        return NULL;
    if (isdigit((int)(signed char)*cursor) || *cursor == '-' || *cursor == '.')
    {
        sscanf(cursor, "%f", value);
    }
    else
    {
        for (i = 0; i < 8; ++i)
        {
            if ((definitions->used_mask & (1UL << i)) &&
                !_strnicmp(definitions->names[i], cursor, strlen(definitions->names[i])))
            {
                *value = (float)definitions->values[i];
                *selector = i;
                return next_token(cursor);
            }
        }
#ifdef DEBUGGING
        printf("SCRIPT ERROR: Model %s, parameter '%s' undefined\n", definitions->model_name, cursor);
#endif
    }
    return next_token(cursor);
}
