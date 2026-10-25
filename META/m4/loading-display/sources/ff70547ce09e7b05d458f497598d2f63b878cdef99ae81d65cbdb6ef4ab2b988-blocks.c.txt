#include "../src/bh_blocks.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <windows.h>
#include <direct.h>

static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<=12)printf("FAIL BSP line %d: %s\n",line,label);}}
static void put32(unsigned char *p,unsigned long v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
static unsigned long word(const unsigned char *p){return (unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);}
static unsigned long bits(float v){unsigned long w;memcpy(&w,&v,4);return w;}
static void putfloat(unsigned char *p,float v){memcpy(p,&v,4);}
static unsigned long crc32(const void *bytes,unsigned long size){const unsigned char *p=(const unsigned char*)bytes;unsigned long crc=0xffffffffUL,i;int b;for(i=0;i<size;i++){crc^=p[i];for(b=0;b<8;b++)crc=(crc>>1)^(0xedb88320UL&(0UL-(crc&1)));}return crc^0xffffffffUL;}
static int write_file(const char *path,const void *bytes,unsigned long size){FILE *f=fopen(path,"wb");int ok;if(!f)return 0;ok=fwrite(bytes,1,size,f)==size;return fclose(f)==0 && ok;}
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long size,void *user){Memory *m=(Memory*)user;unsigned char *p;++m->calls;if(m->fail_at && m->calls==m->fail_at)return NULL;p=(unsigned char*)malloc(size+20);if(!p)return NULL;put32(p,size);put32(p+4,0x752ad901UL);put32(p+16+size,0x8923ab34UL);++m->live;return p+16;}
static void __cdecl release(void *pointer,void *user){Memory *m=(Memory*)user;unsigned char *p=(unsigned char*)pointer-16;CHECK(m->live && word(p+4)==0x752ad901UL && word(p+16+word(p))==0x8923ab34UL,"allocation guards and exact ownership release");--m->live;free(p);}
static BHSceneAllocator hooks(Memory *m){BHSceneAllocator a;a.allocate=allocate;a.release=release;a.user=m;return a;}

static unsigned char fixture[208];
static const unsigned long offsets[]={4,40,104,124,140,200},strides[]={32,60,16,12,56,8};
static void make_fixture(void){unsigned long i;memset(fixture,0,sizeof(fixture));for(i=0;i<6;i++)put32(fixture+offsets[i]-4,1);strcpy((char*)fixture+13,"sample.pcx");memset(fixture+52,0x78,48);put32(fixture+108,0xffffffffUL);put32(fixture+132,0xdeadbeefUL);putfloat(fixture+152,6);putfloat(fixture+156,8);putfloat(fixture+168,1);put32(fixture+184,0xffffffffUL);put32(fixture+188,0xffffffffUL);put32(fixture+192,0xffffffffUL);put32(fixture+204,0xffffffffUL);}

static void decode_tests(void){
    BHBsp bsp;Memory memory={0,0,0};BHSceneAllocator allocator=hooks(&memory);
    unsigned char bad[209],*pages,*end;unsigned char *old;unsigned long i,step,live,control,previous;SYSTEM_INFO system;
    make_fixture();CHECK(bh_bsp_init(&bsp,&allocator),"initialize BSP owner");
    CHECK(bh_bsp_decode(&bsp,fixture,208) && !memcmp(bsp.storage,fixture,208),"decode complete six-section payload without rewriting bytes");
    CHECK(bsp.bounds[0].center.x==3 && bsp.bounds[0].center.y==4 && bsp.bounds[0].center.z==0 && bsp.bounds[0].radius_squared==25 && bsp.bounds[0].radius==5 && bsp.root_radius==10,"analytical 6-8-10 collision and root bounds");
    CHECK(bsp.faces[0].runtime==(long)0xdeadbeefUL && bsp.vertices[0].opaque[47]==0x78,"opaque runtime words retained as data, not trusted pointers");
    old=bsp.storage;live=memory.live;
    GetSystemInfo(&system);pages=(unsigned char*)VirtualAlloc(NULL,system.dwPageSize*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    CHECK(pages!=NULL,"allocate input guard pages");
    if(pages){CHECK(VirtualProtect(pages+system.dwPageSize,system.dwPageSize,PAGE_NOACCESS,&previous),"protect exact input boundary");end=pages+system.dwPageSize;
        for(i=0;i<208;i++){memcpy(end-i,fixture,i);CHECK(!bh_bsp_decode(&bsp,end-i,i) && bsp.error==BH_BSP_TRUNCATED,"reject every short BSP prefix at guard page");CHECK(bsp.storage==old && memory.live==live,"truncation retains old payload");}VirtualFree(pages,0,MEM_RELEASE);}
    memcpy(bad,fixture,208);bad[208]=1;CHECK(!bh_bsp_decode(&bsp,bad,209) && bsp.error==BH_BSP_FORMAT,"reject trailing incompatible data");
    memcpy(bad,fixture,208);memset(bad+13,'X',23);CHECK(!bh_bsp_decode(&bsp,bad,208) && bsp.error==BH_BSP_FORMAT,"unterminated material texture name");
    for(i=0;i<6;i++){memcpy(bad,fixture,208);put32(bad+offsets[i]-4,65536);CHECK(!bh_bsp_decode(&bsp,bad,208) && bsp.error==BH_BSP_LIMIT,"every count cap before multiplication");}
    memcpy(bad,fixture,208);put32(bad,0);CHECK(!bh_bsp_decode(&bsp,bad,208) && bsp.error==BH_BSP_FORMAT,"nonempty section contract");
    for(i=0;i<9;i++){static const unsigned long invalid_offsets[]={104,108,124,128,180,184,188,192,200};memcpy(bad,fixture,208);put32(bad+invalid_offsets[i],1);CHECK(!bh_bsp_decode(&bsp,bad,208) && bsp.error==BH_BSP_FORMAT,"invalid geometry reference rejected");}
    for(i=0;i<3;i++){static const unsigned long cyclic_offsets[]={108,188,204};memcpy(bad,fixture,208);put32(bad+cyclic_offsets[i],0);CHECK(!bh_bsp_decode(&bsp,bad,208) && bsp.error==BH_BSP_CYCLE,"edge/node/leaf cycles rejected without recursive overflow");}
    memcpy(bad,fixture,208);putfloat(bad+140,7);CHECK(!bh_bsp_decode(&bsp,bad,208) && bsp.error==BH_BSP_FORMAT,"inverted node bounds rejected");
    for(i=0;i<4;i++){static const unsigned long f_offsets[]={40,112,140,176};memcpy(bad,fixture,208);put32(bad+f_offsets[i],0x7f812345UL);CHECK(!bh_bsp_decode(&bsp,bad,208) && bsp.error==BH_BSP_FORMAT,"nonfinite geometry words rejected");}
    control=_control87(0,0);_control87(_PC_24,_MCW_PC);
    CHECK(!bh_bsp_decode(&bsp,fixture,208) && bsp.error==BH_BSP_NUMERICAL,"derived geometry requires explicit numerical profile");_control87(control,_MCW_PC);
    for(step=1;step<=3;step++){memory.fail_at=memory.calls+step;CHECK(!bh_bsp_decode(&bsp,fixture,208) && bsp.error==BH_BSP_MEMORY && bsp.storage==old && memory.live==live,"every work/storage/bounds allocation rollback");memory.fail_at=0;}
    for(i=0;i<30;i++)CHECK(bh_bsp_decode(&bsp,fixture,208) && memory.live==2,"repeat geometry reload and disposal ownership");
    CHECK(!bh_bsp_decode(&bsp,fixture,16UL*1024*1024+1) && bsp.error==BH_BSP_LIMIT,"BSP input byte cap");
    bh_bsp_dispose(&bsp);bh_bsp_dispose(&bsp);CHECK(!memory.live && !bsp.storage,"geometry final ownership balanced");
}

static void file_and_registry_tests(void){
    BHBsp bsp;BHBlockLibrary library;BHVFile *file;Memory memory={0,0,0};BHSceneAllocator allocator=hooks(&memory);
    unsigned char *old;BHBlockData *old_blocks;unsigned long step,calls,live,i;
    const char *list=";comment\r\nBHTEST1.BSP\r\nBHTEST1.BSP\r\nEnD\r\nignored trailing data";
    const char *script="render\r\n; retained, NOT executed\r\n";
    make_fixture();_mkdir("bhbsp_prefix");
    CHECK(write_file("BHTEST1.BSP",fixture,208) && write_file("bhbsp_prefix/BHTEST1.BSP",fixture,208),"write own search fixtures");
    CHECK(write_file("bhbsp_prefix/BHTEST1.TXT",script,(unsigned long)strlen(script)) && write_file("bhbsp_list.tmp",list,(unsigned long)strlen(list)),"write own registry/sidecar fixtures");
    bh_bsp_init(&bsp,&allocator);bh_blocks_init(&library,&allocator);
    CHECK(bh_bsp_load(&bsp,"BHTEST1.BSP","bhbsp_prefix/",BH_BSP_PREFIX_ONLY),"load prefixed geometry");old=bsp.storage;live=memory.live;
    file=bh_vfile_open("BHTEST1.BSP","rb");CHECK(file!=NULL,"open borrowed BSP");
    if(file){for(step=1;step<=4;step++){bh_vfile_seek(file,0,SEEK_SET);memory.fail_at=memory.calls+step;CHECK(!bh_bsp_read(&bsp,file) && bsp.error==BH_BSP_MEMORY && bsp.storage==old && memory.live==live && bh_vfile_tell(file)==0,"borrowed blob/work/output allocation rollback and cursor");memory.fail_at=0;}CHECK(bh_bsp_read(&bsp,file) && bh_vfile_tell(file)==208,"successful borrowed BSP read");bh_vfile_close(file);}
    /* Distinguish search modes using a different valid prefixed bound. */
    putfloat(fixture+152,0);putfloat(fixture+156,0);putfloat(fixture+160,12);CHECK(write_file("bhbsp_prefix/BHTEST1.BSP",fixture,208),"distinct prefixed geometry");
    CHECK(bh_bsp_load(&bsp,"BHTEST1.BSP","bhbsp_prefix/",BH_BSP_PREFIX_FIRST) && bsp.root_radius==12,"prefix-first policy");
    CHECK(bh_bsp_load(&bsp,"BHTEST1.BSP","bhbsp_prefix/",BH_BSP_BARE_FIRST) && bsp.root_radius==10,"bare-first policy");
    CHECK(bh_bsp_load(&bsp,"BHTEST1.BSP","bhbsp_prefix/",BH_BSP_BARE_ONLY) && bsp.root_radius==10,"bare-only policy");
    CHECK(bh_bsp_load(&bsp,"BHTEST1.BSP","bhbsp_prefix/",BH_BSP_PREFIX_ONLY) && bsp.root_radius==12,"prefix-only policy");
    CHECK(bh_bsp_load(&bsp,"BHTEST1.BSP","bhbsp_absent/",BH_BSP_PREFIX_FIRST) && bsp.root_radius==10,"prefix-first fallback to bare");
    remove("BHTEST1.BSP");CHECK(bh_bsp_load(&bsp,"BHTEST1.BSP","bhbsp_prefix/",BH_BSP_BARE_FIRST),"bare-first fallback");
    calls=memory.calls;CHECK(bh_blocks_load(&library,"bhbsp_list.tmp","bhbsp_prefix/",BH_BSP_PREFIX_ONLY),"load owned names, geometry and unapplied scripts");calls=memory.calls-calls;
    CHECK(library.count==2 && library.unapplied_scripts==2 && library.blocks[0].script_present && library.blocks[0].script_bytes==strlen(script) && !memcmp(library.blocks[0].script,script,strlen(script)),"duplicate registry order and exact sidecar bytes");
    CHECK(bh_blocks_find(&library,"bhtest1.bsp")==0 && bh_blocks_find(&library,"unknown.bsp")==-1,"first case-insensitive match / unknown explicit");
    old_blocks=library.blocks;live=memory.live;
    for(step=1;step<=calls;step++){memory.fail_at=memory.calls+step;CHECK(!bh_blocks_load(&library,"bhbsp_list.tmp","bhbsp_prefix/",BH_BSP_PREFIX_ONLY) && library.error==BH_BSP_MEMORY && library.blocks==old_blocks && memory.live==live,"failure at every nested registry/BSP/script allocation retains old library");memory.fail_at=0;}
    CHECK(write_file("bhbsp_list.tmp","BHMISSING.BSP\nend\n",18),"write missing dependency list");
    CHECK(!bh_blocks_load(&library,"bhbsp_list.tmp","bhbsp_prefix/",BH_BSP_PREFIX_ONLY) && library.error==BH_BSP_IO && library.blocks==old_blocks && memory.live==live,"missing model is rejection, never success stub");
    CHECK(!bh_blocks_decode_names(&library," leading.BSP\n",13) && library.error==BH_BSP_FORMAT,"do not silently trim unknown original names");
    CHECK(!bh_blocks_decode_names(&library,"../bad.BSP\n",11) && library.error==BH_BSP_FORMAT,"unsafe registry path rejected");
    for(i=0;i<20;i++)CHECK(bh_blocks_decode_names(&library,list,(unsigned long)strlen(list)) && library.count==2 && !library.unapplied_scripts && !library.blocks[0].bsp.storage,"data-only names do not fabricate loaded models");
    bh_blocks_dispose(&library);bh_bsp_dispose(&bsp);CHECK(!memory.live,"nested library and geometry disposal balanced");
    remove("bhbsp_prefix/BHTEST1.BSP");remove("bhbsp_prefix/BHTEST1.TXT");remove("bhbsp_list.tmp");_rmdir("bhbsp_prefix");bh_vfile_shutdown();
}

static void caps(void){
    unsigned char *bytes,*names,*p;unsigned long size=24,i,j,position,node_offset=0;
    BHBsp bsp;BHBlockLibrary library;unsigned char *old;
    make_fixture();bh_bsp_init(&bsp,NULL);bh_blocks_init(&library,NULL);
    for(i=0;i<6;i++)size+=65535UL*strides[i];
    bytes=(unsigned char*)malloc(size);CHECK(bytes!=NULL,"allocate simultaneous exact section-cap fixture");
    if(bytes){position=0;for(i=0;i<6;i++){put32(bytes+position,65535);position+=4;if(i==4)node_offset=position;
        for(j=0;j<65535;j++){p=bytes+position+j*strides[i];memcpy(p,fixture+offsets[i],strides[i]);if(i==4 && j<65534)put32(p+48,j+1);}position+=65535UL*strides[i];}
        CHECK(bh_bsp_decode(&bsp,bytes,size) && bsp.counts[0]==65535 && bsp.counts[5]==65535 && bsp.bounds[65534].radius==5,"all six exact count caps, including deep iterative node traversal");
        old=bsp.storage;put32(bytes+node_offset+65534UL*56+52,0);
        CHECK(!bh_bsp_decode(&bsp,bytes,size) && bsp.error==BH_BSP_CYCLE && bsp.storage==old,"deep back-edge cycle rejected with old owner intact");free(bytes);}
    names=(unsigned char*)malloc(601*6);CHECK(names!=NULL,"allocate registry count-cap fixture");
    if(names){for(i=0;i<601;i++)memcpy(names+i*6,"X.BSP\n",6);
        CHECK(bh_blocks_decode_names(&library,names,600*6) && library.count==600,"registry exact 600-entry cap");
        CHECK(!bh_blocks_decode_names(&library,names,601*6) && library.error==BH_BSP_LIMIT && library.count==600,"registry over-count rollback");
        memset(names,'X',512);names[0]=';';names[511]='\n';
        CHECK(bh_blocks_decode_names(&library,names,512) && !library.count,"511-byte comment line accepted");
        names[511]='X';names[512]='\n';CHECK(!bh_blocks_decode_names(&library,names,513) && library.error==BH_BSP_LIMIT,"overlong line rejected");
        CHECK(!bh_blocks_decode_names(&library,names,65537) && library.error==BH_BSP_LIMIT,"text byte cap checked before access");free(names);}
    bh_blocks_dispose(&library);bh_bsp_dispose(&bsp);
}

typedef struct Manifest {const char *name;unsigned long offset,size,crc,counts[6],bounds_crc,root;int sidecar;unsigned long script_bytes,script_crc;} Manifest;
static const Manifest manifest[]={
#include "bsp_manifest.h"
};
static const char *block_names[]={
#include "block_names.h"
};
static const Manifest *find_manifest(const char *name){unsigned long i;for(i=0;i<505;i++)if(!_stricmp(manifest[i].name,name))return manifest+i;return NULL;}
static void compare(const BHBsp *bsp,const Manifest *expected,const unsigned char *raw){
    static int diagnostic;
    const void *views[6];unsigned long at=0,i,n;
    CHECK(bsp->bytes==expected->size && crc32(bsp->storage,bsp->bytes)==expected->crc,"complete real BSP payload fingerprint");
    views[0]=bsp->materials;views[1]=bsp->vertices;views[2]=bsp->edges;views[3]=bsp->faces;views[4]=bsp->nodes;views[5]=bsp->leaves;
    for(i=0;i<6;i++){n=word(raw+at);at+=4;CHECK(n==expected->counts[i] && n==bsp->counts[i] && !memcmp(views[i],raw+at,n*strides[i]),"every raw section byte/index/order retained");at+=n*strides[i];}
    if(!diagnostic && (crc32(bsp->bounds,bsp->counts[4]*sizeof(BHBspBound))!=expected->bounds_crc || bits(bsp->root_radius)!=expected->root)) {
        printf("FIRST BSP bounds %s nodes=%lu root=%08lx expected=%08lx\n",expected->name,bsp->counts[4],bits(bsp->root_radius),expected->root);
        write_file("bsp-failed-bounds.bin",bsp->bounds,bsp->counts[4]*sizeof(BHBspBound));diagnostic=1;
    }
    CHECK(crc32(bsp->bounds,bsp->counts[4]*sizeof(BHBspBound))==expected->bounds_crc && bits(bsp->root_radius)==expected->root,"all derived collision bounds match independent Python binary32 controls");
}
static void real_assets(const char *archive){
    FILE *raw;unsigned long i,backend,files=0,placed=0;unsigned char *bytes;BHBsp bsp;BHVFile *file;BHBlockLibrary library;BHSceneLevel level;const Manifest *expected;
    raw=fopen(archive,"rb");CHECK(raw!=NULL,"open pinned archive for independent raw reads");if(!raw)return;
    bh_bsp_init(&bsp,NULL);bh_blocks_init(&library,NULL);bh_scene_level_init(&level,NULL);
    for(backend=0;backend<3;backend++) {
        CHECK(bh_vfile_mount(archive),"mount geometry data");
        for(i=0;i<505;i++) {
            bytes=(unsigned char*)malloc(manifest[i].size);CHECK(bytes!=NULL,"allocate independent raw payload");if(!bytes)break;
            CHECK(!fseek(raw,manifest[i].offset,SEEK_SET) && fread(bytes,1,manifest[i].size,raw)==manifest[i].size && crc32(bytes,manifest[i].size)==manifest[i].crc,"independent raw BSP fingerprint control");
            if(backend==1)CHECK(bh_vfile_preload(manifest[i].name,"rb"),"preload real geometry");
            if(backend==2)CHECK(write_file("bhbsp_real.tmp",bytes,manifest[i].size),"write private loose real geometry");
            file=bh_vfile_open(backend==2?"bhbsp_real.tmp":manifest[i].name,"rb");CHECK(file!=NULL,"open real BSP");
            if(file){if(bh_bsp_read(&bsp,file)){compare(&bsp,manifest+i,bytes);++files;}else CHECK(0,"real BSP read/validation");bh_vfile_close(file);}free(bytes);
        }
        bh_vfile_shutdown();
    }
    CHECK(files==1515,"all 505 BSP assets through three backends");
    CHECK(bh_vfile_mount(archive) && bh_blocks_load(&library,"BSP\\BLOCKS.TXT","BSP\\",BH_BSP_PREFIX_ONLY),"real block list loads every model and retains sidecars");
    CHECK(library.count==454 && library.unapplied_scripts==202,"real block registry / unapplied script counts");
    if(library.count==454)for(i=0;i<454;i++) {
        char path[256];CHECK(!strcmp(library.blocks[i].name,block_names[i]),"independent original registry order");sprintf(path,"BSP\\%s",block_names[i]);expected=find_manifest(path);CHECK(expected!=NULL,"registry geometry is an independently captured BSP");
        if(expected){CHECK(crc32(library.blocks[i].bsp.storage,library.blocks[i].bsp.bytes)==expected->crc,"every registry-owned geometry exact");CHECK(library.blocks[i].script_present==expected->sidecar && library.blocks[i].script_bytes==expected->script_bytes && crc32(library.blocks[i].script,library.blocks[i].script_bytes)==expected->script_crc,"every retained sidecar payload exact and explicitly unapplied");}
    }
    file=bh_vfile_open("TEST.LEV","rb");CHECK(file!=NULL && bh_scene_level_read(&level,file),"compose registry with source-decoded real level");
    if(file)bh_vfile_close(file);
    for(i=0;i<level.placement_count;i++){CHECK(bh_blocks_find(&library,level.placements[i].name)>=0,"every real placement resolves to loaded model data");if(bh_blocks_find(&library,level.placements[i].name)>=0)++placed;}
    CHECK(placed==5815,"complete real placement/model name closure");
    printf("{\"scenario\":\"blocks-real-assets\",\"bsp_files\":505,\"backend_decodes\":%lu,\"registry_models\":%lu,\"unapplied_scripts\":%lu,\"resolved_placements\":%lu}\n",files,library.count,library.unapplied_scripts,placed);
    bh_scene_level_dispose(&level);bh_blocks_dispose(&library);bh_bsp_dispose(&bsp);bh_vfile_shutdown();fclose(raw);remove("bhbsp_real.tmp");
}
int bh_blocks_checkpoint(const char *archive,unsigned long *total){checks=0;failures=0;decode_tests();file_and_registry_tests();caps();if(archive)real_assets(archive);*total=checks;printf("{\"scenario\":\"bsp-block-data\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;}
