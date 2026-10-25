#include "../src/bh_script_args.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL script-args %lu: %s\n",checks,s);}}while(0)

typedef struct Literal {char text[40];int integer;float real;unsigned long next;} Literal;
static const Literal literals[]={
 {"0",0,0.0f,1},{"123 next",123,123.0f,4},{"-32768\t next",-32768,-32768.0f,8},
 {"2147483647",INT_MAX,2147483647.0f,10},{"-2147483648",INT_MIN,-2147483648.0f,11},
 {".5\nmore",73,.5f,3},{"-.25 tail",73,-.25f,5},{"1e3\v\f\r x",1,1000.0f,7},
 {"12junk next",12,12.0f,7},{"- tail",73,81.0f,2},{". tail",73,81.0f,2},
 {"42 \t\r\n",42,42.0f,6},{"007 end",7,7.0f,4}
};
static void literal_checks(void) {
 unsigned long j;int value,selector;float real;char text[40],*p;
 for(j=0;j<sizeof(literals)/sizeof(literals[0]);++j){
  strcpy(text,literals[j].text);value=73;selector=99;
  p=bh_script_arg_int(text,NULL,&value,&selector);
  CHECK(value==literals[j].integer,"integer literal or failed scan retains value");
  CHECK(selector==-1,"literal selector remains minus one");
  CHECK(p==text+literals[j].next,"integer cursor skips token and trailing whitespace");
  CHECK(!strcmp(text,literals[j].text),"integer input unmodified");
  real=81.0f;selector=99;p=bh_script_arg_float(text,NULL,&real,&selector);
  CHECK(real==literals[j].real,"float literal or failed scan retains value");
  CHECK(selector==-1,"float literal selector remains minus one");
  CHECK(p==text+literals[j].next,"float cursor skips token and trailing whitespace");
  CHECK(!strcmp(text,literals[j].text),"float input unmodified");
 }
}
static void definition_checks(void) {
 BHScriptArgs d;short values[8]={-32768,32767,-1,0,1,1234,-2048,42};
 char names[8][32]={{0}},before[8][32],text[80],*p;int i,value,selector;float real;
 d.model_name="ARGFIXTURE";d.used_mask=255;d.values=values;d.names=(const char (*)[32])names;
 for(i=0;i<8;++i)sprintf(names[i],"V%d",i);memcpy(before,names,sizeof(names));
 for(i=0;i<8;++i){
  sprintf(text,"v%d_suffix\t remaining",i);value=73;selector=99;
  p=bh_script_arg_int(text,&d,&value,&selector);
  CHECK(value==values[i],"each variable signed-short integer value");
  CHECK(selector==i,"each variable integer selector");
  CHECK(p==text+11,"variable integer cursor after full suffix token");
  real=81;selector=99;p=bh_script_arg_float(text,&d,&real,&selector);
  CHECK(real==(float)values[i],"each variable signed-short float value");
  CHECK(selector==i,"each variable float selector");
  CHECK(p==text+11,"variable float cursor after full suffix token");
 }
 CHECK(!memcmp(names,before,sizeof(names)),"borrowed names unmodified by both readers");
 strcpy(names[0],"prefix");strcpy(names[1],"prefixlong");strcpy(text,"PREFIXLONG tail");
 value=73;selector=99;p=bh_script_arg_int(text,&d,&value,&selector);
 CHECK(value==-32768&&selector==0&&p==text+11,"first active prefix beats longer later name");
 d.used_mask=2;real=81;selector=99;p=bh_script_arg_float(text,&d,&real,&selector);
 CHECK(real==32767.0f&&selector==1&&p==text+11,"used-mask gate exposes longer later slot");
 strcpy(text,"prefixlong");value=73;p=bh_script_arg_int(text,&d,&value,&selector);
 CHECK(p==text+10&&selector==1&&value==32767,"variable ending at NUL returns NUL pointer");
 memset(names[7],0,32);d.used_mask=128;strcpy(text,"+12 rest");
 value=73;p=bh_script_arg_int(text,&d,&value,&selector);
 CHECK(value==42&&selector==7&&p==text+4,"empty active variable name matches nonnumeric token");
 d.used_mask=0;strcpy(text,"missing next");value=73;selector=99;
 p=bh_script_arg_int(text,&d,&value,&selector);
 CHECK(value==73&&selector==-1&&p==text+8,"unknown integer retains prior value and advances");
 real=81;selector=99;p=bh_script_arg_float(text,&d,&real,&selector);
 CHECK(real==81&&selector==-1&&p==text+8,"unknown float retains prior value and advances");
 strcpy(text," 123 tail");value=73;p=bh_script_arg_int(text,&d,&value,&selector);
 CHECK(value==73&&selector==-1&&p==text+1,"leading whitespace is not normalized before dispatch");
 strcpy(text,"+12 tail");real=81;p=bh_script_arg_float(text,&d,&real,&selector);
 CHECK(real==81&&selector==-1&&p==text+4,"plus prefix uses variable path rather than numeric scan");
 strcpy(text,"9 V0");value=73;p=bh_script_arg_int(text,&d,&value,&selector);
 CHECK(value==9&&selector==-1&&p==text+2,"zero variable mask does not suppress literal parse");
 CHECK(values[0]==-32768&&values[1]==32767&&values[7]==42,"borrowed values remain unchanged");
 memset(names[7],'A',31);names[7][31]=0;d.used_mask=128;
 memset(text,'a',31);strcpy(text+31," rest");real=81;
 p=bh_script_arg_float(text,&d,&real,&selector);
 CHECK(real==42&&selector==7&&p==text+32,"31-character slot uses terminator and case-insensitive prefix");
 strcpy(names[7],"12");strcpy(text,"12 rest");value=73;
 p=bh_script_arg_int(text,&d,&value,&selector);
 CHECK(value==12&&selector==-1&&p==text+3,"numeric dispatch precedes matching variable name");
}
int __cdecl bh_script_args_checkpoint(unsigned long *count) {
 int value,selector;float real;char empty[]="",newline[]="\n12",last[]="\n";
 checks=failures=0;value=73;selector=99;
 CHECK(sizeof(short)==2&&sizeof(int)==4&&sizeof(float)==4,"original integer and binary32 widths");
 CHECK(bh_script_arg_int(NULL,NULL,&value,&selector)==NULL&&value==73&&selector==-1,"NULL integer resets selector only");
 selector=99;CHECK(bh_script_arg_int(empty,NULL,&value,&selector)==NULL&&value==73&&selector==-1,"empty integer resets selector only");
 selector=99;CHECK(bh_script_arg_int(newline,NULL,&value,&selector)==NULL&&value==73&&selector==-1,"newline integer returns NULL without scanning");
 real=81;selector=99;CHECK(bh_script_arg_float(NULL,NULL,&real,&selector)==NULL&&real==81&&selector==-1,"NULL float resets selector only");
 selector=99;CHECK(bh_script_arg_float(empty,NULL,&real,&selector)==NULL&&real==81&&selector==-1,"empty float resets selector only");
 selector=99;CHECK(bh_script_arg_float(last,NULL,&real,&selector)==NULL&&real==81&&selector==-1,"newline float returns NULL");
 literal_checks();definition_checks();
 if(count)*count=checks;
 printf("{\"scenario\":\"script-args\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
 return failures!=0;
}
#ifdef BH_SCRIPT_ARGS_STANDALONE
int main(void){return bh_script_args_checkpoint(NULL);}
#endif
