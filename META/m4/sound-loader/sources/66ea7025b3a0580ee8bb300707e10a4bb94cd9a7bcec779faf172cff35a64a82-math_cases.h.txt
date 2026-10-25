#ifndef BH_MATH_CASES_H
#define BH_MATH_CASES_H
#include "../src/bh_math.h"

typedef struct BHMathOps {
    void (__cdecl *multiply)(const BHMat34*,const BHMat34*,BHMat34*);
    void (__cdecl *cross)(const BHVec3*,const BHVec3*,BHVec3*);
    int (__cdecl *normalize)(BHVec3*);
    int (__cdecl *normalize_to)(const BHVec3*,BHVec3*);
    void (__cdecl *point)(const BHMat34*,const BHVec3*,BHVec3*);
    void (__cdecl *direction)(const BHMat34*,const BHVec3*,BHVec3*);
    void (__cdecl *inverse_point)(const BHMat34*,const BHVec3*,BHVec3*);
    void (__cdecl *world)(float,float,float,BHMat34*);
    void (__cdecl *local)(float,float,float,BHMat34*);
    void (__cdecl *inverse)(const BHMat34*,BHMat34*);
} BHMathOps;

typedef struct BHMathCase { BHMat34 a,b;BHVec3 v,w; } BHMathCase;
typedef struct BHMathResult {
    unsigned long words[12];
    int count, status, guards_ok, control_ok;
    unsigned int fp_status;
} BHMathResult;

void bh_math_source_ops(BHMathOps *ops);
void bh_math_case(unsigned long id, BHMathCase *input);
void bh_math_eval(const BHMathOps *ops,int function,int alias,const BHMathCase *input,BHMathResult *result);
int bh_math_word_class(unsigned long word);
unsigned long bh_math_ulp(unsigned long a,unsigned long b);
#endif
