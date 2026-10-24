/* Optional diagnostic only. Reuse the unchanged pinned ten-function mapper. */
#define main historical_math_main
#include "../analysis/math/differential.c"
#undef main
#include "math_cases.h"

int main(int argc,char **argv) {
    BHMathOps reference,source;
    BHMathCase input;
    BHMathResult expected,actual;
    unsigned int modes[]={_PC_53,_PC_24,_PC_64},saved=_control87(0,0);
    unsigned long counts[10][8],id,ulp;
    int mode,function,alias,i,bad,first[10],strict_failures=0;
    FILE *golden=NULL;
    if (argc<2 || argc>3 || !map_original(argv[1])) return 2;
    reference.multiply=(BinaryMat)original(0x401000);reference.cross=(Cross)original(0x4011a0);
    reference.normalize=(Normalize)original(0x4011f0);reference.normalize_to=(NormalizeTo)original(0x401270);
    reference.point=(Transform)original(0x4012e0);reference.direction=(Transform)original(0x401360);
    reference.inverse_point=(Transform)original(0x4013d0);reference.world=(Translate)original(0x401460);
    reference.local=(Translate)original(0x401490);reference.inverse=(Inverse)original(0x401a70);
    bh_math_source_ops(&source);
    if (argc==3) { golden=fopen(argv[2],"wb");if (!golden) return 2;
        fprintf(golden,"/* Original pure-math PC53 captures; generated only by the optional oracle. */\n"); }
    for (mode=0;mode<3;mode++) {
        _clearfp();_control87(modes[mode]|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);
        memset(counts,0,sizeof(counts));memset(first,0,sizeof(first));
        for (id=0;id<10496;id++) {
            bh_math_case(id,&input);
            for (function=0;function<10;function++) for (alias=0;alias<(function==1?5:((function>=3&&function<=6)?2:1));alias++) {
                bh_math_eval(&reference,function,alias,&input,&expected);
                bh_math_eval(&source,function,alias,&input,&actual);
                ++counts[function][0];bad=0;
                if (actual.status!=expected.status) {++counts[function][1];bad=1;}
                if (!actual.guards_ok || !expected.guards_ok || !actual.control_ok || !expected.control_ok) {++counts[function][7];bad=1;}
                if (actual.fp_status!=expected.fp_status) ++counts[function][6];
                for (i=0;i<actual.count;i++) if (actual.words[i]!=expected.words[i]) {
                    int ca=bh_math_word_class(actual.words[i]),ce=bh_math_word_class(expected.words[i]);
                    if (ca==3 && ce==3) ++counts[function][5];
                    else if (ca>=2 || ce>=2) {++counts[function][3];bad=1;}
                    else {++counts[function][2];bad=1;ulp=bh_math_ulp(actual.words[i],expected.words[i]);if(ulp>counts[function][4]) counts[function][4]=ulp;}
                }
                if (bad && !first[function]++) {
                    printf("FIRST mode=%d function=%d alias=%d case=%lu expected_status=%d actual_status=%d expected=",mode,function,alias,id,expected.status,actual.status);
                    for(i=0;i<actual.count;i++)printf("%08lx,",expected.words[i]);
                    printf(" actual=");for(i=0;i<actual.count;i++)printf("%08lx,",actual.words[i]);puts("");
                }
                if (mode==0 && bad) ++strict_failures;
                if (golden && mode==0 && (id<128 || (id>=2048 && id<10240 && id%257==0) || (id>=10240 && id%4==0))) {
                    fprintf(golden,"{%lu,%d,%d,%d,%d,{",id,function,alias,expected.status,expected.count);
                    for(i=0;i<expected.count;i++)fprintf(golden,"0x%08lxUL%s",expected.words[i],i==expected.count-1?"":",");
                    fprintf(golden,"}},\n");
                }
            }
        }
        for(function=0;function<10;function++)printf("{\"mode\":%d,\"function\":%d,\"cases\":%lu,\"return_differences\":%lu,\"finite_word_differences\":%lu,\"classification_or_infinity_differences\":%lu,\"max_ulp\":%lu,\"nan_payload_differences\":%lu,\"fp_status_differences\":%lu,\"guard_or_control_failures\":%lu}\n",mode,function,counts[function][0],counts[function][1],counts[function][2],counts[function][3],counts[function][4],counts[function][5],counts[function][6],counts[function][7]);
    }
    if(golden)fclose(golden);
    _clearfp();_control87(saved,_MCW_PC|_MCW_RC|_MCW_EM);
    VirtualFree(original_image,0,MEM_RELEASE);
    printf("{\"scenario\":\"expanded-math-oracle\",\"pc53_failing_cases\":%d}\n",strict_failures);
    return strict_failures ? 1 : 0;
}
