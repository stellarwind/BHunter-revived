#include "bh_matrix_inverse.h"
#include "bh_round.h"
#include <string.h>
#include <math.h>
#include <float.h>

int __cdecl bh_mat4_lu(float matrix[4][4],long pivot[4],float *parity) {
    float scale[4],largest,best,reciprocal,temporary;
    double sum,score;int i,j,k,chosen;
    if(!matrix || !pivot || !parity)return 0;
    *parity=1;
    for(i=0;i<4;++i) {
        largest=0;
        for(j=0;j<4;++j) {
            if(!_finite(matrix[i][j]))return 0;
            temporary=(float)fabs(matrix[i][j]);if(largest<temporary)largest=temporary;
        }
        if(largest==0)return 0;
        bh_round_float(scale+i,1.0/largest);
    }
    for(j=0;j<4;++j) {
        for(i=0;i<j;++i) {
            sum=matrix[i][j];
            for(k=0;k<i;++k)sum-=(double)matrix[i][k]*matrix[k][j];
            bh_round_float(&matrix[i][j],sum);
        }
        best=0;chosen=j;
        for(i=j;i<4;++i) {
            sum=matrix[i][j];
            for(k=0;k<j;++k)sum-=(double)matrix[i][k]*matrix[k][j];
            /* 00401d35 scores the retained sum, not its rounded matrix store. */
            score=fabs(sum)*scale[i];bh_round_float(&matrix[i][j],sum);
            if(best<=score){bh_round_float(&best,score);chosen=i;}
        }
        if(chosen!=j) {
            for(k=0;k<4;++k){temporary=matrix[chosen][k];matrix[chosen][k]=matrix[j][k];matrix[j][k]=temporary;}
            *parity=-*parity;scale[chosen]=scale[j];
        }
        pivot[j]=chosen;
        if(j!=3 && matrix[j][j]!=0) {
            bh_round_float(&reciprocal,1.0/matrix[j][j]);
            for(i=j+1;i<4;++i)bh_round_float(&matrix[i][j],(double)matrix[i][j]*reciprocal);
        }
    }
    return 1;
}
void __cdecl bh_mat4_lu_solve(const float matrix[4][4],const long pivot[4],float vector[4]) {
    double sum;int i,j,first=-1;long selected;
    if(!matrix || !pivot || !vector)return;
    /* Invalid pivots have undefined behavior in the original. Source guards
       reject them before mutating the caller's right-hand side. */
    for(i=0;i<4;++i)if(pivot[i]<0 || pivot[i]>3)return;
    for(i=0;i<4;++i) {
        selected=pivot[i];sum=vector[selected];vector[selected]=vector[i];
        if(first>=0)for(j=first;j<i;++j)sum-=(double)vector[j]*matrix[i][j];
        else if(sum!=0)first=i;
        bh_round_float(vector+i,sum);
    }
    for(i=3;i>=0;--i) {
        sum=vector[i];for(j=i+1;j<4;++j)sum-=(double)matrix[i][j]*vector[j];
        bh_round_float(vector+i,sum/matrix[i][i]);
    }
}
int __cdecl bh_mat34_inverse_general(const BHMat34 *matrix,BHMat34 *inverse) {
    float lu[4][4],column[4],parity;long pivot[4];BHMat34 result;int i,j;
    if(!matrix || !inverse)return 0;
    memcpy(lu,matrix,sizeof(*matrix));lu[3][0]=lu[3][1]=lu[3][2]=0;lu[3][3]=1;
    if(!bh_mat4_lu(lu,pivot,&parity))return 0;
    for(j=0;j<4;++j) {
        column[0]=column[1]=column[2]=column[3]=0;column[j]=1;
        bh_mat4_lu_solve(lu,pivot,column);
        for(i=0;i<3;++i)result.m[i][j]=column[i];
    }
    *inverse=result;return 1;
}
