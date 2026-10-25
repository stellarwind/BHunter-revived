#include "../src/bh_clipper.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL clipper %d: %s\n",line,label);}}
static int close_value(double a,double b){return fabs(a-b)<=2e-4*(1+fabs(b));}
static void ring(BHClipVertex *v,const float values[][5],unsigned long n) {
    unsigned long i;memset(v,0,n*sizeof(*v));for(i=0;i<n;++i){v[i].point.x=values[i][0];v[i].point.y=values[i][1];v[i].point.z=values[i][2];v[i].u=values[i][3];v[i].v=values[i][4];v[i].next=v+(i+1)%n;v[i].previous=v+(i+n-1)%n;v[i].frame=19;}
}
static unsigned long size(BHClipVertex *p) {
    BHClipVertex *head=p;unsigned long n=0;if(!p)return 0;do{if(!p->next || !p->previous || p->next->previous!=p || p->previous->next!=p || ++n>1036)return 0;p=p->next;}while(p!=head);return n;
}
static void basic(void) {
    BHClipContext c;BHClipVertex v[4],*out;unsigned long i;const float square[4][5]={{-1,-1,2,0,0},{1,-1,2,1,0},{1,1,2,1,1},{-1,1,2,0,1}};
    CHECK(bh_clip_init(&c,1,8,-1,1,-1,1),"explicit valid clip context");ring(v,square,4);out=bh_clip_polygon(&c,v,NULL);
    CHECK(out==v && !c.used && size(out)==4 && c.error==BH_CLIP_OK,"inside ring retains original head/topology and no scratch");
    CHECK(v[0].sx==-.5f && v[0].sy==-.5f && v[2].sx==.5f && v[2].sy==.5f && v[0].projected==1 && v[0].frame==19,"one-time divide caches screen coordinates without altering vertex frame");
    v[0].point.x=1;out=bh_clip_polygon(&c,v,NULL);CHECK(out==v && v[0].sx==-.5f,"existing projected flag retains cache rather than redividing changed input");
    for(i=0;i<6;++i) {
        ring(v,square,4);
        {unsigned long j;for(j=0;j<4;++j){if(i==0)v[j].point.z=.5f;else if(i==1)v[j].point.z=9;else if(i==2)v[j].point.x=-9;else if(i==3)v[j].point.x=9;else if(i==4)v[j].point.y=-9;else v[j].point.y=9;}}
        CHECK(!bh_clip_polygon(&c,v,NULL) && c.error==BH_CLIP_OK && !c.used,"all six trivial reject planes return NULL without an error or scratch insertion");
    }
    ring(v,square,4);for(i=0;i<4;++i){v[i].point.x*=2;v[i].point.y*=2;v[i].point.z=2;}
    CHECK(bh_clip_polygon(&c,v,NULL)==v && !c.used,"exact viewport equality remains inside without invented epsilon/deduplication");
    ring(v,square,4);for(i=0;i<4;++i)v[i].point.z=1;CHECK(bh_clip_polygon(&c,v,NULL)==v && !c.used,"exact near-plane equality retained");
    ring(v,square,4);for(i=0;i<4;++i)v[i].point.z=8;CHECK(bh_clip_polygon(&c,v,NULL)==v && !c.used,"exact far-plane equality retained");
    {BHClipVertex *large=(BHClipVertex *)calloc(1025,sizeof(*large));unsigned long j;
     CHECK(large!=NULL,"bounded ring capacity fixture allocation");if(large) {
      for(j=0;j<1024;++j){large[j].point.z=2;large[j].next=large+(j+1)%1024;large[j].previous=large+(j+1023)%1024;}
      CHECK(bh_clip_polygon(&c,large,NULL)==large && size(large)==1024,"source guard accepts full1024-node input ring");
      for(j=0;j<1025;++j){large[j].point.z=2;large[j].projected=0;large[j].next=large+(j+1)%1025;large[j].previous=large+(j+1024)%1025;}
      CHECK(!bh_clip_polygon(&c,large,NULL) && c.error==BH_CLIP_TOPOLOGY && !large[0].projected,"oversize ring rejects before projecting or modifying links");free(large);}}
    ring(v,square,4);v[2].previous=v;CHECK(!bh_clip_polygon(&c,v,NULL) && c.error==BH_CLIP_TOPOLOGY,"inconsistent source links reject before mutation");
    ring(v,square,4);{unsigned long nan=0x7fc00001UL;memcpy(&v[1].point.z,&nan,4);}CHECK(!bh_clip_polygon(&c,v,NULL) && c.error==BH_CLIP_NUMERICAL,"nonfinite source ring rejects");
}
static void intersections(void) {
    BHClipContext c;BHClipVertex v[3],*out;unsigned long axis,side;
    const float near_values[3][5]={{0,0,.5f,0,0},{2,0,2,1,0},{0,2,2,0,1}};
    const float far_values[3][5]={{0,0,10,0,0},{2,0,2,1,0},{0,2,2,0,1}};
    bh_clip_init(&c,1,8,-1,1,-1,1);ring(v,near_values,3);out=bh_clip_polygon(&c,v,NULL);
    CHECK(out==c.scratch && c.used==2 && size(out)==4 && out->next==c.scratch+1 && out->next->next==v+1 && out->previous==v+2,"near outside chain replaced by paired scratch nodes with exact returned head/order");
    CHECK(out && out->point.z==1 && close_value(out->point.y,2.0/3) && close_value(out->v,1.0/3) && out->u==0 && out->projected==1,"near intersection linearly interpolates camera position/UV then projects once");
    CHECK(out && out->next->point.z==1 && close_value(out->next->point.x,2.0/3) && close_value(out->next->u,1.0/3),"second near intersection preserves forward ring winding");
    ring(v,far_values,3);out=bh_clip_polygon(&c,v,NULL);
    CHECK(out==c.scratch && c.used==2 && size(out)==4 && out->point.z==8 && out->point.y==.5f && out->v==.25f && out->sy==.0625f,"far depth intersection uses original linear depth contract");
    for(axis=0;axis<2;++axis)for(side=0;side<2;++side) {
        float values[3][5]={{-4,0,2,0,0},{0,0,4,1,0},{0,2,4,1,1}};float sign=side?-1.0f:1.0f;
        unsigned long i;
        for(i=0;i<3;++i){values[i][0]*=sign;if(axis){float t=values[i][0];values[i][0]=values[i][1];values[i][1]=t;}}
        ring(v,values,3);out=bh_clip_polygon(&c,v,NULL);
        CHECK(out==c.scratch && c.used==2 && size(out)==4 && out->next==c.scratch+1 && out->next->next==v+1,"all four screen cuts retain paired ring insertion order");
        CHECK(out && close_value(out->point.z,8.0/3) && close_value(out->u,1.0/3) && close_value(out->v,1.0/3) && !out->projected,
            "screen intersection interpolates reciprocal depth and U/Z,V/Z; rejects affine UV=one-half; new side cache flag stays zero");
        CHECK(out && (axis?out->sy:out->sx)==-sign && (axis?out->sx:out->sy)==.25f && out->next->v==0,
            "screen boundary is exact and other screen coordinate interpolates linearly");
    }
    {const float boundary[3][5]={{-4,0,2,0,0},{-2,0,2,1,0},{0,1,2,0,1}};ring(v,boundary,3);out=bh_clip_polygon(&c,v,NULL);
     CHECK(out && size(out)==4 && out->next->sx==v[1].sx && out->next->sy==v[1].sy && out->next!=v+1,"boundary endpoint duplicate is preserved as original rather than deduplicated");}
}
static void projected_texture(void) {
    BHClipContext c;BHClipVertex v[3],*out;BHTextureImage image;BHMat34 camera={{{2,1,0,3},{0,4,2,-2},{0,0,.5f,5}}};
    BHVec3 world={7,-3,2};float coefficients[6]={2,.5f,.25f,-.25f,4,-.5f};unsigned long i;
    const float values[3][5]={{0,0,2,0,0},{1,0,2,1,0},{0,1,2,0,1}};
    memset(&image,0,sizeof(image));image.unused54=1;memcpy(image.padding,coefficients,sizeof(coefficients));bh_clip_init(&c,1,20,-100,100,-100,100);bh_clip_set_camera(&c,&camera);
    ring(v,values,3);for(i=0;i<3;++i){bh_mat34_transform_point(&camera,&world,&v[i].point);world.x+=1;}
    out=bh_clip_polygon(&c,v,&image);
    CHECK(out==v && c.inverse_ready && v[0].u==15.25f && v[0].v==5.75f && v[1].u==17.25f,"actual image metadata drives inverse-camera world X/Z projected UV coefficients");
    ring(v,values,3);CHECK(bh_clip_polygon(&c,v,&image)==v && c.inverse_ready,"projected texture inverse remains cached across polygon calls");
    camera.m[0][3]+=2;CHECK(bh_clip_set_camera(&c,&camera) && !c.inverse_ready,"explicit next-frame camera binding invalidates general inverse cache");
    ring(v,values,3);CHECK(bh_clip_polygon(&c,v,&image)==v && c.inverse_ready,"next projected polygon reconstructs inverse from real recovered owner");
    memset(&camera,0,sizeof(camera));bh_clip_set_camera(&c,&camera);ring(v,values,3);
    CHECK(!bh_clip_polygon(&c,v,&image) && c.error==BH_CLIP_NUMERICAL && !c.inverse_ready,"invalid projected-texture camera rejects rather than using an invented inverse");
}
typedef struct Reference {double x,y,z,u,v;} Reference;
static double distance(const Reference *p,unsigned long side) {
    switch(side){case 0:return p->z-1;case 1:return 8-p->z;case 2:return p->x+p->z;case 3:return p->z-p->x;case 4:return p->y+p->z;default:return p->z-p->y;}
}
/* Independent homogeneous Sutherland-Hodgman oracle, no vertex links, cached
   projection, reciprocal-depth interpolation or calls into source clipper. */
static unsigned long reference(const float input[3][5],Reference result[32]) {
    Reference a[32],b[32],*p,*q;unsigned long i,side,n=3,count;double da,db,t;int k;
    for(i=0;i<3;++i)for(k=0;k<5;++k)((double *)(a+i))[k]=input[i][k];
    for(side=0;side<6 && n;++side) {
        count=0;for(i=0;i<n;++i){p=a+i;q=a+(i+1)%n;da=distance(p,side);db=distance(q,side);
            if(da>=0)b[count++]=*p;
            if((da>=0)!=(db>=0)){t=da/(da-db);for(k=0;k<5;++k)((double *)(b+count))[k]=((double *)p)[k]+t*(((double *)q)[k]-((double *)p)[k]);++count;}}
        n=count;memcpy(a,b,n*sizeof(*a));
    }
    memcpy(result,a,n*sizeof(*a));return n;
}
static void varied(void) {
    BHClipContext c;BHClipVertex v[3],*out,*p;Reference expected[32];float input[3][5];unsigned long seed=0x81373UL,n,i,j,k,actual,want,mask,matched,max_scratch=0;int same;
    bh_clip_init(&c,1,8,-1,1,-1,1);
    {const float full[3][5]={{8.375f,-6.5f,3.25f,-10,9.875f},{-9.375f,-2.75f,-8.625f,-1.875f,2},{-3.5f,8.375f,9.5f,-2.625f,-6.875f}};
     ring(v,full,3);want=reference(full,expected);out=bh_clip_polygon(&c,v,NULL);
     CHECK(out && c.error==BH_CLIP_OK && c.used==12 && size(out)==7 && want==7,"independent six-plane fixture fills all twelve original scratch slots with intact seven-node ring");
     p=out;mask=0;for(i=0;out && i<want;++i) {
      matched=0;for(k=0;k<want;++k)if(!(mask&(1UL<<k)) && close_value(p->sx,expected[k].x/expected[k].z) && close_value(p->sy,expected[k].y/expected[k].z) && close_value(p->point.z,expected[k].z) && close_value(p->u,expected[k].u) && close_value(p->v,expected[k].v)){mask|=1UL<<k;matched=1;break;}
      CHECK(matched,"full scratch-capacity result matches independently clipped position/depth/UV");p=p->next;
     }}
    for(n=0;n<256;++n) {
        for(i=0;i<3;++i)for(j=0;j<5;++j){seed=seed*1664525UL+1013904223UL;input[i][j]=(float)((long)((seed>>16)%161)-80)*.125f;}
        /* Z spans near/far and negative behind-camera values; input triangle
           is always convex. Oracle clips it directly in homogeneous space. */
        ring(v,input,3);want=reference(input,expected);out=bh_clip_polygon(&c,v,NULL);actual=size(out);
        CHECK(c.error==BH_CLIP_OK && actual==want,"varied convex triangles match independent homogeneous clipping topology");
        CHECK(c.used<=12 && !(c.used&1),"all six cuts stay within original twelve scratch records");if(max_scratch<c.used)max_scratch=c.used;
        if(actual && actual==want) {
            p=out;mask=0;do {
                matched=0;for(k=0;k<want;++k)if(!(mask&(1UL<<k))) {
                    same=close_value(p->sx,expected[k].x/expected[k].z) && close_value(p->sy,expected[k].y/expected[k].z) &&
                        close_value(p->point.z,expected[k].z) && close_value(p->u,expected[k].u) && close_value(p->v,expected[k].v);
                    if(same){mask|=1UL<<k;matched=1;break;}
                }
                CHECK(matched,"every linked-ring intersection matches independent position/depth/UV geometry");p=p->next;
            }while(p!=out);
        }
    }
    printf("{\"clipper_oracle_cases\":256,\"maximum_scratch_vertices\":%lu}\n",max_scratch);
    CHECK(max_scratch>=8,"varied controls exercise multiple depth/viewport scratch pairs");
}
int bh_clipper_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    basic();intersections();projected_texture();varied();CHECK(bh_math_environment_supported(),"clip/project owner retains required PC53 caller environment");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"polygon-clipper\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
