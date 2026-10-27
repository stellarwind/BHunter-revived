#ifndef BH_SCRIPT_PARSE_H
#define BH_SCRIPT_PARSE_H
#include "bh_script_vm.h"
#include "bh_script_wait.h"
#include "bh_script_args.h"
#include "bh_sound_load.h"
typedef struct BHScriptDefinition {
 char name[16];unsigned long flags;long grid;
 unsigned long used_mask;short values[8];char names[8][32];
 BHEntity *script;
} BHScriptDefinition;
typedef struct BHScriptParseSlot {BHEntity *node;long fired;} BHScriptParseSlot;
typedef struct BHScriptParse {
 BHScriptVMContext *vm;BHSoundLoad *sounds;
 const char *script_prefix,*sound_prefix;
 BHScriptDefinition *definition;BHScriptParseSlot slots[32];unsigned long depth;
 unsigned long lines,created,ignored;int error;
} BHScriptParse;
/* Whole builders00410d90/00410f60/00411080 and parser004111e0.
   Slot0 initially denotes definition.script; nested timer/wait stacks refer
   to actual fired/next fields, never pointer arithmetic on assumed globals.
   Explicit prefixes include trailing separators. Real VFILE, argument, sound,
   timer/wait and complete VM owners are called; no retained-text substitute.
   Original command order/prefix lengths, variable first-free slots, grid's
   absolute line+4 parse, keyword steam/fountain opcode mapping and nested
   block flags are retained. Missing sidecar is a successful default.
   Malformed/unwritten locals become bounded errors; stack<32/name<32/line512.
   Context/definition/templates/resources remain borrowed until disposal. */
int __cdecl bh_script_parse_sound(BHScriptParse *,char *);
int __cdecl bh_script_parse_flag(BHScriptParse *,char *,unsigned long);
int __cdecl bh_script_parse_command(BHScriptParse *,char *,long opcode);
int __cdecl bh_script_parse_file(BHScriptParse *,BHScriptDefinition *);
#endif
