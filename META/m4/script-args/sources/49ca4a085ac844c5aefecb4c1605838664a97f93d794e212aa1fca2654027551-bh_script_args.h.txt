#ifndef BH_SCRIPT_ARGS_H
#define BH_SCRIPT_ARGS_H

/* Borrowed definition state corresponding to the original block's name,
   mask +3c, signed-short values +64 and eight 32-byte names +74.
   Active names must be NUL-terminated within their slots. The caller retains
   all storage; this is not an original-layout overlay or a global binding. */
typedef struct BHScriptArgs {
    const char *model_name;
    unsigned long used_mask;
    const short *values;
    const char (*names)[32];
} BHScriptArgs;

/* Complete 004109c0 / 00410ba0. Selector is always reset to -1; failed
   conversion/unknown variables leave value unchanged. Return is the next
   token (possibly its terminating NUL), or NULL for NULL/empty/newline input.
   No leading-whitespace normalization: callers supply the current token.
   Names use original case-insensitive prefix matching, first active slot wins.
   Values/selector must be valid; definitions must be valid on variable paths.
   Token/name bytes follow the original ASCII/CRT classification contract. */
char * __cdecl bh_script_arg_int(char *cursor,const BHScriptArgs *definitions,
                                int *value,int *selector);
char * __cdecl bh_script_arg_float(char *cursor,const BHScriptArgs *definitions,
                                  float *value,int *selector);
#endif
