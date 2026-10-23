/* Execute isolated original math routines, never the game's entry point. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "../../src/bh_math.h"

typedef void (__cdecl *BinaryMat)(const BHMat34*,const BHMat34*,BHMat34*);
typedef void (__cdecl *Cross)(const BHVec3*,const BHVec3*,BHVec3*);
typedef int (__cdecl *Normalize)(BHVec3*);
typedef int (__cdecl *NormalizeTo)(const BHVec3*,BHVec3*);
typedef void (__cdecl *Transform)(const BHMat34*,const BHVec3*,BHVec3*);
typedef void (__cdecl *Translate)(float,float,float,BHMat34*);
typedef void (__cdecl *Inverse)(const BHMat34*,BHMat34*);

static unsigned long seed=0x19990911UL;
static unsigned char *original_image;
static void *original(unsigned long address) {return original_image+address-0x400000UL;}
static float sample(void) {
    seed=seed*1664525UL+1013904223UL;
    return (float)((long)((seed>>8)&65535)-32768)/1024.0f;
}
static int map_original(const char *filename) {
    FILE *f=fopen(filename,"rb");
    unsigned char *raw,*image;
    long size;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *section;
    unsigned int i;
    if(!f) {printf("Open failed: %s\n",filename);return 0;}
    fseek(f,0,SEEK_END);size=ftell(f);fseek(f,0,SEEK_SET);
    raw=(unsigned char*)malloc(size);
    if(!raw || fread(raw,1,size,f)!=(size_t)size) return 0;
    fclose(f);
    nt=(IMAGE_NT_HEADERS*)(raw+((IMAGE_DOS_HEADER*)raw)->e_lfanew);
    if(nt->OptionalHeader.ImageBase!=0x400000UL) {printf("Unexpected base: %lx\n",nt->OptionalHeader.ImageBase);return 0;}
    image=(unsigned char*)VirtualAlloc(NULL,nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!image) {printf("VirtualAlloc failed: %lu\n",GetLastError());return 0;}
    memcpy(image,raw,nt->OptionalHeader.SizeOfHeaders);
    section=IMAGE_FIRST_SECTION(nt);
    for(i=0;i<nt->FileHeader.NumberOfSections;i++,section++)
        if(section->SizeOfRawData) memcpy(image+section->VirtualAddress,raw+section->PointerToRawData,section->SizeOfRawData);
    /* The selected routines have no external calls. Their only absolute data
       operands are the two normalization routines' 0.0/1.0 constants.
       Relocate just those four disassembly-verified x87 operands. */
    {
        int patches=0;
        for(i=0x11f0;i<0x12e0-5;i++) {
            if(image[i]==0xdc && (image[i+1]==0x15 || image[i+1]==0x3d)) {
                unsigned long *operand=(unsigned long*)(image+i+2);
                if(*operand==0x4891e0UL || *operand==0x4891e8UL) {
                    *operand=(unsigned long)(image+*operand-0x400000UL);patches++;
                }
            }
        }
        if(patches!=4) {printf("Unexpected constant operand count: %d\n",patches);return 0;}
    }
    original_image=image;
    FlushInstructionCache(GetCurrentProcess(),image,nt->OptionalHeader.SizeOfImage);
    free(raw);
    return 1;
}

int main(int argc,char **argv) {
    static const char *names[]={"multiply","cross","normalize","normalize_to","transform_point","transform_direction","inverse_transform_point","translate_world","translate_local","inverse_rigid"};
    unsigned int precision[]={_PC_24,_PC_53,_PC_64};
    int bits[10]={0},numerical[10]={0},tested[10]={0};
    int bits_by_precision[3][10]={{0}};
    int pc,n,k,i,ra,rb,failed=0;
    BHMat34 a,b,oa,ob;
    BHVec3 v,w,va,vb;
    float *fa,*fb;
    if(argc!=2 || !map_original(argv[1])) {puts("Could not map original EXE");return 2;}
    for(pc=0;pc<3;pc++) {
      _controlfp(precision[pc],_MCW_PC);
      for(n=0;n<2000;n++) {
        for(i=0;i<12;i++) {((float*)&a)[i]=sample();((float*)&b)[i]=sample();}
        v.x=sample();v.y=sample();v.z=sample();w.x=sample();w.y=sample();w.z=sample();
        if(n==0) {v.x=0;v.y=0;v.z=0;}
        for(k=0;k<10;k++) {
          memset(&oa,0x3f,sizeof(oa));ob=oa;
          va.x=va.y=va.z=123.0f;vb=va;ra=rb=0;
          switch(k) {
          case 0: ((BinaryMat)original(0x401000))(&a,&b,&oa);bh_mat34_multiply(&a,&b,&ob);break;
          case 1:
            if(n%3==1) {
                va=vb=v;
                ((Cross)original(0x4011a0))(&va,&w,&va);bh_vec3_cross(&vb,&w,&vb);
            } else if(n%3==2) {
                va=vb=w;
                ((Cross)original(0x4011a0))(&v,&va,&va);bh_vec3_cross(&v,&vb,&vb);
            } else {
                ((Cross)original(0x4011a0))(&v,&w,&va);bh_vec3_cross(&v,&w,&vb);
            }
            break;
          case 2: va=vb=v;ra=((Normalize)original(0x4011f0))(&va);rb=bh_vec3_normalize(&vb);break;
          case 3: ra=((NormalizeTo)original(0x401270))(&v,&va);rb=bh_vec3_normalize_to(&v,&vb);break;
          case 4: ((Transform)original(0x4012e0))(&a,&v,&va);bh_mat34_transform_point(&a,&v,&vb);break;
          case 5: ((Transform)original(0x401360))(&a,&v,&va);bh_mat34_transform_direction(&a,&v,&vb);break;
          case 6: ((Transform)original(0x4013d0))(&a,&v,&va);bh_mat34_inverse_transform_point(&a,&v,&vb);break;
          case 7: oa=ob=a;((Translate)original(0x401460))(v.x,v.y,v.z,&oa);bh_mat34_translate_world(v.x,v.y,v.z,&ob);break;
          case 8: oa=ob=a;((Translate)original(0x401490))(v.x,v.y,v.z,&oa);bh_mat34_translate_local(v.x,v.y,v.z,&ob);break;
          case 9: ((Inverse)original(0x401a70))(&a,&oa);bh_mat34_inverse_rigid(&a,&ob);break;
          }
          fa=(k==0||k>=7)?(float*)&oa:(float*)&va;
          fb=(k==0||k>=7)?(float*)&ob:(float*)&vb;
          i=(k==0||k>=7)?12:3;
          tested[k]++;
          if(ra!=rb || memcmp(fa,fb,i*sizeof(float))) {
            int different=(ra!=rb),j;bits[k]++;bits_by_precision[pc][k]++;
            for(j=0;j<i;j++) if(fabs((double)fa[j]-fb[j])>1e-5*(1+fabs(fa[j])+fabs(fb[j]))) different=1;
            if(different) {numerical[k]++;failed=1;}
          }
        }
      }
    }
    for(k=0;k<10;k++) printf("%s cases=%d bit_differences=%d pc24=%d pc53=%d pc64=%d numerical_failures=%d\n",names[k],tested[k],bits[k],bits_by_precision[0][k],bits_by_precision[1][k],bits_by_precision[2][k],numerical[k]);
    return failed?1:0;
}
