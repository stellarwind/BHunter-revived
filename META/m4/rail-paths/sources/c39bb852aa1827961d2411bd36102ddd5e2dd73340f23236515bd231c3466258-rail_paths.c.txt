#include "../src/bh_rail_paths.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<20)printf("FAIL rail-paths: %s\n",s);}}while(0)
typedef struct Memory {unsigned long live,calls,deny,released;} Memory;
static void *__cdecl allocate(unsigned long n,void *u){Memory *m=(Memory*)u;unsigned long *p;++m->calls;if(m->deny)return NULL;
    p=(unsigned long*)malloc(n+8);if(!p)return NULL;p[0]=0x12345678;p[n/4+1]=0x87654321;++m->live;return p+1;}
static void __cdecl release(void *v,void *u){Memory *m=(Memory*)u;unsigned long *p=(unsigned long*)v-1;
    CHECK(m->live&&p[0]==0x12345678&&p[2201]==0x87654321,"whole200*44-byte owning allocation guards remain intact");--m->live;++m->released;free(p);}
static BHVec3 v(float x,float y,float z){BHVec3 a;a.x=x;a.y=y;a.z=z;return a;}
static int equal(const BHVec3 *a,const BHVec3 *b){return a->x==b->x&&a->y==b->y&&a->z==b->z;}
static void add(BHRailPaths *p,float a,float b){BHVec3 start=v(a,0,0),end=v(b,0,0),zero=v(0,0,0);CHECK(bh_rail_add(p,&start,&end,&zero,&zero),"actual whole segment registration");}
static void numerical(void){BHVec3 zero=v(0,0,0),point=v(0.5f,0,0);unsigned long bits;float nan;
    CHECK(!bh_rail_near(&point,&zero),"strict exact squared-distance threshold equality excluded");
    bits=0x3effffff;memcpy(&point.x,&bits,4);CHECK(bh_rail_near(&point,&zero),"nearest binary32 value below radius accepted");
    bits=0x3f000001;memcpy(&point.x,&bits,4);CHECK(!bh_rail_near(&point,&zero),"nearest binary32 value above radius excluded");
    bits=0x7fc00000;memcpy(&nan,&bits,4);point.x=nan;CHECK(bh_rail_near(&point,&zero),"raw unordered comparison sets nearness true");
    {BHVec3 a=v(0.5f,0.0001f,0),b=v(0.00000001490116119384765625f,0,0);double dx=(double)a.x-b.x;
        CHECK(dx*dx+(double)a.y*a.y<0.25&&!bh_rail_near(&a,&b),"positive discriminator: original X float-spill product rejects a pair that naive Euclidean distance accepts");
        a=v(0,0.0001f,0.5f);b=v(0,0,0.00000001490116119384765625f);
        CHECK(!bh_rail_near(&a,&b),"the independent Z difference also retains its original float-spill product");}
    CHECK(!bh_rail_near(NULL,&zero),"source null-vector guard");
}
static void placement_modes(void){
    /* Independently transcribed original endpoint stores, including duplicate
       modes4/6 and multi-piece7..9; tests also exercise transformed real LEVs. */
    static const float ends[16][6]={
        {5,0,0,5,0,10},{5,0,0,5,0,20},{5,-10,0,5,0,20},
        {5,0,0,5,-10,10},{5,-20,10,5,0,20},{5,0,0,5,-20,0},{5,-20,10,5,0,20},
        {5,0,0,10,-6.66f,18},{10,-6.66f,18,22,-13.33f,30},{22,-13.33f,30,40,-20,35},
        {35,0,0,30,-6.66f,18},{30,-6.66f,18,18,-13.33f,30},{18,-13.33f,30,0,-20,35},
        {5,0,0,10,0,18},{10,0,18,22,0,30},{22,0,30,40,0,35}};
    BHRailPaths p;BHVec3 position=v(100,200,-300),rotation=v(0,123,456),a,b;char name[16];long digit,j,at=0;unsigned long axis;
    CHECK(bh_rail_paths_init(&p,NULL),"actual fresh200-slot rail owner");
    for(digit=0;digit<10;++digit){sprintf(name,"rC_%ld.bsp",digit);CHECK(bh_rail_placement(&p,name,&position,&rotation),"case-insensitive literal placement dispatch");
        for(j=0;j<(digit<7?1:3);++j,++at){a=v(ends[at][0]+100,ends[at][1]+200,ends[at][2]-300);b=v(ends[at][3]+100,ends[at][4]+200,ends[at][5]-300);
            CHECK(equal(&p.segments[at].start,&a)&&equal(&p.segments[at].end,&b),"every literal endpoint, multi-piece order and translation matches original stores");
            CHECK(!p.segments[at].next&&!p.segments[at].previous&&!p.segments[at].visited,"registered linkage reset without guessed callbacks");}}
    CHECK(p.count==16,"all ten RC modes expand into original sixteen segments");
    CHECK(bh_rail_placement(&p,"RC_X",&position,&rotation)&&bh_rail_placement(&p,"XX_0",&position,&rotation)&&p.count==16,"non-digit and unrelated placements do not register geometry");
    bh_rail_paths_dispose(&p);
    CHECK(bh_rail_paths_init(&p,NULL),"new owner for yaw-only transformation");
    for(axis=0;axis<4;++axis){BHVec3 start=v(3,4,5),end=v(-2,8,7),zero=v(0,0,0);double angle=axis*1.57079632679489661923;rotation=v((float)angle,9,11);
        CHECK(bh_rail_add(&p,&start,&end,&zero,&rotation),"whole transform registration at real LEV quarter-turn angles");
        CHECK(fabs(p.segments[axis].start.x-(axis==0?3:axis==1?5:axis==2?-3:-5))<0.00001&&
              fabs(p.segments[axis].start.z-(axis==0?5:axis==1?-3:axis==2?-5:3))<0.00001&&p.segments[axis].start.y==4,
              "independent quarter-turn geometry confirms correct sign and yaw-only orientation");}
    bh_rail_paths_dispose(&p);
}
static void chains(void){BHRailPaths p;BHRailSegment *s;BHVec3 zero=v(0,0,0),a,b;
    CHECK(bh_rail_paths_init(&p,NULL),"fresh owner for chain swap and pointer repair");
    add(&p,0,10);add(&p,10,20);add(&p,100,110);add(&p,20,0);
    CHECK(bh_rail_link(&p)&&p.chain_count==2,"closed three-segment cycle plus disconnected open chain");s=p.segments;
    CHECK(s[0].next==s+2&&s[2].previous==s&&s[2].next==s+3&&s[3].previous==s+2&&s[3].next==s&&s[0].previous==s+3,
        "moving a new chain head into prefix repairs both neighbors of displaced old chain member");
    CHECK(s[1].start.x==100&&!s[1].next&&!s[1].previous&&s[2].start.x==10,"head prefix ordering and displaced segment identity retained");
    CHECK(s[0].length==10&&s[1].length==10&&s[2].length==10&&s[3].length==20&&s[3].rise==0,"whole final metric pass covers every swapped segment");
    CHECK(bh_rail_link(&p)&&p.chain_count==2,"repeated link leaves already visited chain count stable");bh_rail_paths_dispose(&p);
    CHECK(bh_rail_paths_init(&p,NULL),"fresh reversal owner");add(&p,0,10);add(&p,20,10);
    CHECK(bh_rail_link(&p)&&p.chain_count==1&&p.segments[1].start.x==10&&p.segments[1].end.x==20&&p.segments[0].next==p.segments+1,"reverse endpoint match reverses geometry before linking");bh_rail_paths_dispose(&p);
    CHECK(bh_rail_paths_init(&p,NULL),"fresh duplicate owner");add(&p,0,10);add(&p,10,0);
    CHECK(bh_rail_link(&p)&&p.chain_count==2&&!p.segments[0].next&&!p.segments[1].next,"opposite duplicate endpoints do not become a false two-segment cycle");bh_rail_paths_dispose(&p);
    CHECK(bh_rail_paths_init(&p,NULL),"fresh rise metric owner");a=v(1,2,3);b=v(4,6,15);
    CHECK(bh_rail_add(&p,&a,&b,&zero,&zero)&&bh_rail_link(&p)&&p.segments[0].rise==4&&p.segments[0].length==13,"3-4-12 analytic length and signed rise independent control");
    p.segments[0].next=(BHRailSegment*)1;CHECK(!bh_rail_link(&p),"invalid borrowed link rejected before dereference");bh_rail_paths_dispose(&p);
}
static void lifetime(void){Memory m={0,0,0,0};BHSceneAllocator allocator={allocate,release,&m};BHRailPaths p;BHVec3 zero=v(0,0,0);long i;
    CHECK(bh_rail_paths_init(&p,&allocator)&&m.live==1,"actual borrowed allocator creates one stable owner");p.segments[0].rise=23;p.segments[0].length=29;
    CHECK(bh_rail_add(&p,&zero,&zero,&zero,&zero)&&p.segments[0].rise==23&&p.segments[0].length==29,"registration preserves deferred rise/length fields");
    for(i=1;i<199;++i)bh_rail_add(&p,&zero,&zero,&zero,&zero);
    CHECK(bh_rail_placement(&p,"RC_7",&zero,&zero)&&p.count==200&&p.dropped==2&&p.additions==200,"three-part placement at199 retains first part and silently drops two at exact original capacity");
    bh_rail_paths_dispose(&p);bh_rail_paths_dispose(&p);CHECK(!m.live&&m.released==1,"owning allocation released exactly once");m.deny=1;
    CHECK(!bh_rail_paths_init(&p,&allocator)&&!p.segments&&!m.live,"real allocator rejection leaves no live segment storage");bh_rail_paths_dispose(&p);
}
static void real_level(const char *archive){BHRailPaths p;BHSceneLevel level;BHVFile *file=NULL;unsigned long i,n=0,recognized=0;
    BHVec3 position,rotation;BHRailSegment *cursor;unsigned char seen[200];int complete=1;
    CHECK(bh_rail_paths_init(&p,NULL)&&bh_scene_level_init(&level,NULL),"real world-data owners initialize separately from entity construction");
    CHECK(bh_vfile_mount(archive),"immutable archive mounted as data only");file=bh_vfile_open("TEST.LEV","rb");
    CHECK(file&&bh_scene_level_read(&level,file),"admitted LEV reader provides actual placement records");if(file)bh_vfile_close(file);
    for(i=0;i<level.placement_count;++i){BHScenePlacement *s=level.placements+i;
        position=v((float)s->x,(float)s->y,(float)s->z);rotation=v((float)((s->flags&3)*1.57079632679489661923),0,0);
        if(!_strnicmp(s->name,"RC_",3)){++n;if(s->name[3]>='0'&&s->name[3]<='9')++recognized;}
        CHECK(bh_rail_placement(&p,s->name,&position,&rotation),"whole RC placement consumer borrows every real decoded placement");}
    CHECK(level.placement_count==5815&&n==119&&recognized==76&&p.count==132&&!p.dropped,
        "independent raw LEV census pins76 digit placements,43 RC_P no-ops and132 resulting segments");
    CHECK(bh_rail_link(&p)&&p.chain_count==1,"real transformed rail geometry forms one original first-match chain without world/VM callback fabrication");
    for(i=0;i<(unsigned long)p.count;++i)CHECK(p.segments[i].visited&&p.segments[i].length>=0,"real complete metric/link pass covers all registered segments");
    memset(seen,0,sizeof(seen));cursor=p.segments;
    for(i=0;i<132;++i){long at;if(!cursor){complete=0;break;}at=(long)(cursor-p.segments);
        if(at<0||at>=132||seen[at]||!cursor->next||cursor->next->previous!=cursor||!bh_rail_near(&cursor->end,&cursor->next->start)){complete=0;break;}
        seen[at]=1;cursor=cursor->next;
    }
    CHECK(complete&&i==132&&cursor==p.segments,"actual real rail loop traverses every segment exactly once with reciprocal links and matching geometric endpoints");
    printf("{\"scenario\":\"rail-paths-real\",\"placements\":%lu,\"segments\":%ld,\"chains\":%ld}\n",n,p.count,p.chain_count);
    bh_scene_level_dispose(&level);bh_rail_paths_dispose(&p);CHECK(bh_vfile_detach_archive(),"fixture detaches its owned archive");bh_vfile_shutdown();
}
int bh_rail_paths_checkpoint(const char *archive,unsigned long *count){BHMathEnvironment environment;int entered;checks=failures=0;
    entered=bh_math_environment_enter(&environment);CHECK(entered,"admitted PC53 nearest masked source scope");
    if(entered){numerical();placement_modes();chains();lifetime();if(archive)real_level(archive);bh_math_environment_leave(&environment);}
    if(count)*count=checks;printf("{\"scenario\":\"rail-paths\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_RAIL_PATHS_STANDALONE
int main(int argc,char **argv){return bh_rail_paths_checkpoint(argc>1?argv[1]:NULL,NULL);}
#endif
