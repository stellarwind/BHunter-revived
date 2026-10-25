#include "../src/bh_loading_blit.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct Case {long kind,width,height;unsigned long from,to,rows[24];} Case;
static const Case cases[]={
#include "loading_blit_cases.h"
};
typedef int (__cdecl *Blit)(BHLoadingBlit *,const BHLoadingBlitTarget *,float,float);
static Blit functions[5]={bh_loading_blit_top,bh_loading_blit_right,bh_loading_blit_bottom,bh_loading_blit_left,bh_loading_blit_top_last};
typedef struct FullCase {long kind,width,height;unsigned long from,to,writes,crc[3];} FullCase;
static const FullCase full_cases[]={
#include "loading_blit_full_cases.h"
};
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){if(failures++<12)printf("FAIL loading-blit %lu: %s\n",checks,s);}}while(0)
static unsigned long expected_pixel(BHLoadingImages *images,unsigned long x,unsigned long y,unsigned long width) {
 unsigned char index=images->arena[images->frame_stride+y*width+x];return images->pixel_bytes==1?images->indexed[index]:images->native[index];
}
static void expected_store(unsigned char *expected,long at,unsigned long value,unsigned long n) {
 unsigned long i;for(i=0;i<n;++i)expected[at+i]=(unsigned char)(value>>(i*8));
}
static void guarded_spans(void) {
 SYSTEM_INFO system;DWORD old;unsigned char *source=NULL,*destination=NULL;
 BHLoadingImages images;BHLoadingBlit c;BHLoadingBlitTarget t;unsigned long i,n;
 memset(&images,0,sizeof(images));memset(&c,0,sizeof(c));GetSystemInfo(&system);
 source=(unsigned char*)VirtualAlloc(NULL,system.dwPageSize*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 destination=(unsigned char*)VirtualAlloc(NULL,system.dwPageSize*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 CHECK(source&&destination,"real guarded source and target allocations");if(!source||!destination)goto done;
 CHECK(VirtualProtect(source+system.dwPageSize,system.dwPageSize,PAGE_NOACCESS,&old)&&
  VirtualProtect(destination+system.dwPageSize,system.dwPageSize,PAGE_NOACCESS,&old),"actual inaccessible end pages");
 images.arena=source+system.dwPageSize-800;images.arena_bytes=800;images.frame_stride=32;
 for(i=0;i<800;++i)images.arena[i]=(unsigned char)i;
 for(i=0;i<256;++i){images.indexed[i]=(unsigned char)i;images.native[i]=0x010101UL*i;}
 c.images=&images;c.width=32;c.height=24;
 for(n=1;n<=4;++n){images.pixel_bytes=n;t.bytes=32*24*n;t.base=destination+system.dwPageSize-t.bytes;t.row0=0;t.pitch=32*n;
  memset(t.base,0xcd,t.bytes);c.writes=0;
  CHECK(bh_loading_blit_right(&c,&t,.125f,.375f)&&bh_loading_blit_bottom(&c,&t,.375f,.625f)&&
   bh_loading_blit_left(&c,&t,.625f,.875f)&&bh_loading_blit_top(&c,&t,0,.125f)&&bh_loading_blit_top_last(&c,&t,.875f,1),"all whole owners operate at exact protected source/target end");
  CHECK(n==3?c.writes==0:c.writes>0,"3-byte no-store and supported depth guarded writes");
 }
done:
 if(source)VirtualFree(source,0,MEM_RELEASE);if(destination)VirtualFree(destination,0,MEM_RELEASE);
}
static unsigned long crc32(const unsigned char *p,unsigned long n) {
 unsigned long crc=0xffffffffUL,i;int bit;
 for(i=0;i<n;++i){crc^=p[i];for(bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320UL&(0UL-(crc&1)));}return crc^0xffffffffUL;
}
static void full_resolution(void) {
 BHLoadingImages images;BHLoadingBlit c;BHLoadingBlitTarget t;unsigned char *source=NULL,*pixels=NULL;
 unsigned long i,j,n,size,x,y,formats[3]={1,2,4};float from,to;
 memset(&images,0,sizeof(images));memset(&c,0,sizeof(c));c.images=&images;source=(unsigned char*)malloc(640*480);pixels=(unsigned char*)malloc(64+(640*4+3)*480);
 CHECK(source&&pixels,"full-resolution source and target allocations");if(!source||!pixels)goto done;
 images.arena=source;images.arena_bytes=640*480;images.frame_stride=0;
 for(j=0;j<256;++j){images.indexed[j]=(unsigned char)(j*5+7);images.native[j]=0xaa000000UL+j*0x010203UL;}
 for(i=0;i<sizeof(full_cases)/sizeof(full_cases[0]);++i)for(j=0;j<3;++j) {
  const FullCase *f=full_cases+i;n=formats[j];c.width=f->width;c.height=f->height;images.pixel_bytes=n;
  for(y=0;y<(unsigned long)f->height;++y)for(x=0;x<(unsigned long)f->width;++x)source[y*f->width+x]=(unsigned char)(x+11*y+3);
  size=64+(f->width*n+3)*f->height;t.base=pixels;t.bytes=size;t.row0=32;t.pitch=f->width*n+3;
  memset(pixels,0xcd,size);c.writes=0;memcpy(&from,&f->from,4);memcpy(&to,&f->to,4);
  CHECK(functions[f->kind](&c,&t,from,to),"complete320/640 wedge succeeds with sequential PC53 fixture");
  if(c.writes!=f->writes||crc32(pixels,size)!=f->crc[j])printf("full fixture%lu kind%ld %ldx%ld bytes%lu writes%lu/%lu crc%08lx/%08lx\n",i,f->kind,f->width,f->height,n,c.writes,f->writes,crc32(pixels,size),f->crc[j]);
  CHECK(c.writes==f->writes&&crc32(pixels,size)==f->crc[j],"full-resolution nonbinary32/threshold footprint CRC and store count");
 }
done:
 if(source)free(source);if(pixels)free(pixels);
}
int __cdecl bh_loading_blit_checkpoint(unsigned long *count) {
 BHLoadingImages images;BHLoadingBlit context;BHLoadingBlitTarget target;BHMathEnvironment fp;
 unsigned char source[1600],saved[1600],actual[8192],expected[8192];
 unsigned long i,j,n,format,pad,negative,writes,value;long x,y,at,first,pitch;float from,to;
 checks=failures=0;memset(&images,0,sizeof(images));memset(&context,0,sizeof(context));memset(&target,0,sizeof(target));
 if(!bh_math_environment_enter(&fp))return 2;
 images.arena=source;images.arena_bytes=sizeof(source);images.frame_stride=800;context.images=&images;
 for(i=0;i<256;++i){images.indexed[i]=(unsigned char)(i*5+7);images.native[i]=0xaa000000UL+i*0x010203UL;}
 for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i)for(format=0;format<4;++format)for(pad=0;pad<2;++pad)for(negative=0;negative<2;++negative) {
  const Case *c=cases+i;context.width=c->width;context.height=c->height;images.display_width=c->width;
  n=format+1;images.pixel_bytes=n;pitch=c->width*n+pad;target.pitch=negative?-pitch:pitch;target.row0=64+(negative?(c->height-1)*pitch:0);
  target.base=actual;target.bytes=sizeof(actual);memset(actual,0xcd,sizeof(actual));memcpy(expected,actual,sizeof(actual));
  memset(source,0xee,sizeof(source));for(y=0;y<c->height;++y)for(x=0;x<c->width;++x)source[800+y*c->width+x]=(unsigned char)(x+11*y+3);
  memcpy(saved,source,sizeof(source));writes=0;
  if(n!=3) {
   if(c->kind==1||c->kind==3) {
    for(j=0;j<(unsigned long)(c->width/2);++j){x=c->kind==1?c->width/2+j:c->width/2-j-1;first=-1;
     for(y=0;y<c->height;++y)if(c->rows[y]&(1UL<<x)){if(first<0)first=y;at=target.row0+first*target.pitch+x*n+(y-first)*(target.pitch/(long)n)*(long)n;value=expected_pixel(&images,x,y,c->width);expected_store(expected,at,value,n);++writes;}}
   } else for(y=0;y<c->height;++y)for(x=0;x<c->width;++x)if(c->rows[y]&(1UL<<x)){at=target.row0+y*target.pitch+x*n;expected_store(expected,at,expected_pixel(&images,x,y,c->width),n);++writes;}
  }
  memcpy(&from,&c->from,4);memcpy(&to,&c->to,4);context.writes=0;
  CHECK(functions[c->kind](&context,&target,from,to),"complete wedge owner succeeds on bounded source/target");
  if(memcmp(actual,expected,sizeof(actual))){if(failures<12)printf("fixture%lu kind%ld dimensions%ldx%ld bytes%lu pad%lu negative%lu from%.9g to%.9g actualwrites%lu expected%lu\n",i,c->kind,c->width,c->height,n,pad,negative,from,to,context.writes,writes);}
  CHECK(!memcmp(actual,expected,sizeof(actual)),"all pixels and untouched padding match independent rational mask");
  CHECK(context.writes==writes&&!memcmp(source,saved,sizeof(source)),"exact store count and source arena retained");
 }
 context.width=8;context.height=6;images.pixel_bytes=2;target.pitch=16;target.row0=0;target.bytes=sizeof(actual);
 CHECK(!bh_loading_blit_top(NULL,&target,0,.125f),"NULL context guard");
 images.arena_bytes=800+47;CHECK(!bh_loading_blit_top(&context,&target,0,.125f),"short second page rejected");images.arena_bytes=sizeof(source);
 target.bytes=1;CHECK(!bh_loading_blit_top(&context,&target,0,.125f),"short actual target rejected");target.bytes=sizeof(actual);
 i=0x7fc12345;memcpy(&from,&i,4);CHECK(!bh_loading_blit_top(&context,&target,from,1),"nonfinite conversion source guard");
 context.width=0;CHECK(!bh_loading_blit_top(&context,&target,0,1),"invalid display geometry guard");
 guarded_spans();
 full_resolution();
 bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"loading-blit\",\"checks\":%lu,\"failures\":%lu,\"fixtures\":100,\"full_resolution_fixtures\":40,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_LOADING_BLIT_STANDALONE
int main(void){unsigned long n;return bh_loading_blit_checkpoint(&n);}
#endif
