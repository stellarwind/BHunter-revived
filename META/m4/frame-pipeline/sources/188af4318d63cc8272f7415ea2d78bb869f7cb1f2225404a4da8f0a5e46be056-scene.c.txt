/* Independent scene fixtures and real-data comparisons; no original code. */
#include "../src/bh_scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static unsigned long checks;
static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line) {
    ++checks;if(!ok){++failures;printf("FAIL scene line %d: %s\n",line,label);}
}
static void put32(unsigned char *p,unsigned long v) {
    p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);
}
static unsigned long get32(const unsigned char *p) {
    return (unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);
}
static unsigned long crc32(const unsigned char *p,unsigned long size) {
    unsigned long crc=0xffffffffUL,i;int bit;
    for(i=0;i<size;i++){crc^=p[i];for(bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320UL& (0UL-(crc&1)));}
    return crc^0xffffffffUL;
}
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long size,void *user) {
    Memory *memory=(Memory*)user;unsigned char *p;
    ++memory->calls;if(memory->fail_at && memory->calls==memory->fail_at)return NULL;
    p=(unsigned char*)malloc(size+20);if(!p)return NULL;
    put32(p,size);put32(p+4,0x8a123456UL);put32(p+16+size,0x98172634UL);++memory->live;
    return p+16;
}
static void __cdecl release(void *pointer,void *user) {
    Memory *memory=(Memory*)user;unsigned char *p=(unsigned char*)pointer-16;
    CHECK(memory->live && get32(p+4)==0x8a123456UL && get32(p+16+get32(p))==0x98172634UL,"owned allocation guards and release");
    --memory->live;free(p);
}
static BHSceneAllocator hooks(Memory *memory) {
    BHSceneAllocator result;result.allocate=allocate;result.release=release;result.user=memory;return result;
}
static int write_file(const char *name,const void *bytes,unsigned long size) {
    FILE *file=fopen(name,"wb");int ok;
    if(!file)return 0;ok=fwrite(bytes,1,size,file)==size;return fclose(file)==0 && ok;
}

static unsigned char lev[108],pth[77];
static void fixture(void) {
    unsigned long i;unsigned char *p;
    memset(lev,0,sizeof(lev));put32(lev,2);
    strcpy((char*)lev+4,"KNOWN.BSP");put32(lev+20,0x80000000UL);put32(lev+24,30);put32(lev+28,0x7fffffffUL);put32(lev+32,0xf0000003UL);
    strcpy((char*)lev+36,"UNKNOWN.BSP");put32(lev+52,17);put32(lev+56,0xffffffe2UL);put32(lev+60,1410);put32(lev+64,0);
    put32(lev+68,1);lev[72]=0xce;lev[73]=0xff;lev[74]=0x0f;lev[75]=0x27; /* -50 and 9999: opaque, valid */
    put32(lev+88,1);lev[92]=42; /* duplicate owner retained, file order matters */
    put32(pth,3);
    for(i=0;i<9;i++) {p=pth+4+8*i;put32(p,i==0?0xffffffffUL:i%6);put32(p+4,i==1 ? 0x80000000UL:0x3f800000UL+i*0x1000);}
    put32(pth+8,0x7e876abdUL); /* original unreachable-distance sentinel */
    pth[76]=0x55;
}

static void memory_readers(void) {
    BHSceneLevel level;BHSceneRoutes routes;
    BHScenePlacement *old;BHSceneRoute *old_routes;
    unsigned long i,step,old_live;
    Memory memory={0,0,0};BHSceneAllocator allocator=hooks(&memory);
    unsigned char bad[108],empty[4]={0};
    SYSTEM_INFO system;unsigned char *pages,*boundary;unsigned long page;
    fixture();CHECK(bh_scene_level_init(&level,&allocator) && bh_scene_routes_init(&routes,&allocator),"initialize owning outputs");
    CHECK(bh_scene_level_decode(&level,lev,sizeof(lev)) && level.placement_count==2 && level.link_count==2,"decode placements and ordered link records");
    CHECK(level.placements[0].x==(-2147483647L-1) && level.placements[0].z==2147483647L && level.placements[0].flags==0xf0000003UL,"preserve signed coordinates and all flag bits");
    CHECK(!strcmp(level.placements[1].name,"UNKNOWN.BSP") && level.placements[1].y==-30,"unknown block names retained without fabricated entities");
    CHECK(level.links[0].values[0]==-50 && level.links[0].values[1]==9999 && level.links[1].placement==1 && level.links[1].values[0]==42,"opaque links and duplicate owners retained");
    CHECK(bh_scene_routes_decode(&routes,pth,76,3) && routes.node_count==3,"decode square route cache");
    CHECK(!memcmp(routes.cells,pth+4,72) && routes.cells[0].exit_slot==-1,"exact route distance bits, sentinel and local exits");
    CHECK(routes.cells[1].distance==0 && get32((const unsigned char*)&routes.cells[1].distance)==0x80000000UL,"negative zero distance preserved");
    CHECK(routes.cells[0].distance>1e30 && routes.cells[4].distance!=0 && routes.cells[1].distance!=routes.cells[3].distance,"do not normalize diagonal/symmetry or unreachable sentinel");
    old=level.placements;old_routes=routes.cells;old_live=memory.live;
    /* Put each short fixture against a no-access page: declared size is a real
       memory boundary, rather than a padded array that hides overreads. */
    GetSystemInfo(&system);page=system.dwPageSize;
    pages=(unsigned char*)VirtualAlloc(NULL,page*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    CHECK(pages!=NULL,"allocate guard-page fixture");
    if(pages) {
        unsigned long previous;
        CHECK(VirtualProtect(pages+page,page,PAGE_NOACCESS,&previous),"protect adjacent input page");boundary=pages+page;
        for(i=0;i<sizeof(lev);i++) {
            if(i==68 || i==88)continue; /* valid level with fewer whole tail records */
            memcpy(boundary-i,lev,i);
            CHECK(!bh_scene_level_decode(&level,boundary-i,i),"every partial level prefix rejected at real memory boundary");
            CHECK(level.placements==old && memory.live==old_live,"level rejection preserves old owner");
        }
        for(i=0;i<76;i++) {
            memcpy(boundary-i,pth,i);
            CHECK(!bh_scene_routes_decode(&routes,boundary-i,i,3) && routes.error==BH_SCENE_TRUNCATED,"every partial route prefix rejected at real memory boundary");
            CHECK(routes.cells==old_routes && memory.live==old_live,"route rejection preserves old owner");
        }
        CHECK(VirtualFree(pages,0,MEM_RELEASE),"release guard pages");
    }
    memcpy(bad,lev,sizeof(bad));memset(bad+4,'X',16);
    CHECK(!bh_scene_level_decode(&level,bad,sizeof(bad)) && level.error==BH_SCENE_FORMAT,"unterminated placement name rejected");
    memcpy(bad,lev,sizeof(bad));put32(bad+68,2);
    CHECK(!bh_scene_level_decode(&level,bad,sizeof(bad)) && level.error==BH_SCENE_FORMAT,"invalid link-owner ordinal rejected");
    put32(bad,0xffffffffUL);CHECK(!bh_scene_level_decode(&level,bad,4) && level.error==BH_SCENE_FORMAT,"negative placement count rejected");
    put32(bad,65536UL);CHECK(!bh_scene_level_decode(&level,bad,4) && level.error==BH_SCENE_LIMIT,"placement count cap before size multiplication");
    CHECK(!bh_scene_routes_decode(&routes,pth,76,2) && routes.error==BH_SCENE_COUNT_MISMATCH,"cache/graph node count mismatch reported without fake recompute");
    CHECK(!bh_scene_routes_decode(&routes,pth,77,3) && routes.error==BH_SCENE_FORMAT,"route trailing bytes rejected");
    put32(bad,0x80000000UL);CHECK(!bh_scene_routes_decode(&routes,bad,4,3) && routes.error==BH_SCENE_FORMAT,"negative route dimension");
    put32(bad,2049);CHECK(!bh_scene_routes_decode(&routes,bad,4,2048) && routes.error==BH_SCENE_LIMIT,"route dimension cap");
    CHECK(!bh_scene_routes_decode(&routes,pth,76,0xffffffffUL) && routes.error==BH_SCENE_LIMIT,"unbounded expected count rejected");
    for(i=0;i<6;i++) {
        static const unsigned long invalid[]={0x7f800000UL,0xff800000UL,0x7fc12345UL,0x7f812345UL,0xbf800000UL,0x80000001UL};
        memcpy(bad,pth,76);put32(bad+8,invalid[i]);
        CHECK(!bh_scene_routes_decode(&routes,bad,76,3) && routes.error==BH_SCENE_FORMAT,"nonfinite/negative distance bits rejected without FP operations");
    }
    memcpy(bad,pth,76);put32(bad+4,6);CHECK(!bh_scene_routes_decode(&routes,bad,76,3),"exit slot six rejected");
    put32(bad+4,0xfffffffeUL);CHECK(!bh_scene_routes_decode(&routes,bad,76,3),"exit slot below minus one rejected");
    CHECK(!bh_scene_level_decode(&level,NULL,0) && level.error==BH_SCENE_ARGUMENT,"null level input");
    CHECK(!bh_scene_level_decode(&level,lev,64UL*1024*1024+1) && level.error==BH_SCENE_LIMIT,"input byte cap before dereference");
    CHECK(!bh_scene_routes_decode(&routes,pth,64UL*1024*1024+1,3) && routes.error==BH_SCENE_LIMIT,"route byte cap before dereference");
    for(step=1;step<=2;step++) {
        memory.fail_at=memory.calls+step;
        CHECK(!bh_scene_level_decode(&level,lev,sizeof(lev)) && level.error==BH_SCENE_MEMORY,"fail every level output allocation");
        CHECK(level.placements==old && memory.live==old_live,"allocation failure rolls back candidate");memory.fail_at=0;
    }
    memory.fail_at=memory.calls+1;
    CHECK(!bh_scene_routes_decode(&routes,pth,76,3) && routes.error==BH_SCENE_MEMORY && routes.cells==old_routes && memory.live==old_live,"route allocation rollback");memory.fail_at=0;
    for(i=0;i<30;i++)CHECK(bh_scene_level_decode(&level,lev,sizeof(lev)) && bh_scene_routes_decode(&routes,pth,76,3) && memory.live==3,"repeated source reload ownership");
    CHECK(bh_scene_level_decode(&level,empty,4) && !level.placements && !level.links && !level.placement_count && !level.link_count,"empty level valid and releases old payload");
    CHECK(bh_scene_routes_decode(&routes,empty,4,0) && !routes.cells && !routes.node_count && !memory.live,"empty matching cache releases old payload");
    bh_scene_level_dispose(&level);bh_scene_routes_dispose(&routes);
    CHECK(!memory.live,"all output owners released");
}

static void file_readers(void) {
    BHSceneLevel level;BHSceneRoutes routes;
    BHVFile *file;unsigned char prefix[5+108];unsigned long step,live;
    Memory memory={0,0,0};BHSceneAllocator allocator=hooks(&memory);
    fixture();bh_scene_level_init(&level,&allocator);bh_scene_routes_init(&routes,&allocator);
    memset(prefix,0x5a,5);memcpy(prefix+5,lev,108);
    CHECK(write_file("bhscene_level.tmp",prefix,sizeof(prefix)) && write_file("bhscene_routes.tmp",pth,76),"write loose scene fixtures");
    file=bh_vfile_open("bhscene_level.tmp","rb");CHECK(file!=NULL,"open borrowed loose level");
    if(file) {
        CHECK(!bh_scene_level_read(&level,file) && bh_vfile_tell(file)==0,"bad prefix restores initial cursor");
        CHECK(!bh_vfile_seek(file,5,SEEK_SET) && bh_scene_level_read(&level,file) && bh_vfile_tell(file)==113,"read remaining payload from nonzero cursor");
        live=memory.live;
        for(step=1;step<=3;step++) {
            CHECK(!bh_vfile_seek(file,5,SEEK_SET),"rewind borrowed level");memory.fail_at=memory.calls+step;
            CHECK(!bh_scene_level_read(&level,file) && level.error==BH_SCENE_MEMORY && bh_vfile_tell(file)==5 && memory.live==live,"blob/record allocation rollback restores cursor and owner");memory.fail_at=0;
        }
        CHECK(!bh_vfile_close(file),"reader never closes caller handle");
    }
    file=bh_vfile_open("bhscene_routes.tmp","rb");CHECK(file!=NULL,"open borrowed route cache");
    if(file) {
        CHECK(!bh_scene_routes_read(&routes,file,2) && routes.error==BH_SCENE_COUNT_MISMATCH && bh_vfile_tell(file)==0,"mismatched cache preserves cursor");
        live=memory.live;
        for(step=1;step<=2;step++) {
            memory.fail_at=memory.calls+step;
            CHECK(!bh_scene_routes_read(&routes,file,3) && routes.error==BH_SCENE_MEMORY && bh_vfile_tell(file)==0 && memory.live==live,"route blob/output allocation rollback");memory.fail_at=0;
        }
        CHECK(bh_scene_routes_read(&routes,file,3) && bh_vfile_tell(file)==76,"load route cache through real VFILE");
        CHECK(!bh_vfile_close(file),"close borrowed route handle");
    }
    CHECK(!bh_scene_level_read(&level,NULL) && level.error==BH_SCENE_ARGUMENT,"null borrowed level handle");
    bh_scene_level_dispose(&level);bh_scene_routes_dispose(&routes);CHECK(!memory.live,"file reader temporary/output allocations balanced");
    bh_vfile_shutdown();remove("bhscene_level.tmp");remove("bhscene_routes.tmp");
}

static void boundaries(void) {
    BHSceneLevel level;BHSceneRoutes routes;
    unsigned char *bytes;unsigned long size,i;
    bh_scene_level_init(&level,NULL);bh_scene_routes_init(&routes,NULL);
    size=4+65535UL*32;bytes=(unsigned char*)calloc(1,size);
    CHECK(bytes!=NULL,"allocate maximum level fixture");
    if(bytes) {
        put32(bytes,65535);
        CHECK(bh_scene_level_decode(&level,bytes,size) && level.placement_count==65535,"maximum placement count supported");
        free(bytes);
    }
    size=4+32+65535UL*20;bytes=(unsigned char*)calloc(1,size+20);
    CHECK(bytes!=NULL,"allocate maximum link fixture");
    if(bytes) {
        put32(bytes,1);
        CHECK(bh_scene_level_decode(&level,bytes,size) && level.link_count==65535,"maximum link count supported");
        CHECK(!bh_scene_level_decode(&level,bytes,size+20) && level.error==BH_SCENE_LIMIT && level.link_count==65535,"link count cap preserves prior owner");free(bytes);
    }
    size=4+2048UL*2048*8;bytes=(unsigned char*)calloc(1,size);
    CHECK(bytes!=NULL,"allocate maximum bounded route fixture");
    if(bytes) {
        put32(bytes,2048);
        CHECK(bh_scene_routes_decode(&routes,bytes,size,2048) && routes.node_count==2048,"maximum route dimension multiplication and copy");
        /* -1 and slots 0..5 are all accepted in either source/destination. */
        for(i=0;i<7;i++)put32(bytes+4+i*8,i ? i-1:0xffffffffUL);
        CHECK(bh_scene_routes_decode(&routes,bytes,size,2048),"all observed local exit slots accepted");free(bytes);
    }
    bh_scene_level_dispose(&level);bh_scene_routes_dispose(&routes);
}

/* Locate preserved payloads using the raw directory, independently of VFILE. */
static unsigned char *raw_asset(FILE *archive,const char *name,unsigned long *length) {
    unsigned char header[4],entry[256];unsigned long count,i,start,end;unsigned char *data;
    if(fseek(archive,0,SEEK_SET) || fread(header,1,4,archive)!=4)return NULL;
    count=get32(header);
    for(i=0;i<count;i++) {
        if(fread(entry,1,256,archive)!=256)return NULL;
        if(memchr(entry,0,248) && !_stricmp((char*)entry,name)) {
            start=get32(entry+248);end=get32(entry+252);if(end<start)return NULL;*length=end-start;
            data=(unsigned char*)malloc(*length);if(!data)return NULL;
            if(fseek(archive,start,SEEK_SET) || fread(data,1,*length,archive)!=*length){free(data);return NULL;}
            return data;
        }
    }
    return NULL;
}
static void compare(BHSceneLevel *level,BHSceneRoutes *routes,const unsigned char *raw_level,const unsigned char *raw_routes,const char *backend) {
    unsigned long i,n=level->placement_count,links=level->link_count;
    CHECK(n==5815 && links==543 && routes->node_count==283,"real decoded scene counts");
    if(n!=5815 || links!=543 || routes->node_count!=283)return;
    for(i=0;i<n;i++)CHECK(!memcmp(&level->placements[i],raw_level+4+i*32,32),"every real placement field/order matches independent raw bytes");
    for(i=0;i<links;i++)CHECK(!memcmp(&level->links[i],raw_level+4+n*32+i*20,20),"every real opaque link record matches raw bytes");
    for(i=0;i<283UL*283;i++)CHECK(!memcmp(&routes->cells[i],raw_routes+4+i*8,8),"every real route exit/distance bit matches raw bytes");
    CHECK(!strcmp(level->placements[0].name,"H_SQUA1.BSP") && level->placements[0].x==-920 && level->placements[0].y==-30 && level->placements[0].z==1410 && level->placements[0].flags==3,"independent first placement semantic control");
    printf("{\"scenario\":\"scene-real-assets\",\"backend\":\"%s\",\"placements\":%lu,\"links\":%lu,\"route_nodes\":283,\"route_cells\":80089}\n",backend,n,links);
}
static void real_assets(const char *path) {
    FILE *raw;
    unsigned char *raw_level,*raw_routes;unsigned long level_bytes=0,route_bytes=0;
    BHVFile *level_file,*route_file;
    BHSceneLevel level;BHSceneRoutes routes;
    int backend,ok;
    raw=fopen(path,"rb");CHECK(raw!=NULL,"open preserved archive as data");if(!raw)return;
    raw_level=raw_asset(raw,"TEST.LEV",&level_bytes);raw_routes=raw_asset(raw,"TEST.PTH",&route_bytes);fclose(raw);
    CHECK(raw_level && raw_routes,"independent raw directory finds both scene assets");
    if(!raw_level || !raw_routes){free(raw_level);free(raw_routes);return;}
    CHECK(level_bytes==196944 && route_bytes==640716 && crc32(raw_level,level_bytes)==0xa16870eaUL && crc32(raw_routes,route_bytes)==0x7cb20e83UL,"independent complete payload fingerprints");
    bh_scene_level_init(&level,NULL);bh_scene_routes_init(&routes,NULL);
    for(backend=0;backend<3;backend++) {
        CHECK(bh_vfile_mount(path),"mount real scene data");
        if(backend==1)CHECK(bh_vfile_preload("TEST.LEV","rb") && bh_vfile_preload("TEST.PTH","rb"),"cache both scene assets");
        if(backend==2)CHECK(write_file("bhscene_real_level.tmp",raw_level,level_bytes) && write_file("bhscene_real_routes.tmp",raw_routes,route_bytes),"private loose copies of real scene data");
        /* Open both before either read to exercise the archive's shared cursor. */
        level_file=bh_vfile_open(backend==2?"bhscene_real_level.tmp":"TEST.LEV","rb");
        route_file=bh_vfile_open(backend==2?"bhscene_real_routes.tmp":"TEST.PTH","rb");
        CHECK(level_file && route_file,"open real scene pair");
        ok=level_file && route_file && bh_scene_level_read(&level,level_file) && bh_scene_routes_read(&routes,route_file,283);
        CHECK(ok,"real level and matching cached-route data decode together");
        if(ok)compare(&level,&routes,raw_level,raw_routes,backend==0?"embedded":backend==1?"preloaded":"loose");
        if(level_file)bh_vfile_close(level_file);if(route_file)bh_vfile_close(route_file);
        bh_vfile_shutdown();
    }
    bh_scene_level_dispose(&level);bh_scene_routes_dispose(&routes);free(raw_level);free(raw_routes);
    remove("bhscene_real_level.tmp");remove("bhscene_real_routes.tmp");
}

int bh_scene_checkpoint(const char *archive,unsigned long *total) {
    checks=0;failures=0;memory_readers();file_readers();boundaries();
    if(archive)real_assets(archive);
    *total=checks;
    printf("{\"scenario\":\"scene-readers\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures ? 1:0;
}
