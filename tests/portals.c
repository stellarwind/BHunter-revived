#include "../src/bh_portals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "portal_cases.h"

static unsigned long checks;
static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line) {
    ++checks;
    if (!ok) {++failures;if(failures<16)printf("FAIL portal line %d: %s\n",line,label);}
}
static void init(BHPortalNode *n,unsigned long count) {
    unsigned long i;
    memset(n,0,sizeof(*n)*count);
    for(i=0;i<count;++i) {n[i].next=&n[(i+1)%count];n[i].count=1;}
}
static void port(BHPortal *p,unsigned long direction,long plane,float lo,float hi) {
    unsigned long i;
    memset(p,0,sizeof(*p));p->direction=direction;p->plane=plane;
    p->aperture[0]=p->aperture[2]=lo;p->aperture[1]=p->aperture[3]=hi;
    for(i=0;i<4;++i)p->overlap[i]=p->scratch[i]=p->aperture[i];
}
static void edge_cases(void) {
    BHPortalNode n[3],saved[3];BHPortal *p=&n[0].portals[0],*q=&n[1].portals[0];
    unsigned long axis,bits;
    CHECK(sizeof(BHPortal)==60 && offsetof(BHPortal,neighbor)==56,"original portal layout");
    for(axis=0;axis<3;++axis) {
        init(n,3);port(p,axis,-17,0,10);port(q,axis^4,-17,2,12);
        port(&n[2].portals[0],axis^4,-17,3,7);
        CHECK(bh_portals_match(n,3) && p->neighbor==n+1,"first scene node wins");
        CHECK(p->overlap[0]==2 && p->overlap[1]==10 && p->overlap[2]==2 && p->overlap[3]==10,"aperture intersection");
        CHECK(!q->neighbor,"matching does not write reciprocal portal");
        CHECK(!memcmp(p->scratch,p->overlap,16),"scratch initialized from clipped extents");
        CHECK(bh_portals_match(n+1,3) && q->neighbor==n,"opposite signs match independently");
    }
    init(n,3);port(p,0,5,0,10);port(q,4,6,0,10);port(&n[2].portals[0],0,5,0,10);
    memcpy(saved,n,sizeof(n));CHECK(bh_portals_match(n,3) && !memcmp(saved,n,sizeof(n)),"wrong plane and direction reject without writes");
    q->plane=5;q->aperture[0]=9.9f;
    CHECK(bh_portals_match(n,3) && p->neighbor==n+1,"float boundary below double 9.9 accepted");
    port(p,0,5,0,10);q->aperture[0]=9.900001f;
    CHECK(bh_portals_match(n,3) && !p->neighbor,"float boundary above double 9.9 rejected");
    port(p,0,5,0,10);q->aperture[0]=0;q->aperture[1]=0.1f;
    CHECK(bh_portals_match(n,3) && p->neighbor==n+1,"float 0.1 above exact double threshold accepted");
    port(p,0,5,0,10);q->aperture[1]=0.09999999f;
    CHECK(bh_portals_match(n,3) && !p->neighbor,"below low-side threshold rejected");
    port(p,0,5,0,10);port(q,4,5,10,20);CHECK(bh_portals_match(n,3) && !p->neighbor,"touching edges do not connect");
    port(q,4,5,0,10);q->aperture[2]=10;q->aperture[3]=20;
    CHECK(bh_portals_match(n,3) && !p->neighbor,"second projected dimension independently rejects");
    port(q,4,5,0,10);p->neighbor=n+2;p->scratch[0]=7;
    memcpy(saved,n,sizeof(n));CHECK(bh_portals_match(n,3) && !memcmp(saved,n,sizeof(n)),"prelinked portal completely untouched");
    p->neighbor=NULL;n[1].count=2;port(&n[1].portals[1],4,5,2,8);
    CHECK(bh_portals_match(n,3) && p->neighbor==n+1 && p->overlap[0]==0,"first matching local slot wins");
    port(p,0,5,0,10);port(q,4,5,2,12);p->overlap[0]=4;p->overlap[1]=8;
    CHECK(bh_portals_match(n,3) && p->overlap[0]==4 && p->overlap[1]==8,"existing clipped interval is further intersected, not reset");
    port(p,0,5,0,10);port(q,4,5,-1,12);bits=0x80000000UL;
    memcpy(p->overlap,&bits,4);CHECK(bh_portals_match(n,3),"signed zero fixture matches");
    memcpy(&bits,p->scratch,4);CHECK(bits==0x80000000UL,"equal clipping boundary preserves signed-zero bits");
    init(n,1);port(p,0,5,0,10);memcpy(saved,n,sizeof(n));
    CHECK(bh_portals_match(n,1) && !memcmp(n,saved,sizeof(n)),"singleton never matches itself");
    n[0].count=0;CHECK(bh_portals_match(n,1),"zero-port node");
    CHECK(!bh_portals_match(NULL,1) && !bh_portals_match(n,0) && !bh_portals_match(n,65536UL),"argument limits");
    init(n,3);port(p,0,5,0,10);port(q,4,5,0,10);port(&n[2].portals[0],4,5,0,10);
    n[2].count=7;memcpy(saved,n,sizeof(n));
    CHECK(!bh_portals_match(n,3) && !memcmp(n,saved,sizeof(n)),"late invalid count rejected before any mutation");
    n[2].count=1;n[2].next=n+1;memcpy(saved,n,sizeof(n));
    CHECK(!bh_portals_match(n,3) && !memcmp(n,saved,sizeof(n)),"ring not returning to start rejected");
    n[2].next=n;n[1].next=NULL;CHECK(!bh_portals_match(n,3),"null successor rejected");
    n[1].next=n+2;CHECK(!bh_portals_match(n,4) && !bh_portals_match(n,2),"early and late ring closure rejected");
    bits=0x7f800001UL;memcpy(&n[2].portals[0].aperture[0],&bits,4);memcpy(saved,n,sizeof(n));
    CHECK(!bh_portals_match(n,3) && !memcmp(n,saved,sizeof(n)),"nonfinite late aperture rejected without partial match");
    port(&n[2].portals[0],4,5,0,10);n[2].portals[0].direction=3;CHECK(!bh_portals_match(n,3),"non-axis direction rejected");
}
static void captured_graph(void) {
    BHPortalNode *nodes;BHPortal *p;const unsigned long *row;
    unsigned long i,j,neighbor,connected=0;
    nodes=(BHPortalNode*)calloc(1454,sizeof(*nodes));
    CHECK(nodes!=NULL,"real component storage allocation");if(!nodes)return;
    for(i=0;i<1454;++i) {nodes[i].count=portal_counts[i];nodes[i].next=&nodes[(i+1)%1454];}
    for(i=0;i<3739;++i) {
        row=portal_cases[i];p=&nodes[row[0]].portals[row[1]];
        memcpy(p,row+2,24);memcpy(p->overlap,p->aperture,16);memcpy(p->scratch,p->aperture,16);
    }
    for(i=0;i<1454;++i)CHECK(bh_portals_match(nodes+i,1454),"captured scene matcher succeeds");
    for(i=0;i<3739;++i) {
        row=portal_cases[i];p=&nodes[row[0]].portals[row[1]];
        neighbor=p->neighbor ? (unsigned long)(p->neighbor-nodes) : 0xffffffffUL;
        CHECK(neighbor==row[16],"captured neighbor index");if(p->neighbor)++connected;
        for(j=0;j<14;++j) {unsigned long word;memcpy(&word,(unsigned char*)p+j*4,4);CHECK(word==row[j+2],"captured portal field bits");}
    }
    CHECK(connected==3120,"all captured directed connections recovered");
    free(nodes);
}
int bh_portals_checkpoint(unsigned long *count) {
    checks=0;failures=0;edge_cases();captured_graph();*count=checks;
    printf("{\"scenario\":\"portal-matching\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures ? 1 : 0;
}
